#include <stdlib.h>
#include "probability.h"

int
flip_coin (double probability)
{
  double rand_val;

  rand_val = (double)(rand() % 100000 + 1) / 100000;

  return rand_val <= probability;
}

