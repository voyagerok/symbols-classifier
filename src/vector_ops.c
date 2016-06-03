#include "vector_ops.h"
#include <stdlib.h>
#include <time.h>
#include <math.h>

double*
generate_random_vector (int vecsize)
{
  double *vector;

  vector = (double*)malloc(sizeof(double) * vecsize);
  srand(time(NULL));
  for (int i = 0; i < vecsize; ++i)
    vector[i] = rand() % 2;

  return vector;
}

void
generate_random_vector_2 (double *vec, int vecsize)
{
  srand(time(NULL));
  for (int i = 0; i < vecsize; ++i)
    vec[i] = rand() % 2;
}


double*
invert_vector (double *vec, int size)
{
  double *inverted;

  inverted = (double*)malloc(sizeof(double) * size);
  for (int i = 0; i < size; ++i)
    inverted[i] = - vec[i];

  return inverted;
}

double
scalar_multiplication (double *vec1, double *vec2, int size)
{
  double res;

  res = 0;
  for (int i = 0; i < size; ++i)
    res += vec1[i] * vec2[i];

  return res;
}

double*
sum_2_vectors (double *vec1, double *vec2, int size)
{
  double *res;

  res = (double*)malloc(sizeof(double) * size);
  for (int i = 0; i < size; ++i)
    res[i] = vec1[i] + vec2[i];

  return res;
}

void
sum_2_vectors_2 (double *vec1, double *vec2, double *out, int size)
{
  for (int i = 0; i < size; ++i)
    out[i] = vec1[i] + vec2[i];
}

double*
substract_2_vectors (double *vec1, double *vec2, int size)
{
  double *res;

  res = (double*)malloc(sizeof(double) * size);
  for (int i = 0; i < size; ++i)
    res[i] = vec1[i] - vec2[i];

  return res;
}

void
substract_2_vectors_2 (double *vec1, double *vec2, double *out, int size)
{
  for (int i = 0; i < size; ++i)
    out[i] = vec1[i] - vec2[i];
}


double*
vector_foreach (double *vec, int size, vec_proc_func func, double arg)
{
  double *res;

  res = (double*)malloc(sizeof(double) * size);
  for (int i = 0; i < size; ++i)
    res[i] = func (vec[i], arg);

  return res;
}

void
vector_foreach_2 (double *vec, double *out, int size, vec_proc_func func, double arg)
{
  for (int i = 0; i < size; ++i)
    out[i] = func (vec[i], arg);
}

double
vector_length (double *vec, int size)
{
  double sum;

  sum = 0;
  for (int i = 0; i < size; ++i)
    sum += SQR(vec[i]);

  return sqrt(sum);
}
