#include "classes.h"
#include "constants.h"
#include <stdlib.h>
#include "imgproc.h"
#include <string.h>

void
init_container (classes_container **container)
{
  (*container) = (classes_container*)malloc(sizeof(classes_container));
  (*container)->n_of_classes = 0;
  (*container)->classes = (class_instance*)malloc(sizeof(class_instance) * CLASSES_MAX_COUNT);
  (*container)->classes_capacity = CLASSES_MAX_COUNT;
}

void
free_container (classes_container **container)
{
  if ((*container) != NULL)
    {
      if ((*container)->n_of_classes > 0)
        {
          for (int i = 0; i < (*container)->n_of_classes; ++i)
            {
              for (int j = 0; j < TEST_SAMPLES_COUNT; ++j)
                  free ((*container)->classes[i].test_samples[j]);
              for (int j = 0; j < TRAIN_SAMPLES_COUNT; ++j)
                  free ((*container)->classes[i].train_samples[j]);
              free ((*container)->classes[i].test_samples);
              free ((*container)->classes[i].train_samples);
              free ((*container)->classes[i].name);
              g_object_unref ((*container)->classes[i].init_image);
            }
        }
      free ((*container)->classes);
      free ((*container));
    }
}

void
add_class_to_container (classes_container *container, GdkPixbuf *image, char *name)
{
  double **train, **test, *vector;
  int n_of_classes;
  GdkPixbuf *resized, *noised;
  int name_length;

  g_assert (container != NULL);

  n_of_classes = container->n_of_classes;
  container->classes[n_of_classes].test_samples = (double**)malloc(sizeof(double*) * TEST_SAMPLES_COUNT);
  container->classes[n_of_classes].train_samples = (double**)malloc(sizeof(double*) * TRAIN_SAMPLES_COUNT);

  train = container->classes[n_of_classes].train_samples;
  test = container->classes[n_of_classes].test_samples;

  srand(time(NULL));
  for (int i = 0; i < TEST_SAMPLES_COUNT; ++i)
    {
      test[i] = (double*)malloc(sizeof(double) * VEC_SIZE);

      resized = resize (image, WIDTH_STEP, HEIGHT_STEP, LESSER, TRUE);
      noised = get_modified_image (resized, TRUE);
      //to_binary_image (noised);
      vector = get_vector_from_pixbuf (noised);

      memcpy (test[i], vector, sizeof(double) * VEC_SIZE);

      free (vector);
      g_object_unref (resized);
      g_object_unref (noised);
    }

  for (int i = 0; i < TRAIN_SAMPLES_COUNT; ++i)
    {
      train[i] = (double*)malloc(sizeof(double) * VEC_SIZE);

      resized = resize (image, WIDTH_STEP, HEIGHT_STEP, LESSER, TRUE);
      noised = get_modified_image (resized, TRUE);
      //to_binary_image (noised);
      vector = get_vector_from_pixbuf (noised);

      memcpy (train[i], vector, sizeof(double) * VEC_SIZE);

      free (vector);
      g_object_unref (resized);
      g_object_unref (noised);
    }

  name_length = strlen(name) + 1;
  container->classes[n_of_classes].name = (char*)malloc(name_length);
  memcpy (container->classes[n_of_classes].name, name, name_length);

  container->classes[n_of_classes].init_image = gdk_pixbuf_copy(image);

  container->n_of_classes++;
}

void
clear_container (classes_container *container)
{
  int n_of_classes;

  g_assert (container != NULL);

  n_of_classes = container->n_of_classes;
  for (int i = 0; i < n_of_classes; ++i)
    {
      for (int j = 0; j < TEST_SAMPLES_COUNT; ++j)
          free (container->classes[i].test_samples[j]);
      for (int j = 0; j < TRAIN_SAMPLES_COUNT; ++j)
          free (container->classes[i].train_samples[j]);
      free (container->classes[i].test_samples);
      free (container->classes[i].train_samples);
      free (container->classes[i].name);
      g_object_unref (container->classes[i].init_image);
    }
  container->n_of_classes = 0;
}
