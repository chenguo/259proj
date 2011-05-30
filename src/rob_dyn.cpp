#include <iostream>

#include "rob_dyn.h"
using namespace std;

extern int ins_cyc[];

ROB_Dyn::ROB_Dyn (int s, int in, int fn, int overflow, int parts)
  : ROB_Circ (s, in, fn)
{
  m_update_period = 2048;
  m_sample_period = 256;
  m_samples_taken = 0;
  m_current_size = s;
  m_active_size = 0;
  m_overflow_cnt = 0;
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
  cout << "In dynamic class run method." << endl;

  int cycles = 0;
  int ins_num = 0;

  // TODO: power tabulation. Should be largely the same as circular, but
  // with the added cost of sampling and resizing circuitry.
  while (1)
    {
      // TODO: get overflow count.
      // Should be when m_head - m_prev_head < m_n, but think about it more.

      bool old_empty = m_empty;
      update_entries ();
      write_to_arf ();
      ins_num = read_from_iq (old_empty, ins_num, ins);

      cycles++;
      dyn_process (cycles);

      if (m_empty && ins[ins_num].type == -1)
        break;
    }
}

// Increment a point (head or tail) by one. Does not check for
// validity of pointer (i.e. tail passing head)
uint32_t ROB_Dyn::ptr_incr (uint32_t ptr)
{
  ptr++;
  if (ptr % m_part_size == 0)
    {
      // Pointer incremented to next partition. Make sure it's a valid one.
      int part_num = ptr / m_part_size;
      while (m_parts[part_num++].use == false)
        {
          // If pointer runs off end, put it at beginning.
          if (part_num == m_nparts)
            {
              part_num = 0;
              ptr = 0;
            }
          else
            ptr += m_part_size;
        }
    }
  return ptr;
}

// Get the entry associated with the pointer.
entry_t *ROB_Dyn::get_entry (uint32_t ptr)
{
  int part = ptr / m_nparts;
  int off = ptr % m_nparts;

  return &m_parts[part].buf[off];
}

// Take samples, and check if the buffer needs to be resized.
void ROB_Dyn::dyn_process (int cycles)
{
  // Update sampled average active size.
  if (cycles % m_sample_period == 0)
    {
      // TODO: Get entries used count.
      int used = 0;

      // Figure into samples.
      int tmp = m_active_size * m_samples_taken + used;
      m_samples_taken++;
      m_active_size = tmp / m_samples_taken;
    }

  // Update buffer size.
  if (cycles % m_update_period == 0)
    {
      if (m_current_size - m_active_size > m_part_size)
        {
          // Shrink buffer.

        }
      else if (m_overflow_cnt > m_overflow_thresh)
        {
          // Grow buffer.
        }

      m_overflow_cnt = 0;
      m_samples_taken = 0;
    }
}
