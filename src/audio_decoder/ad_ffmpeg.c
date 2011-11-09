#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include <gtk/gtk.h>
#include "typedefs.h"
#include "support.h"
#include "audio_decoder/ad_plugin.h"

#ifdef HAVE_FFMPEG

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

typedef struct {
  AVFormatContext* formatContext;
  AVCodecContext*  codecContext;
  AVCodec*         codec;
  AVPacket         packet;
  int              audioStream;

  int16_t          m_tmpBuffer[AVCODEC_MAX_AUDIO_FRAME_SIZE];
  int16_t*         m_tmpBufferStart;
  unsigned long    m_tmpBufferLen;
  int64_t          m_pts;
#ifdef CLKDEBUG
  double           decoder_clock;
  double           output_clock;
  uint32_t         seek_frame;
#endif
  unsigned int     samplerate;
  unsigned int     channels;
  int64_t          length;
} ffmpeg_audio_decoder;


/* internal abstraction */
void *ad_open_ffmpeg(const char *fn, struct adinfo *nfo) {
  ffmpeg_audio_decoder *priv = (ffmpeg_audio_decoder*) calloc(1, sizeof(ffmpeg_audio_decoder));
  
  priv->m_tmpBufferStart=NULL;
  priv->m_tmpBufferLen=priv->m_pts=0;
#ifdef CLKDEBUG
  priv->decoder_clock=priv->output_clock=0.0; 
#endif

  if (avformat_open_input(&priv->formatContext, fn, NULL, NULL) <0) {
    dbg(0, "ffmpeg is unable to open file '%s'.", fn);
    free(priv); return(NULL);
  }

  if (av_find_stream_info(priv->formatContext) < 0) {
    av_close_input_file(priv->formatContext);
    dbg(0, "av_find_stream_info failed" );
    free(priv); return(NULL);
  }

  priv->audioStream = -1;
  int i;
  for (i=0; i<priv->formatContext->nb_streams; i++) {
    if (priv->formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
      priv->audioStream = i;
      break;
    }
  }
  if (priv->audioStream == -1) {
    dbg(0, "No Audio Stream found in file");
    av_close_input_file(priv->formatContext);
    free(priv); return(NULL);
  }

  priv->codecContext = priv->formatContext->streams[priv->audioStream]->codec;
  priv->codec        = avcodec_find_decoder(priv->codecContext->codec_id);

  if (priv->codec == NULL) {
    av_close_input_file(priv->formatContext);
    dbg(0, "Codec not supported by ffmpeg");
    free(priv); return(NULL);
  }
  if (avcodec_open(priv->codecContext, priv->codec) < 0) {
    dbg(0, "avcodec_open failed" );
    free(priv); return(NULL);
  }

  dbg(2, "ffmpeg - audio tics: %i/%i [sec]",priv->formatContext->streams[priv->audioStream]->time_base.num,priv->formatContext->streams[priv->audioStream]->time_base.den);

  int64_t len = priv->formatContext->duration - priv->formatContext->start_time;

  priv->formatContext->flags|=AVFMT_FLAG_GENPTS;
  priv->formatContext->flags|=AVFMT_FLAG_IGNIDX;

  priv->samplerate = priv->codecContext->sample_rate;
  priv->channels   = priv->codecContext->channels ;
  priv->length     = (int64_t)( len * priv->samplerate / AV_TIME_BASE );

  nfo->sample_rate = priv->samplerate;
  nfo->channels    = priv->channels;
  nfo->frames      = priv->length;
  nfo->length      = (nfo->frames * 1000) / nfo->sample_rate;

  dbg(1, "ffmpeg - %s", fn);
  dbg(1, "ffmpeg - sr:%i c:%i d:%lld f:%lld", nfo->sample_rate, nfo->channels, nfo->length, nfo->frames);

  return (void*) priv;
}

int ad_close_ffmpeg(void *sf) {
  ffmpeg_audio_decoder *priv = (ffmpeg_audio_decoder*) sf;
  if (!priv) return -1;
  avcodec_close(priv->codecContext);
  av_close_input_file(priv->formatContext);
  free(priv);
  return 0;
}

