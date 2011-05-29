#include <iostream>

#include "rob_dyn.h"
using namespace std;

extern int ins_cyc[];

ROB_Dyn::ROB_Dyn (int s, int in, int fn, int overflow, int parts)
  : ROB_Circ (s, in, fn)
{
  m_update_period = 2048;
  m_sample_period = 256;
  m_overflow_thresh = overflow;

  m_nparts = parts;
  m_part_size = (uint32_t) (s / m_nparts);
  m_parts = new part_t [m_nparts];

  for (int i = 0; i < m_nparts; i++)
    {
      m_parts[i].use = true;
      m_parts[i].buf = new entry_t [m_part_size];
    }
}

ROB_Dyn::~ROB_Dyn ()
{
  for (int i = 0; i < m_nparts; i++)
    delete [] m_parts[i].buf;

  delete [] m_parts;
}

void ROB_Dyn::run (ins_t ins[])
{

}
