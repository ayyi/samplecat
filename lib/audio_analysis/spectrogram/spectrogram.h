#ifndef __SPECTROGRAM_H_
#define __SPECTROGRAM_H_
#include <gtk/gtk.h>
void get_spectrogram             (const char* path, gpointer callback, gpointer);
void get_spectrogram_with_target (const char* path, gpointer callback, void* target, gpointer);
void render_spectrogram          (const char* path, gpointer callback, gpointer);
void cancel_spectrogram          (const char* path);
#endif
