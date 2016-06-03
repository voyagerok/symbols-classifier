#ifndef CLASSES_H
#define CLASSES_H

#include <gtk/gtk.h>

typedef struct class_instance
{
  char *name;
  double **train_samples;
  double **test_samples;
  GdkPixbuf *init_image;
} class_instance;

typedef struct _classes_container
{
  class_instance *classes;
  int n_of_classes;
  int classes_capacity;
} classes_container;

void
init_container (classes_container **container);
void
add_class_to_container (classes_container *container, GdkPixbuf *image, char *name);
void
clear_container (classes_container *container);
void
free_container (classes_container **conatiner);

#endif // CLASSES_H
