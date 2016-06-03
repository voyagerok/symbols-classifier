#ifndef IMGPROC_H
#define IMGPROC_H

#define CHANNEL_DEPTH 8
#define N_CHANNELS_RGB 3
#define N_CHANNELS_RGBA 4
#define N_CHANNELS_GRAY 1
#define GREATER 10
#define LESSER 20
#define BIN_THERSHOLD 180

#define P_WIDTH(image) (gdk_pixbuf_get_width(image))
#define P_HEIGHT(image) (gdk_pixbuf_get_height(image))
#define P_STRIDE(image) (gdk_pixbuf_get_rowstride(image))
#define P_PIXELS(image) (gdk_pixbuf_get_pixels(image))
#define P_PIXELS_READ(image) (gdk_pixbuf_read_pixels(image))
#define P_N_CHANNELS(image) (gdk_pixbuf_get_n_channels(image))

#include <gtk/gtk.h>

GdkPixbuf*
noise (GdkPixbuf *image, int w_step, int h_step, double power);
GdkPixbuf*
rotate (GdkPixbuf *image, float angle);
GdkPixbuf*
resize (const GdkPixbuf *image, int width_step, int height_step, int mode, int need_binarization);
GdkPixbuf*
get_modified_image (GdkPixbuf *image, int use_effects_probability);
double*
get_vector_from_pixbuf (const GdkPixbuf *image);
void
to_binary_image (GdkPixbuf *image);

GdkPixbuf*
translation (GdkPixbuf *input, int shiftx, int shifty);

#endif // IMGPROC_H
