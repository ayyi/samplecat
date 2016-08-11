/**
* +----------------------------------------------------------------------+
* | This file is part of Samplecat. http://ayyi.github.io/samplecat/     |
* +----------------------------------------------------------------------+
* | This program is free software; you can redistribute it and/or modify |
* | it under the terms of the GNU General Public License version 3       |
* | as published by the Free Software Foundation.                        |
* +----------------------------------------------------------------------+
*
*/
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include <gtk/gtk.h>
#include "debug/debug.h"
#include "typedefs.h"
#include "support.h"
#include "audio_decoder/ad_plugin.h"

#ifdef HAVE_FFMPEG

#include "ffcompat.h"

typedef struct _ffmpeg_audio_decoder ffmpeg_audio_decoder;

typedef struct
{
    short*          buf[2];
    guint           size;           // number of shorts allocated, NOT bytes. When accessing, note that the last block will likely not be full.
} Buf16;

struct _ffmpeg_audio_decoder {
  AVFormatContext* format_context;
  AVCodecContext*  codec_context;
  AVCodec*         codec;
  AVPacket         packet;
  int              audio_stream;
  int              pkt_len;
  uint8_t*         pkt_ptr;

  AVFrame          frame;
  int              frame_iter;

  struct {
    int16_t        buf[AVCODEC_MAX_AUDIO_FRAME_SIZE];
    int16_t*       start;
    unsigned long  len;
  }                tmp_buf;

  int64_t          decoder_clock;
  int64_t          output_clock;
  int64_t          seek_frame;
  unsigned int     samplerate;
  unsigned int     channels;
  int64_t          length;

  ssize_t (*read)  (ffmpeg_audio_decoder*, float*, size_t len);
  ssize_t (*read_planar) (ffmpeg_audio_decoder*, Buf16*);
};

static ssize_t ad_ff_read_short_planar_to_planar            (ffmpeg_audio_decoder*, Buf16* buf);
static ssize_t ad_ff_read_short_planar_to_interleaved       (ffmpeg_audio_decoder*, float*, size_t);
static ssize_t ad_ff_read_short_interleaved_to_interleaved  (ffmpeg_audio_decoder*, float*, size_t);
static ssize_t ad_ff_read_float_planar_to_interleaved       (ffmpeg_audio_decoder*, float*, size_t);
static ssize_t ad_read_ffmpeg_default_interleaved           (ffmpeg_audio_decoder*, float*, size_t);


static gboolean
ad_metadata_array_set_tag_postion(GPtrArray* tags, const char* tag_name, int pos)
{
	// move a metadata item up (can only be up) to the specified position.
	// returns true if the tag was found.

	if((tags->len < 4) || (pos >= tags->len - 2)) return false;
	pos *= 2;

	char** data = (char**)tags->pdata;
	int i; for(i=pos;i<tags->len;i+=2){
		if(!strcmp(data[i], tag_name)){
			const char* artist[] = {data[i], (const char*)data[i+1]};

			int j; for(j=i;j>=pos;j-=2){
				data[j    ] = data[j - 2    ];
				data[j + 1] = data[j - 2 + 1];
			}
			data[pos    ] = (char*)artist[0];
			data[pos + 1] = (char*)artist[1];

			return true;
		}
	}
	return false;
}


