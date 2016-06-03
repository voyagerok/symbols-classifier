#include "neuron.h"
#include <stdlib.h>
#include "constants.h"

void
init_neuron (neuron_model **neuron)
{
  (*neuron) = (neuron_model*)malloc(sizeof(neuron_model));
  (*neuron)->theta = 0;
  (*neuron)->vector = NULL;
}

void
free_neuron (neuron_model **neuron)
{
  if ((*neuron)->vector != NULL)
    free ((*neuron)->vector);
  free ((*neuron));
}

void
clear_neuron (neuron_model *neuron)
{
  if (neuron->vector != NULL)
    {
      free (neuron->vector);
      neuron->vector = NULL;
    }
  neuron->theta = 0;
}
