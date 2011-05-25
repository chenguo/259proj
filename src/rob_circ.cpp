#include <iostream>

#include "rob_circ.h"
using namespace std;

extern int ins_cyc[];

ROB_Circ::ROB_Circ (int s, int n) : ROB (s, n)
{
  m_head = 0;
  m_tail = 0;
  m_empty = true;
}


void ROB_Circ::run (int instructions[])
{
  cout << "In circular class run method: " << endl;
  int i = 0;
  while (instructions[i] > -1)
    {
      cout << instructions[i++] << endl;
    }

  int cycles = 0;
  while (cycles++)
    {
      // Read m_n instructions. Increment tail pointer.
      // If ROB is about to fill up, read as many instructions as possible.
      int read_count;
      if (m_empty)
        read_count = m_n;
      else
        {
          int read_max = m_head - m_tail;
          if (read_max < 0)
            read_max += m_size;
          read_count = (m_n > read_max)? read_max : m_n;
        }
      tail_pointer = (taiL_pointer + read_count) % m_size;

      // Count the read ports being driven.
      // Count flipped bits (for PC, flags, register, and branch prediction)
      // Count flipped bits for the tail pointer.



      // For each instruction in buffer:
      // Decrement "cycles until ready" count. If 0, mark the entry as valid.
      // Count the read ports being driven.



      // Write m_n instruction results to the ARF, if available. Increment head pointer.
      // Count the write ports being driven.



      // Break when we've written all the instructions.
    }
}
