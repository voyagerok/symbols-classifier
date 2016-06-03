#include "kozinets_learning.h"
#include "vector_ops.h"
#include "constants.h"
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include "imgproc.h"
#include <math.h>

#define MAX_ITER_COUNT 100

static void
pack_train_samples_for_2_classes (classes_container *container, double **packed, int index1, int index2)
{
  double inverted_sample[VEC_SIZE];

  for (int i = 0; i < TRAIN_SAMPLES_COUNT; ++i)
      memcpy (packed[i], container->classes[index1].train_samples[i], sizeof(double) * VEC_SIZE);

  for (int i = 0; i < TRAIN_SAMPLES_COUNT; ++i)
    {
      INVERT_VECTOR (container->classes[index2].train_samples[i], inverted_sample, VEC_SIZE);
      memcpy (packed[i + TRAIN_SAMPLES_COUNT], inverted_sample, sizeof(double) * VEC_SIZE);
    }
}

static double
mul_2_values (double v1, double v2) { return v1 * v2; }

static gboolean
check_linear_separability (double *vector, double **samples, int samples_count)
{
  int min_index;
  double min_scalar, cur_scalar;
  double *shift, *cur_vector, *nex_vector;
  gboolean are_separable;

  nex_vector = (double*)malloc(sizeof(double) * VEC_SIZE);
  shift = (double*)malloc(sizeof(double) * VEC_SIZE);
  cur_vector = (double*)malloc(sizeof(double) * VEC_SIZE);

  memcpy (cur_vector, vector, sizeof(double) * VEC_SIZE);

  are_separable = FALSE;

  for (int j = 0; j < MAX_ITER_COUNT; ++j)
    {
      min_index = 0;
      min_scalar = DBL_MAX;
      for (int i = 0; i < samples_count; ++i)
        {
          cur_scalar = scalar_multiplication (cur_vector, samples[i], VEC_SIZE);
          if (cur_scalar < min_scalar)
            {
              min_scalar = cur_scalar;
              min_index = i;
            }
        }

      if (min_scalar > 0)
        {
          are_separable = TRUE;
          goto ending;
        }

      VECTOR_FOREACH (samples[min_index], shift, VEC_SIZE, mul_2_values, CONFIG_STEP);
      SUM_2_VECTORS (cur_vector, shift, nex_vector, VEC_SIZE);

      memcpy (cur_vector, nex_vector, sizeof(double) * VEC_SIZE);
    }

ending:
  free (shift);
  free (nex_vector);
  free (cur_vector);
  return are_separable;
}

gboolean
check_linear_separability_for_2_classes (classes_container *container)
{
  int n_of_classes, packed_size;
  double *rand_vector;
  double **packed_samples;
  gboolean res;

  if ((n_of_classes = container->n_of_classes) < 2)
    return FALSE;

  rand_vector = (double*)malloc(sizeof(double) * VEC_SIZE);
  packed_size = TRAIN_SAMPLES_COUNT * 2;
  packed_samples = (double**)malloc(sizeof(double*) * packed_size);
  for (int i = 0; i < packed_size; ++i)
    packed_samples[i] = (double*)malloc(sizeof(double) * VEC_SIZE);

  GENERATE_RANDOM_VECTOR (rand_vector, VEC_SIZE);
  pack_train_samples_for_2_classes (container, packed_samples, 0, 1);
  res = check_linear_separability (rand_vector, packed_samples, TRAIN_SAMPLES_COUNT * 2);

  for (int i = 0; i < packed_size; ++i)
    free (packed_samples[i]);
  free (packed_samples);
  free (rand_vector);

  return res;
}

