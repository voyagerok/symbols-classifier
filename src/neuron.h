#ifndef NEURON_H
#define NEURON_H

typedef struct neuron_model
{
  double *vector;
  double theta;
} neuron_model;

void
init_neuron (neuron_model **neuron);

void
free_neuron (neuron_model **neuron);

void
clear_neuron (neuron_model *neuron);

#endif // NEURON_H
