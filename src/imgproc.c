#include "imgproc.h"
#include <stdlib.h>
#include <opencv2/core/core_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#include "constants.h"
#include "probability.h"

#define STEP_RATIO 40
#define NOISE_POW 0.03
#define ROT_PROB 0.5
#define NOISE_PROB 0.5
#define TRANS_PROB 0.5

static IplImage *
pixbuf2ipl(const GdkPixbuf *image)
{
  IplImage *res_image;
  int width, height;
  int depth, n_channels;
  int stride, res_img_stride;
  guchar *image_data;
  guchar *res_image_data;
  gboolean has_alpha;

  width = gdk_pixbuf_get_width(image);
  height = gdk_pixbuf_get_height(image);
  depth = gdk_pixbuf_get_bits_per_sample(image);
  n_channels = gdk_pixbuf_get_n_channels(image);
  stride = gdk_pixbuf_get_rowstride(image);
  has_alpha = gdk_pixbuf_get_has_alpha(image);

  g_assert(depth == CHANNEL_DEPTH);

  image_data = gdk_pixbuf_get_pixels(image);
  res_image = cvCreateImage(cvSize(width, height),
                            depth, n_channels);
  res_image_data = (guchar*)res_image->imageData;
  res_img_stride = res_image->widthStep;

  for(int i = 0; i < height; ++i)
    for(int j = 0; j < width; ++j)
      {
        int index = i * res_img_stride + j * n_channels;
        res_image_data[index] = image_data[index + 2];
        res_image_data[index + 1] = image_data[index + 1];
        res_image_data[index + 2] = image_data[index];
      }

  return res_image;
}

static GdkPixbuf *
ipl2pixbuf(const IplImage *image)
{
  uchar *imageData;
  guchar *pixbufData;
  int widthStep, n_channels, res_stride;
  int width, height, depth, res_n_channels;
  int data_order;
  GdkPixbuf *res_image;
  long ipl_depth;
  CvSize roi;

  cvGetRawData(image, &imageData, &widthStep, &roi);
  width = roi.width;
  height = roi.height;
  n_channels = image->nChannels;
  data_order = image->dataOrder;

  g_assert(data_order == IPL_DATA_ORDER_PIXEL);
  g_assert(n_channels == N_CHANNELS_RGB  ||
           n_channels == N_CHANNELS_RGBA ||
           n_channels == N_CHANNELS_GRAY);

  switch(ipl_depth = image->depth)
    {
    case IPL_DEPTH_8U:
      depth = 8;
      break;
    default:
      depth = 0;
      break;
    }
  g_assert(depth == CHANNEL_DEPTH);

  res_image = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE,
                             depth, width, height);
  pixbufData = gdk_pixbuf_get_pixels(res_image);
  res_stride = gdk_pixbuf_get_rowstride(res_image);
  res_n_channels = N_CHANNELS_RGB;

  for(int i = 0; i < height; ++i)
    for(int j = 0; j < width; ++j)
      {
        int index = i * widthStep + j * n_channels;
        int res_index = i * res_stride + j * res_n_channels;
        if(n_channels == N_CHANNELS_GRAY)
          pixbufData[res_index] = pixbufData[res_index + 1] =
              pixbufData[res_index + 2] = imageData[index];
        else
          {
            pixbufData[res_index] = imageData[index + 2];
            pixbufData[res_index + 1] = imageData[index + 1];
            pixbufData[res_index + 2] = imageData[index];
          }
      }
  return res_image;
}

GdkPixbuf*
noise (GdkPixbuf *image, int w_step, int h_step, double power)
{
  int width, height;
  const guchar *pixels;
  guchar *res_pix;
  int channels, stride, depth;
  int res_stride, rand_val;
  int s_index, d_index;
  GdkPixbuf *res;
  gboolean has_alpha;

  width = gdk_pixbuf_get_width(image);
  height = gdk_pixbuf_get_height(image);
  pixels = gdk_pixbuf_read_pixels(image);
  channels = gdk_pixbuf_get_n_channels(image);
  stride = gdk_pixbuf_get_rowstride(image);
  has_alpha = gdk_pixbuf_get_has_alpha(image);
  depth = gdk_pixbuf_get_bits_per_sample(image);

  res = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
                       has_alpha, depth,
                       width, height);
  res_pix = gdk_pixbuf_get_pixels(res);
  res_stride = gdk_pixbuf_get_rowstride(res);

