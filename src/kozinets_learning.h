#ifndef KOZINETS_LEARNING_H
#define KOZINETS_LEARNING_H

#include <gtk/gtk.h>
#include "classes.h"
#include "neuron.h"

#define CONFIG_STEP 0.1
#define CONFIG_EPS 0.0001

gboolean
check_linear_separability_for_2_classes (classes_container *classes);

gboolean
check_linear_separability_for_multiple_classes (classes_container *classes);

void
learning_for_2_classes (classes_container *classes, neuron_model *neuron);

void
learning_for_multiple_classes (classes_container *classes, neuron_model **neurons);

int
predict_result_for_2_classes (GdkPixbuf *image, neuron_model *neuron);

int
predict_result_for_multiple_classes (GdkPixbuf *image, neuron_model **neurons, int n_of_classes);

#endif // KOZINETS_LEARNING_H
