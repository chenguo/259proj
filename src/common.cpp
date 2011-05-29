#include "common.h"
#include <iostream>

using namespace std;


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

int num_trans(uint32_t oval, uint32_t nval, uint32_t num) {
  if(num > 32)
    num = 32;
  uint32_t count = 0;
  uint32_t mask = 0x0001;
  for(uint32_t i = 0; i < num ; i++) {
    if((mask&oval)^(mask&nval))
      count++;
    oval = oval>>1;
    nval = nval>>1;
  }
  return count;
}

int num_hi(uint32_t val, uint32_t num) {
  if(num > 32)
    num = 32;
  uint32_t count = 0;
  uint32_t mask = 0x0001;
  for(uint32_t i = 0; i < num; i++) {
    if(mask&val)
      count++;
    val = val>>1;
  }
  return count;
}

//count number of 0 to 1 transtions from oval to nval
//num lower bits are used
int num_hi_trans(uint32_t oval, uint32_t nval, uint32_t num){
	if (num > 32) 
		num = 32;

	uint32_t count=0; 
	uint32_t mask = 0x0001; 
	
	for (uint32_t i = 0; i< num ; i++){
		if (!(oval&mask) )
		{
			if (nval & mask){
				count++;
			}
		}
		oval= oval>>1;
		nval = nval >>1;
	}
	return count;
}

int num_lo_trans(uint32_t oval, uint32_t nval, uint32_t num){
        if (num > 32)
                num = 32;

        uint32_t count=0;
        uint32_t mask = 0x0001;

        for (uint32_t i = 0; i< num ; i++){
                if (oval&mask)
                {
                        if (!(nval & mask)){
                                count++;
                        }
                }
                oval= oval>>1;
                nval = nval >>1;
        }
        return count;
}