int
ad_info_ffmpeg(void* sf, struct adinfo* nfo)
{
	ffmpeg_audio_decoder* priv = (ffmpeg_audio_decoder*) sf;
	if (!priv) return -1;

	if (nfo) {
		nfo->sample_rate = priv->samplerate;
		nfo->channels    = priv->channels;
		nfo->frames      = priv->length;
		if (!nfo->sample_rate) return -1;
		nfo->length      = (nfo->frames * 1000) / nfo->sample_rate;
		nfo->bit_rate    = priv->format_context->bit_rate;
		nfo->bit_depth   = 0;
		nfo->meta_data   = NULL;

		GPtrArray* tags = g_ptr_array_new_full(32, g_free);

		AVDictionaryEntry *tag = NULL;
		// Tags in container
		while ((tag = av_dict_get(priv->format_context->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
			dbg(2, "FTAG: %s=%s", tag->key, tag->value);
			g_ptr_array_add(tags, g_utf8_strdown(tag->key, -1));
			g_ptr_array_add(tags, g_strdup(tag->value));
		}
		// Tags in stream
		tag = NULL;
		AVStream *stream = priv->format_context->streams[priv->audio_stream];
		while ((tag = av_dict_get(stream->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
			dbg(2, "STAG: %s=%s", tag->key, tag->value);
			g_ptr_array_add(tags, g_utf8_strdown(tag->key, -1));
			g_ptr_array_add(tags, g_strdup(tag->value));
		}

		while ((tag = av_dict_get(stream->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
			g_ptr_array_add(tags, g_utf8_strdown(tag->key, -1));
			g_ptr_array_add(tags, g_strdup(tag->value));
		}

		if(tags->len){
			// sort tags
			char* order[] = {"artist", "title", "album", "track", "date"};
			int p = 0;
			int i; for(i=0;i<G_N_ELEMENTS(order);i++) if(ad_metadata_array_set_tag_postion(tags, order[i], p)) p++;

			nfo->meta_data = tags;
		}else
			g_ptr_array_free(tags, true);
	}
	return 0;
}


void*
ad_open_ffmpeg(const char* fn, struct adinfo* nfo)
{
  ffmpeg_audio_decoder *priv = (ffmpeg_audio_decoder*) calloc(1, sizeof(ffmpeg_audio_decoder));
  
  priv->tmp_buf.start = NULL;
  priv->tmp_buf.len = 0;
  priv->decoder_clock = priv->output_clock = priv->seek_frame = 0;
  priv->packet.size=0; priv->packet.data=NULL;

  if (avformat_open_input(&priv->format_context, fn, NULL, NULL) < 0) {
    dbg(1, "ffmpeg is unable to open file '%s'.", fn);
    free(priv); return(NULL);
  }

  if (avformat_find_stream_info(priv->format_context, NULL) < 0) {
    avformat_close_input(&priv->format_context);
    dbg(1, "av_find_stream_info failed");
    free(priv); return(NULL);
  }

  priv->audio_stream = -1;
  int i;
  for (i=0; i<priv->format_context->nb_streams; i++) {
    if (priv->format_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
      priv->audio_stream = i;
      break;
    }
  }
  if (priv->audio_stream == -1) {
    dbg(1, "No Audio Stream found in file");
    avformat_close_input(&priv->format_context);
    free(priv); return(NULL);
  }

  priv->codec_context = priv->format_context->streams[priv->audio_stream]->codec;
  priv->codec         = avcodec_find_decoder(priv->codec_context->codec_id);

  if (priv->codec == NULL) {
    avformat_close_input(&priv->format_context);
    dbg(1, "Codec not supported by ffmpeg");
    free(priv); return(NULL);
  }
  if (avcodec_open2(priv->codec_context, priv->codec, NULL) < 0) {
    dbg(1, "avcodec_open failed" );
    free(priv); return(NULL);
  }

  dbg(2, "ffmpeg - audio tics: %i/%i [sec]", priv->format_context->streams[priv->audio_stream]->time_base.num, priv->format_context->streams[priv->audio_stream]->time_base.den);

  int64_t len = priv->format_context->duration - priv->format_context->start_time;

  priv->format_context->flags |= AVFMT_FLAG_GENPTS;
  priv->format_context->flags |= AVFMT_FLAG_IGNIDX;

  priv->samplerate = priv->codec_context->sample_rate;
  priv->channels   = priv->codec_context->channels;
  priv->length     = (int64_t)(len * priv->samplerate / AV_TIME_BASE);

  if (ad_info_ffmpeg((void*)priv, nfo)) {
    dbg(1, "invalid file info (sample-rate==0)");
    free(priv); return(NULL);
  }

	dbg(2, "ffmpeg - %s", fn);
	if (nfo) dbg(2, "ffmpeg - sr:%i c:%i d:%"PRIi64" f:%"PRIi64, nfo->sample_rate, nfo->channels, nfo->length, nfo->frames);

	switch(priv->codec_context->sample_fmt){
		case AV_SAMPLE_FMT_S16P:
			priv->read_planar = ad_ff_read_short_planar_to_planar;
			priv->read = ad_ff_read_short_planar_to_interleaved;
			break;
		case AV_SAMPLE_FMT_FLTP:
			priv->read = ad_ff_read_float_planar_to_interleaved;
			break;
		case AV_SAMPLE_FMT_S32P:
			gwarn("planar (non-interleaved) unhandled!");
		//case AV_SAMPLE_FMT_FLT:
		default:
			priv->read = ad_read_ffmpeg_default_interleaved;
			break;
	}

	return (void*) priv;
}


int
ad_close_ffmpeg(void* sf)
{
	ffmpeg_audio_decoder *priv = (ffmpeg_audio_decoder*) sf;
	if (!priv) return -1;
	avcodec_close(priv->codec_context);
	avformat_close_input(&priv->format_context);
	free(priv);
	return 0;
}


/*
 *  Input and output is both interleaved
 */
void
int16_to_float(float* out, int16_t* in, int n_channels, int n_frames, int out_offset)
{
	int f, c;
	for (f=0;f<n_frames;f++) {
		for (c=0;c<n_channels;c++) {
			out[(f+out_offset)*n_channels+c] = (float) in[f*n_channels+c] / 32768.0;
		}
	}
}

#define SHORT_TO_FLOAT(A) (((float)A) / 32768.0)
#define FLOAT_TO_SHORT(A) (A * (1<<15));

#if 0
static void
interleave(float* out, size_t len, Buf16* buf, int n_channels)
{
	int i,c; for(i=0; i<len/n_channels;i++){
		for(c=0;c<n_channels;c++){
			out[i * n_channels + c] = SHORT_TO_FLOAT(buf->buf[c][i]);
		}
	}
}
#endif


/*
 *  Implements ad_plugin.read
 *  Output is interleaved
 *  len is the size of the out array (n_frames * n_channels).
 */
ssize_t
ad_read_ffmpeg(void* sf, float* out, size_t len)
{
	ffmpeg_audio_decoder* priv = (ffmpeg_audio_decoder*)sf;
	if (!priv) return -1;

	g_return_val_if_fail(priv->read, -1);
	return priv->read(sf, out, len);
}


ssize_t
ad_read_ffmpeg_default_interleaved(ffmpeg_audio_decoder* priv, float* out, size_t len)
{
	//ffmpeg_audio_decoder* priv = (ffmpeg_audio_decoder*)sf;
	if (!priv) return -1;

  size_t frames = len / priv->channels;

  int written = 0;
  int ret = 0;
  while (ret >= 0 && written < frames) {
    dbg(3, "loop: %i/%i (bl:%lu)", written, frames, priv->tmp_buf.len);
    if (priv->seek_frame == 0 && priv->tmp_buf.len > 0) {
      int s = MIN(priv->tmp_buf.len / priv->channels, frames - written);
      int16_to_float(out, priv->tmp_buf.start, priv->channels, s, written);
      written += s;
      priv->output_clock += s;
      s = s * priv->channels;
      priv->tmp_buf.start += s;
      priv->tmp_buf.len -= s;
      ret = 0;
    } else {
      priv->tmp_buf.start = priv->tmp_buf.buf;
      priv->tmp_buf.len = 0;

      if (!priv->pkt_ptr || priv->pkt_len < 1) {
        if (priv->packet.data) av_free_packet(&priv->packet);
        ret = av_read_frame(priv->format_context, &priv->packet);
        if (ret < 0) { dbg(2, "reached end of file."); break; }
        priv->pkt_len = priv->packet.size;
        priv->pkt_ptr = priv->packet.data;
      }

      if (priv->packet.stream_index != priv->audio_stream) {
        priv->pkt_ptr = NULL;
        continue;
      }

      int data_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
      AVFrame avf; // TODO statically allocate
      memset(&avf, 0, sizeof(AVFrame)); // not sure if that is needed
      int got_frame = 0;
      ret = avcodec_decode_audio4(priv->codec_context, &avf, &got_frame, &priv->packet);
      data_size = avf.linesize[0];
      memcpy(priv->tmp_buf.buf, avf.data[0], avf.linesize[0] * sizeof(uint8_t));

      if (ret < 0 || ret > priv->pkt_len) {
#if 0
        dbg(0, "audio decode error");
        return -1;
#endif
        priv->pkt_len = 0;
        ret = 0;
        continue;
      }

      priv->pkt_len -= ret; priv->pkt_ptr += ret;

      /* sample exact alignment  */
      if (priv->packet.pts != AV_NOPTS_VALUE) {
        priv->decoder_clock = priv->samplerate * av_q2d(priv->format_context->streams[priv->audio_stream]->time_base) * priv->packet.pts;
      } else {
        dbg(0, "!!! NO PTS timestamp in file");
        priv->decoder_clock += (data_size>>1) / priv->channels;
      }

      if (data_size > 0) {
        priv->tmp_buf.len += (data_size>>1); // 2 bytes per sample
      }

      /* align buffer after seek. */
      if (priv->seek_frame > 0) {
        const int diff = priv->output_clock-priv->decoder_clock;
        if (diff < 0) { 
          /* seek ended up past the wanted sample */
          gwarn("audio seek failed.");
          return -1;
        } else if (priv->tmp_buf.len < (diff*priv->channels)) {
          /* wanted sample not in current buffer - keep going */
          dbg(2, " !!! seeked sample was not in decoded buffer. frames-to-go: %li", diff);
          priv->tmp_buf.len = 0;
        } else if (diff!=0 && data_size > 0) {
          /* wanted sample is in current buffer but not at the beginnning */
          dbg(2, " !!! sync buffer to seek. (diff:%i)", diff);
          priv->tmp_buf.start += diff * priv->codec_context->channels;
          priv->tmp_buf.len   -= diff * priv->codec_context->channels;
#if 1
          memmove(priv->tmp_buf.buf, priv->tmp_buf.start, priv->tmp_buf.len);
          priv->tmp_buf.start = priv->tmp_buf.buf;
#endif
          priv->seek_frame = 0;
          priv->decoder_clock += diff;
        } else if (data_size > 0) {
          dbg(2, "Audio exact sync-seek (%"PRIi64" == %"PRIi64")", priv->decoder_clock, priv->seek_frame);
          priv->seek_frame = 0;
        } else {
          dbg(0, "Error: no audio data in packet");
        }
      }
      //dbg(0, "PTS: decoder:%"PRIi64". - want: %"PRIi64, priv->decoder_clock, priv->output_clock);
      //dbg(0, "CLK: frame:  %"PRIi64"  T:%.3fs",priv->decoder_clock, (float) priv->decoder_clock/priv->samplerate);
    }
  }
  if (written != frames) {
    dbg(2, "short-read");
  }
  return written * priv->channels;
}


static ssize_t
ad_ff_read_short_planar_to_planar(ffmpeg_audio_decoder* f, Buf16* buf)
{
	int64_t n_fr_done = 0;

	int data_size = av_get_bytes_per_sample(f->codec_context->sample_fmt);
	g_return_val_if_fail(data_size == 2, 0);

	bool have_frame = false;
	if(f->frame.nb_samples && f->frame_iter < f->frame.nb_samples){
		have_frame = true;
	}

	while(have_frame || !av_read_frame(f->format_context, &f->packet)){
		have_frame = false;
		if(f->packet.stream_index == f->audio_stream){

			int got_frame = false;
			if(f->frame_iter && f->frame_iter < f->frame.nb_samples){
				got_frame = true;
			}else{
				memset(&f->frame, 0, sizeof(AVFrame));
				av_frame_unref(&f->frame);
				f->frame_iter = 0;

				if(avcodec_decode_audio4(f->codec_context, &f->frame, &got_frame, &f->packet) < 0){
					dbg(0, "Error decoding audio");
				}
			}
			if(got_frame){
				int size = av_samples_get_buffer_size (NULL, f->codec_context->channels, f->frame.nb_samples, f->codec_context->sample_fmt, 1);
				if (size < 0)  {
					dbg(0, "av_samples_get_buffer_size invalid value");
				}

				int64_t fr = f->frame.best_effort_timestamp * f->samplerate / f->format_context->streams[f->audio_stream]->time_base.den + f->frame_iter;
				int ch;
				int i; for(i=f->frame_iter; i<f->frame.nb_samples; i++){
					if(n_fr_done >= buf->size) break;
					if(fr >= f->seek_frame){
						for(ch=0; ch<MIN(2, f->codec_context->channels); ch++){
							memcpy(buf->buf[ch] + n_fr_done, f->frame.data[ch] + data_size * i, data_size);
						}
						n_fr_done++;
						f->frame_iter++;
					}
				}
				f->output_clock = fr + n_fr_done;

				if(n_fr_done >= buf->size) goto stop;
			}
		}
		av_free_packet(&f->packet);
		continue;

		stop:
			av_free_packet(&f->packet);
			break;
	}

	return n_fr_done;
}


static ssize_t
ad_ff_read_short_planar_to_interleaved(ffmpeg_audio_decoder* f, float* out, size_t len)
{
	int n_frames = len / f->channels;
	int64_t n_fr_done = 0;

	int data_size = av_get_bytes_per_sample(f->codec_context->sample_fmt);
	g_return_val_if_fail(data_size == 2, 0);

	bool have_frame = false;
	if(f->frame.nb_samples && f->frame_iter < f->frame.nb_samples){
		have_frame = true;
	}

	while(have_frame || !av_read_frame(f->format_context, &f->packet)){
		have_frame = false;
		if(f->packet.stream_index == f->audio_stream){

			int got_frame = false;
			if(f->frame_iter && f->frame_iter < f->frame.nb_samples){
				got_frame = true;
			}else{
				memset(&f->frame, 0, sizeof(AVFrame));
				av_frame_unref(&f->frame);
				f->frame_iter = 0;

				if(avcodec_decode_audio4(f->codec_context, &f->frame, &got_frame, &f->packet) < 0){
					dbg(0, "Error decoding audio");
				}
			}
			if(got_frame){
				int size = av_samples_get_buffer_size (NULL, f->codec_context->channels, f->frame.nb_samples, f->codec_context->sample_fmt, 1);
				if (size < 0)  {
					dbg(0, "av_samples_get_buffer_size invalid value");
				}

				int64_t fr = f->frame.best_effort_timestamp * f->samplerate / f->format_context->streams[f->audio_stream]->time_base.den + f->frame_iter;
				int ch;
				int i; for(i=f->frame_iter; i<f->frame.nb_samples && (n_fr_done < n_frames); i++){
					if(fr >= f->seek_frame){
						for(ch=0; ch<MIN(2, f->codec_context->channels); ch++){
							out[f->codec_context->channels * n_fr_done + ch] = SHORT_TO_FLOAT(*(((int16_t*)f->frame.data[ch]) + i));
						}
						n_fr_done++;
						f->frame_iter++;
					}
				}
				f->output_clock = fr + n_fr_done;

				if(n_fr_done >= n_frames) goto stop;
			}
		}
		av_free_packet(&f->packet);
		continue;

		stop:
			av_free_packet(&f->packet);
			break;
	}

	return n_fr_done * f->channels;
}


/*
 *
 */
static ssize_t
ad_ff_read_short_interleaved_to_interleaved(ffmpeg_audio_decoder* f, float* out, size_t len)
{
	int n_frames = len / f->channels;
	int64_t n_fr_done = 0;

	int data_size = av_get_bytes_per_sample(f->codec_context->sample_fmt);
	g_return_val_if_fail(data_size == 2, 0);

	bool have_frame = false;
	if(f->frame.nb_samples && f->frame_iter < f->frame.nb_samples){
		have_frame = true;
	}

	while(have_frame || !av_read_frame(f->format_context, &f->packet)){
		have_frame = false;
		if(f->packet.stream_index == f->audio_stream){

			int got_frame = false;
			if(f->frame_iter && f->frame_iter < f->frame.nb_samples){
				got_frame = true;
			}else{
				memset(&f->frame, 0, sizeof(AVFrame));
				av_frame_unref(&f->frame);
				f->frame_iter = 0;

				if(avcodec_decode_audio4(f->codec_context, &f->frame, &got_frame, &f->packet) < 0){
					dbg(0, "Error decoding audio");
				}
			}
			if(got_frame){
				int size = av_samples_get_buffer_size (NULL, f->codec_context->channels, f->frame.nb_samples, f->codec_context->sample_fmt, 1);
				if (size < 0)  {
					dbg(0, "av_samples_get_buffer_size invalid value");
				}

				int64_t fr = f->frame.best_effort_timestamp * f->samplerate / f->format_context->streams[f->audio_stream]->time_base.den + f->frame_iter;
				int ch;
				int i; for(i=f->frame_iter; i<f->frame.nb_samples; i++){
					if(n_fr_done >= n_frames) break;
					if(fr >= f->seek_frame){
						for(ch=0; ch<MIN(2, f->codec_context->channels); ch++){
							uint8_t* src = f->frame.data[0] + f->codec_context->channels * data_size * i + data_size * ch;
							int16_t a;
							memcpy(
								&a,
								&src,
								sizeof(int16_t)
							);
							out[n_fr_done * f->channels + ch] = FLOAT_TO_SHORT(a);
						}
						n_fr_done++;
						f->frame_iter++;
					}
				}
				f->output_clock = fr + n_fr_done;

				if(n_fr_done >= n_frames) goto stop;
			}
		}
		av_free_packet(&f->packet);
		continue;

		stop:
			av_free_packet(&f->packet);
			break;
	}

	return n_fr_done;
}


static ssize_t
ad_ff_read_float_planar_to_interleaved(ffmpeg_audio_decoder* f, float* out, size_t len)
{
	int n_frames = len / f->channels;
	int64_t n_fr_done = 0;

	int data_size = av_get_bytes_per_sample(f->codec_context->sample_fmt);
	g_return_val_if_fail(data_size == 4, 0);

	bool have_frame = false;
	if(f->frame.nb_samples && f->frame_iter < f->frame.nb_samples){
		have_frame = true;
	}

	while(have_frame || !av_read_frame(f->format_context, &f->packet)){
		have_frame = false;
		if(f->packet.stream_index == f->audio_stream){

			int got_frame = false;
			if(f->frame_iter && f->frame_iter < f->frame.nb_samples){
				got_frame = true;
			}else{
				memset(&f->frame, 0, sizeof(AVFrame));
				av_frame_unref(&f->frame);
				f->frame_iter = 0;

				if(avcodec_decode_audio4(f->codec_context, &f->frame, &got_frame, &f->packet) < 0){
					dbg(0, "Error decoding audio");
				}
			}
			if(got_frame){
				int size = av_samples_get_buffer_size (NULL, f->codec_context->channels, f->frame.nb_samples, f->codec_context->sample_fmt, 1);
				if (size < 0)  {
					dbg(0, "av_samples_get_buffer_size invalid value");
				}

				int64_t fr = f->frame.best_effort_timestamp * f->samplerate / f->format_context->streams[f->audio_stream]->time_base.den + f->frame_iter;
				int ch;
				int i; for(i=f->frame_iter; i<f->frame.nb_samples && (n_fr_done < n_frames); i++){
					if(fr >= f->seek_frame){
						for(ch=0; ch<MIN(2, f->codec_context->channels); ch++){
							out[f->codec_context->channels * n_fr_done + ch] = *(((float*)f->frame.data[ch]) + i);
						}
						n_fr_done++;
						f->frame_iter++;
					}
				}
				f->output_clock = fr + n_fr_done;

				if(n_fr_done >= n_frames) goto stop;
			}
		}
		av_free_packet(&f->packet);
		continue;

		stop:
			av_free_packet(&f->packet);
			break;
	}

	return n_fr_done;
}


int64_t
ad_seek_ffmpeg(void* sf, int64_t pos)
{
	ffmpeg_audio_decoder* f = (ffmpeg_audio_decoder*) sf;
	if (!sf) return -1;
	if (pos == f->output_clock) return pos;

	/* flush internal buffer */
	f->tmp_buf.len = 0;
	f->seek_frame = pos;
	f->output_clock = pos;
	f->pkt_len = 0; f->pkt_ptr = NULL;
	f->decoder_clock = 0;

	memset(&f->frame, 0, sizeof(AVFrame));
	av_frame_unref(&f->frame);
	f->frame_iter = 0;

#if 0
	/* TODO seek at least 1 packet before target.
	 * for mpeg compressed files, the
	 * output may depend on past frames! */
	if (pos > 8192) pos -= 8192;
	else pos = 0;
#endif

	const int64_t timestamp = pos / av_q2d(f->format_context->streams[f->audio_stream]->time_base) / f->samplerate;
	dbg(2, "seek frame:%"PRIi64" - idx:%"PRIi64, pos, timestamp);

	av_seek_frame(f->format_context, f->audio_stream, timestamp, AVSEEK_FLAG_ANY | AVSEEK_FLAG_BACKWARD);
	avcodec_flush_buffers(f->codec_context);

	return pos;
}

int
ad_eval_ffmpeg(const char* f)
{
  char* ext = strrchr(f, '.');
  if (!ext) return 10;
  // libavformat.. guess_format.. 
  return 40;
}
#endif

const static ad_plugin ad_ffmpeg = {
#ifdef HAVE_FFMPEG
  &ad_eval_ffmpeg,
  &ad_open_ffmpeg,
  &ad_close_ffmpeg,
  &ad_info_ffmpeg,
  &ad_seek_ffmpeg,
  &ad_read_ffmpeg
#else
  &ad_eval_null,
  &ad_open_null,
  &ad_close_null,
  &ad_info_null,
  &ad_seek_null,
  &ad_read_null
#endif
};

/* dlopen handler */
const ad_plugin * get_ffmpeg() {
#ifdef HAVE_FFMPEG
  static int ffinit = 0;
  if (!ffinit) {
    ffinit=1;
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(53, 5, 0)
    avcodec_init();
#endif
    av_register_all();
    avcodec_register_all();
    av_log_set_level(_debug_ > 1 ? AV_LOG_VERBOSE : AV_LOG_QUIET);
  }
#endif
  return &ad_ffmpeg;
}