static void
find_best_dividing_with_kozinets (double *vector, double *best, double **shell, int shell_size)
{
  double *zk;
  double temp[VEC_SIZE];
  double alpha;
  double *cur_vec, *nex_vec;

  cur_vec = (double*)malloc(sizeof(double) * VEC_SIZE);
  nex_vec = (double*)malloc(sizeof(double) * VEC_SIZE);

  memcpy (cur_vec, vector, sizeof(double) * VEC_SIZE);

  while (TRUE)
    {
      zk = NULL;
      for (int i = 0; i < shell_size; ++i)
        {
          double scalar;

          scalar = scalar_multiplication (shell[i], cur_vec, VEC_SIZE);
          if (scalar <= 0)
            {
              zk = shell[i];
              break;
            }
        }

      if (zk == NULL)
        {
          memcpy (best, cur_vec, sizeof(double) * VEC_SIZE);
          goto ending;
        }

      SUBSTRACT_2_VECTORS (cur_vec, zk, temp, VEC_SIZE);
      alpha = scalar_multiplication (cur_vec, temp, VEC_SIZE) / scalar_multiplication (temp, temp, VEC_SIZE);

      if (alpha <= 0)
        memcpy (nex_vec, cur_vec, sizeof(double) * VEC_SIZE);
      else
        {
          SUBSTRACT_2_VECTORS (zk, cur_vec, temp, VEC_SIZE);
          VECTOR_FOREACH (temp, temp, VEC_SIZE, mul_2_values, alpha);
          SUM_2_VECTORS (temp, cur_vec, nex_vec, VEC_SIZE);
        }

      SUBSTRACT_2_VECTORS (cur_vec, nex_vec, temp, VEC_SIZE);
      if (vector_length (temp, VEC_SIZE) < CONFIG_EPS)
        {
          memcpy (best, nex_vec, sizeof(double) * VEC_SIZE);
          goto ending;
        }

      memcpy (cur_vec, nex_vec, sizeof(double) * VEC_SIZE);
    }

ending:
  free (cur_vec);
  free (nex_vec);
}

static void
learning (double **samples1, double **samples2, int size1, int size2, neuron_model *neuron)
{
  double min_length, shell_length;
  double **shell;
  double *best_vec;
  double max_gamma, min_beta;
  double *last_op_res, *div_vector;

  min_length = DBL_MAX;
  shell_length = size1 * size2;
  shell = (double**)malloc(sizeof(double*) * shell_length);

  last_op_res = (double*)malloc(sizeof(double) * VEC_SIZE);
  div_vector = (double*)malloc(sizeof(double) * VEC_SIZE);

  for (int i = 0; i < size1; ++i)
    for (int j = 0; j < size2; ++j)
      {
        int index, cur_len;

        index = i * size2 + j;
        SUBSTRACT_2_VECTORS (samples1[i], samples2[j], last_op_res, VEC_SIZE);
        shell[index] = (double*)malloc(sizeof(double) * VEC_SIZE);
        memcpy (shell[index], last_op_res, sizeof(double) * VEC_SIZE);

        cur_len = vector_length ( last_op_res, VEC_SIZE);
        if (cur_len < min_length)
          {
            min_length = cur_len;
            memcpy ( div_vector, last_op_res, sizeof(double) * VEC_SIZE);
          }
      }

  best_vec = (double*)malloc(sizeof(double) * VEC_SIZE);
  find_best_dividing_with_kozinets (div_vector, best_vec, shell, shell_length);

  max_gamma = -DBL_MAX;
  min_beta = DBL_MAX;
  for (int i = 0; i < size1; ++i)
    {
      double beta;

      beta = scalar_multiplication (samples1[i], best_vec, VEC_SIZE);
      min_beta = (beta < min_beta) ? beta : min_beta;
    }
  for (int i = 0; i < size2; ++i)
    {
      double gamma;

      gamma = scalar_multiplication (samples2[i], best_vec, VEC_SIZE);
      max_gamma = (gamma > max_gamma) ? gamma : max_gamma;
    }

  if (neuron->vector == NULL)
    neuron->vector = (double*)malloc(sizeof(double) * VEC_SIZE);
  memcpy (neuron->vector, best_vec, sizeof(double) * VEC_SIZE);
  neuron->theta = (min_beta + max_gamma) / 2;

  for (int i = 0; i < shell_length; ++i)
    free (shell[i]);
  free (shell);
  free (best_vec);
  free (last_op_res);
  free (div_vector);
}

