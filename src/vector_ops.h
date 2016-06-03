#ifndef VECTOR_OPS_H
#define VECTOR_OPS_H

#include <gtk/gtk.h>

#define SQR(val) ((val) * (val))

#define GENERATE_RANDOM_VECTOR(vec, vecsize)\
{\
  srand(time(NULL));\
  for (int _ind = 0; _ind < vecsize; ++_ind)\
    *(vec + _ind) = rand() % 2;\
}


#define SUM_2_VECTORS(vec1, vec2, out, size)\
{\
  for (int _ind = 0; _ind < size; ++_ind)\
    *(out + _ind) = *(vec1 + _ind) + *(vec2 + _ind);\
}

#define SUBSTRACT_2_VECTORS(vec1, vec2, out, size)\
{\
  for (int _ind = 0; _ind < size; ++_ind)\
    *(out + _ind) = *(vec1 + _ind) - *(vec2 + _ind);\
}

#define VECTOR_FOREACH(vec, out, size, func, arg)\
{\
  for (int _ind = 0; _ind < size; ++_ind)\
    *(out + _ind) = func (*(vec + _ind), arg);\
}



#define INVERT_VECTOR(vec, out, size)\
{\
  for (int _ind = 0; _ind < size; ++_ind)\
    out[_ind] = - vec[_ind];\
}

typedef double (*vec_proc_func) (double, double);

double*
generate_random_vector (int vecsize);

void
generate_random_vector_2 (double *vec, int vecsize);

double*
invert_vector (double *vec, int size);

double
scalar_multiplication (double *vec1, double *vec2, int size);

double*
sum_2_vectors (double *vec1, double *vec2, int size);

void
sum_2_vectors_2 (double *vec1, double *vec2, double *out, int size);

double*
substract_2_vectors (double *vec1, double *vec2, int size);

void
substract_2_vectors_2 (double *vec1, double *vec2, double *out, int size);

double*
vector_foreach (double *vec, int size, vec_proc_func func, double arg);

void
vector_foreach_2 (double *vec, double *out, int size, vec_proc_func func, double arg);

double
vector_length (double *vec, int size);

#endif // VECTOR_OPS_H