void int16_to_double(int16_t *in, double *out, int num_channels, int num_samples, int out_offset) {
  int i,ii;
  for (i=0;i<num_samples;i++) {
    for (ii=0;ii<num_channels;ii++) {
     out[(i+out_offset)*num_channels+ii]= (double) in[i*num_channels+ii]/ 32768.0;
    }
  }
}

ssize_t ad_read_ffmpeg(void *sf, double* d, size_t len) {
  ffmpeg_audio_decoder *priv = (ffmpeg_audio_decoder*) sf;
  if (!priv) return -1;
  size_t frames = len / priv->channels;

  int written = 0;
  int ret = 0;
  while ( ret >= 0 && written < frames ) {
    dbg(3,"loop: %i/%i (bl:%lu)",written, frames, priv->m_tmpBufferLen );
    if ( priv->m_tmpBufferLen > 0 ) {
      int s = MIN(priv->m_tmpBufferLen / priv->channels, frames - written );
      int16_to_double(priv->m_tmpBufferStart, d, priv->channels, s , written);
      written += s;
#ifdef CLKDEBUG
      priv->output_clock+=s;
#endif
      s = s * priv->channels;
      priv->m_tmpBufferStart += s;
      priv->m_tmpBufferLen -= s;
      ret = 0;
    } else {
      ret = av_read_frame(priv->formatContext, &priv->packet);
      if (ret<1) { dbg(1, "reached end of file."); break; }

      int len = priv->packet.size;
      uint8_t *ptr = priv->packet.data;

      priv->m_tmpBufferStart = priv->m_tmpBuffer;
      while (ptr != NULL && ret >= 0 && priv->packet.stream_index == priv->audioStream && len > 0) {
        int data_size= AVCODEC_MAX_AUDIO_FRAME_SIZE - priv->m_tmpBufferLen - (priv->m_tmpBufferStart-priv->m_tmpBuffer);
        if(data_size < FF_MIN_BUFFER_SIZE || data_size < priv->codecContext->channels * priv->codecContext->frame_size * sizeof(int16_t)){
           dbg(0, "WARNING: audio buffer too small.");
           break;
        }

        ret = avcodec_decode_audio3(priv->codecContext, priv->m_tmpBufferStart+priv->m_tmpBufferLen, &data_size, &priv->packet);

        if ( ret < 0 ) {
          ret = 0;
          dbg(0, "audio decode error");
          break;
        }
        len -= ret;
        ptr += ret;
#ifdef CLKDEBUG
        if (priv->packet.pts != AV_NOPTS_VALUE) {
          priv->decoder_clock = priv->codecContext->sample_rate * av_q2d(priv->formatContext->streams[priv->audioStream]->time_base)*priv->packet.pts;
          dbg(2, "GOT PTS: %.1f. (want: %.1f)",priv->decoder_clock, priv->output_clock);
        }

        if (priv->packet.pts == AV_NOPTS_VALUE) {
          dbg(2, "got no PTS");
          priv->decoder_clock += (double) (data_size>>1) / priv->codecContext->channels;
        }
        dbg(3, "CLK: %.0f T:%.2f",priv->decoder_clock,priv->decoder_clock/priv->codecContext->sample_rate);
#endif
        if ( data_size > 0 ) {
                priv->m_tmpBufferLen+= (data_size>>1); // 2 bytes per sample
        }

      }
      av_free_packet( &priv->packet );
    }
  }
  if (written!=frames) {
        dbg(2, "short-read");
  }
  return written;
}
#endif

const static ad_plugin ad_ffmpeg = {
#ifdef HAVE_FFMPEG
  &ad_open_ffmpeg,
  &ad_close_ffmpeg,
  &ad_read_ffmpeg
#else
  &ad_open_null,
  &ad_close_null,
  &ad_read_null
#endif
};

/* dlopen handler */
const ad_plugin * get_ffmpeg() {
  static int ffinit = 0;
  if (!ffinit) {
    ffinit=1;
    av_register_all();
    avcodec_register_all();
    avcodec_init();
    if(0)
      av_log_set_level(AV_LOG_QUIET);
    else 
      av_log_set_level(AV_LOG_VERBOSE);
  }
  return &ad_ffmpeg;
}