void
learning_for_2_classes (classes_container *container, neuron_model *neuron)
{
  g_assert (container->n_of_classes == 2);
  learning (container->classes[0].train_samples, container->classes[1].train_samples,
      TRAIN_SAMPLES_COUNT, TRAIN_SAMPLES_COUNT, neuron);
}

static int
predict (GdkPixbuf *image, neuron_model *neuron, int is_mult)
{
  double *vector;
  GdkPixbuf *resized;
  double scalar;

  resized = resize (image, WIDTH_STEP, HEIGHT_STEP, LESSER, FALSE);
  vector = get_vector_from_pixbuf (resized);

  scalar = scalar_multiplication (vector, neuron->vector, VEC_SIZE);

  g_object_unref (resized);
  free (vector);

//  if (abs(scalar) < neuron->theta && !is_mult)
//    return -1;

  if (scalar >= neuron->theta)
    return 0;
  else
    return 1;
}

int
predict_result_for_2_classes (GdkPixbuf *image, neuron_model *neuron)
{
  return  predict (image, neuron, FALSE);
}

gboolean
check_linear_separability_for_multiple_classes (classes_container *container)
{
  int n_of_classes, packed_size;
  double *rand_vector;
  double **packed_samples;
  gboolean res;

  if ((n_of_classes = container->n_of_classes) < 3)
    return FALSE;

  rand_vector = (double*)malloc(sizeof(double) * VEC_SIZE);
  packed_size = TRAIN_SAMPLES_COUNT * 2;
  packed_samples = (double**)malloc(sizeof(double*) * packed_size);
  for (int i = 0; i < packed_size; ++i)
    packed_samples[i] = (double*)malloc(sizeof(double) * VEC_SIZE);

  for (int i = 0; i < n_of_classes; ++i)
    for (int j = i + 1; j < n_of_classes; ++j)
      {
        GENERATE_RANDOM_VECTOR (rand_vector, VEC_SIZE);
        pack_train_samples_for_2_classes (container, packed_samples, i, j);
        res = check_linear_separability (rand_vector, packed_samples, TRAIN_SAMPLES_COUNT * 2);

        if (!res)
          goto ending;
      }

ending:
  for (int i = 0; i < packed_size; ++i)
    free (packed_samples[i]);
  free (packed_samples);
  free (rand_vector);

  return res;
}

void
learning_for_multiple_classes (classes_container *container, neuron_model **neurons)
{
  int n_of_classes, offset;
  double **other_classes_samp;

  n_of_classes = container->n_of_classes;
  other_classes_samp = (double**)malloc(sizeof(double*) * TRAIN_SAMPLES_COUNT * (n_of_classes - 1));

  for (int i = 0; i < n_of_classes; ++i)
    {
      offset = 0;
      for (int j = 0; j < n_of_classes; ++j)
        {
          if (i != j)
            {
              memcpy (other_classes_samp + offset, container->classes[j].train_samples, sizeof(double*) * TRAIN_SAMPLES_COUNT);
              offset += TRAIN_SAMPLES_COUNT;
            }
        }
      learning (container->classes[i].train_samples, other_classes_samp, TRAIN_SAMPLES_COUNT,
                TRAIN_SAMPLES_COUNT * (n_of_classes - 1), neurons[i]);
    }

  free (other_classes_samp);
}

int
predict_result_for_multiple_classes (GdkPixbuf *image, neuron_model **neurons, int n_of_classes)
{
  int *predict_results, predicted_class;

  predict_results = (int*)malloc(sizeof(int) * n_of_classes);

  for (int i = 0; i < n_of_classes; ++i)
    predict_results[i] = predict (image, neurons[i], TRUE);

  predicted_class = -1;
  for (int i = 0; i < n_of_classes; ++i)
    {
      if (predict_results[i] == 0)
        {
          if (predicted_class == -1)
            predicted_class = i;
          else
            return -1;
        }
      else if (predict_results[i] == -1)
        return -1;
    }

  free (predict_results);

  return predicted_class;
}