//  srand(time(NULL));

//  int width_step = width / STEP_RATIO;
//  int height_step = height / STEP_RATIO;
  int width_step = w_step;
  int height_step = h_step;

  for(int i = 0; i < height; i += height_step)
    for(int j = 0; j < width; j += width_step)
      {

        //rand_val = rand() % 20;
        rand_val = flip_coin (power);

        for(int k = 0; k < height && k < height_step; ++k)
          for(int l = 0; l < width && l < width_step; ++l)
            {
              s_index = (k + i) * stride + (l + j) * channels;
              d_index = (k + i) * res_stride + (l + j) * channels;

              if(rand_val)
                {
                  res_pix[d_index] = (1 << depth) - pixels[s_index] - 1;
                  res_pix[d_index + 1] = (1 << depth) - pixels[s_index + 2] - 1;
                  res_pix[d_index + 2] = (1 << depth) - pixels[s_index + 2] - 1;
                }
              else
                {
                  res_pix[d_index] = pixels[s_index];
                  res_pix[d_index + 1] = pixels[s_index + 1];
                  res_pix[d_index + 2] = pixels[s_index + 2];
                }
            }


      }
  return res;
}

GdkPixbuf*
rotate (GdkPixbuf *image, float angle)
{
  IplImage *cvimage;
  GdkPixbuf *rotated;
  CvMat *matrix;
  int width, height;
  CvPoint2D32f center;

  width = gdk_pixbuf_get_width(image);
  height = gdk_pixbuf_get_height(image);
  cvimage = pixbuf2ipl(image);
  center.x = (float)width / 2;
  center.y = (float)height / 2;
  matrix = cvCreateMat(2, 3, CV_32FC1);
  cv2DRotationMatrix(center, angle, 1, matrix);
  cvWarpAffine(cvimage, cvimage, matrix, CV_INTER_CUBIC+CV_WARP_FILL_OUTLIERS, cvScalarAll(255));
  rotated = ipl2pixbuf(cvimage);

  cvReleaseImage(&cvimage);
  cvReleaseMat(&matrix);

  return rotated;
}

GdkPixbuf*
resize (const GdkPixbuf *image, int width_step, int height_step, int mode, int need_binarization)
{
  GdkPixbuf *result;
  int width, height, step;
  int new_width, new_height, new_step;
  const guchar *pixels;
  guchar *res_pixels;
  int index, new_index;

  g_assert (mode == GREATER || mode == LESSER);

  width = gdk_pixbuf_get_width(image);
  height = gdk_pixbuf_get_height(image);
  step = gdk_pixbuf_get_rowstride(image);
  pixels = gdk_pixbuf_read_pixels(image);

  if (mode == LESSER)
    {
      new_width = width / width_step;
      new_height = height / height_step;
    }
  else
    {
      new_width = width * width_step;
      new_height = height * height_step;
    }
  result = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, CHANNEL_DEPTH, new_width, new_height);
  new_step = gdk_pixbuf_get_rowstride(result);
  res_pixels = P_PIXELS(result);

  for (int i = 0; i < new_height; ++i)
    for (int j = 0; j < new_width; ++j)
      {
        new_index = i * new_step + j * N_CHANNELS_RGB;
        if (mode == LESSER)
          index = (i * height_step) * step + (j * width_step) * N_CHANNELS_RGB;
        else
          index = (i / height_step) * step + (j / width_step) * N_CHANNELS_RGB;
        if (!need_binarization)
          {
            res_pixels[new_index] = pixels[index];
            res_pixels[new_index + 1] = pixels[index + 1];
            res_pixels[new_index + 2] = pixels[index + 2];
          }
        else
          {
            res_pixels[new_index] = pixels[index] > BIN_THERSHOLD ? 255 : 0;
            res_pixels[new_index + 1] = pixels[index + 1] > BIN_THERSHOLD ? 255 : 0;
            res_pixels[new_index + 2] = pixels[index + 2] > BIN_THERSHOLD ? 255 : 0;
          }
      }
  return result;
}

