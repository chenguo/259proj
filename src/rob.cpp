#include <iostream>

#include "rob.h"
using namespace std;

extern int ins_cyc[];

ROB::ROB (int s, int n)
  : m_size(s)
  , m_n(n)
{
  m_buf = new entry_t [m_size];
  m_prev_buf = new entry_t [m_size];
  FROM_IQ = PC_SIZE;
  FROM_EX = DATA_SIZE;
  TO_DU = DATA_SIZE;
  TO_ARF = DATA_SIZE;

  p_totalBitTransitions = 0;
  p_totalBitTransitionsHigh = 0;
  p_totalBitTransitionsLow = 0;
  p_totalBitsHigh = 0;
  p_totalBitsLow = 0;
}

ROB::~ROB ()
{
  delete [] m_buf;
}

void ROB::run (ins_t instructions[])
{
  cout << "In base class run method: " << endl;
}
