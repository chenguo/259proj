#include "common.h"

int ins_cyc[] = {
  20, 20, 1, 1, 3, 18, 3, 3, 5, 6
};

// Count the number of bits that were turn on from B to A.
int bits_on (uint32_t a, uint32_t b)
{
  int bits_on = 0;
  uint32_t mask = 1;
  for (int i = 0; i < 32; i++)
    {
      if ((a & mask) && !(b & mask))
        bits_on++;
      mask = mask << 1;
    }

  return bits_on;
}