GdkPixbuf*
get_modified_image (GdkPixbuf *image, int use_effects_probability)
{
  float rot_angle, shiftx, shifty;
  GdkPixbuf *result, *rotated, *translated;


  if (flip_coin (TRANS_PROB) || !use_effects_probability)
    {
      shiftx = rand() % 20 - 10;
      shifty = rand() % 20 - 10;
      translated = translation (image, shiftx, shifty);
    }
  else
    translated = gdk_pixbuf_copy(image);

  if (flip_coin (ROT_PROB) || !use_effects_probability)
    {
      rot_angle = rand() % 60 - 30;
      rotated = rotate(translated, rot_angle);
      to_binary_image (rotated);
    }
  else
    rotated = gdk_pixbuf_copy(translated);

  if (flip_coin (NOISE_PROB) || !use_effects_probability)
    result = noise (rotated, 1, 1, NOISE_POW);
  else
    result = gdk_pixbuf_copy(rotated);

  g_object_unref(translated);
  g_object_unref(rotated);

  return result;
}

double*
get_vector_from_pixbuf (const GdkPixbuf *image)
{
  double *vector;
  int width, height, stride, n_channels;
  const guchar *pixels;
  int img_index, vec_index;

  width = P_WIDTH(image);
  height = P_HEIGHT(image);
  stride = P_STRIDE(image);
  pixels = P_PIXELS_READ(image);
  n_channels = P_N_CHANNELS(image);

  g_assert (width * height == VEC_SIZE);

  vector = (double*)malloc(sizeof(double) * width * height);

  for (int i = 0; i < height; ++i)
      for (int j = 0; j < width; ++j)
        {
          img_index = i * stride + j * n_channels;
          vec_index = i * width + j;
          g_assert (pixels[img_index] == 0 || pixels[img_index] == 255);
          vector[vec_index] = pixels[img_index] == 0;
        }

  return vector;
}

void
to_binary_image (GdkPixbuf *image)
{
  int width, height, stride, n_channels;
  guchar *pixels;
  int index;

  width = P_WIDTH(image);
  height = P_HEIGHT(image);
  stride = P_STRIDE(image);
  pixels = P_PIXELS(image);
  n_channels = P_N_CHANNELS(image);

  for (int i = 0; i < height; ++i)
      for (int j = 0; j < width; ++j)
        {
          index = i * stride + j * n_channels;
          pixels[index] = pixels[index] < BIN_THERSHOLD ? 0 : 255;
          pixels[index + 1] = pixels[index + 1] < BIN_THERSHOLD ? 0 : 255;
          pixels[index + 2] = pixels[index + 2] < BIN_THERSHOLD ? 0 : 255;
        }
}

GdkPixbuf*
translation (GdkPixbuf *input, int shiftx, int shifty)
{
  int width, height, n_channels;
  guchar *i_pixels, *o_pixels;
  int i_index, o_index;
  int i_stride, o_stride;
  GdkPixbuf *output;

  width = P_WIDTH(input);
  height = P_HEIGHT(input);
  i_stride = P_STRIDE(input);
  i_pixels = P_PIXELS(input);
  n_channels = P_N_CHANNELS(input);

  output = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, CHANNEL_DEPTH, width, height);
  o_pixels = P_PIXELS(output);
  o_stride = P_STRIDE (output);

  for (int i = 0; i < height; ++i)
      for (int j = 0; j < width; ++j)
        {
          int new_i, new_j;

          i_index = i * i_stride + j * n_channels;

          new_i = (new_i = (i + shifty)) < 0 ? height + new_i : (new_i >= height) ?
                                               new_i - height : new_i;
          new_j = (new_j = (j + shiftx)) < 0 ? width + new_j : (new_j >= width) ?
                                                 new_j - width : new_j;
          o_index = new_i * o_stride + new_j * n_channels;

          o_pixels[o_index] = i_pixels[i_index];
          o_pixels[o_index + 1] = i_pixels[i_index + 1];
          o_pixels[o_index + 2] = i_pixels[i_index + 2];
        }

  return output;
}
