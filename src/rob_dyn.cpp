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
      m_parts[i].state = ENABLED;
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
      pre_cycle_power_snapshot ();

      // TODO: get overflow count.
      // Should be when m_head - m_prev_head < m_n, but think about it more.

      bool old_empty = m_empty;
      update_entries ();
      write_to_arf ();
      ins_num = read_from_iq (old_empty, ins_num, ins);

      post_cycle_power_tabulation ();
      update_power_totals ();

      cycles++;
      dyn_process (cycles);
      print_msgs (cycles);

      // Break when we've written all the instructions.
      if (m_empty && ins[ins_num].type == -1)
        break;
    }
}

// Increment a head pointer by one.
uint32_t ROB_Dyn::head_incr (uint32_t head_ptr)
{
  head_ptr = (head_ptr + 1) % m_size;
  if (head_ptr % m_part_size == 0)
    {
      // Pointer incremented to next partition. Make sure it's a valid one.
      int part_num = head_ptr / m_part_size;
      while (m_parts[part_num].state == DISABLED
             || m_parts[part_num].state == TO_ENABLE)
        {
          // If pointer runs off end, put it at beginning.
          if (++part_num == m_nparts)
            {
              part_num = 0;
              head_ptr = 0;
            }
          else
            head_ptr += m_part_size;
        }
    }
  return head_ptr;
}

// Increment a tail pointer by one.
uint32_t ROB_Dyn::tail_incr (uint32_t tail_ptr)
{
  tail_ptr = (tail_ptr + 1) % m_size;
  if (tail_ptr % m_part_size == 0)
    {
      // Pointer incremented to next partition. Make sure it's a valid one.
      int part_num = tail_ptr / m_part_size;
      while (m_parts[part_num].state == DISABLED
             || m_parts[part_num].state == TO_DISABLE)
        {
          // If pointer runs off end, put it at beginning.
          if (++part_num == m_nparts)
            {
              part_num = 0;
              tail_ptr = 0;
            }
          else
            tail_ptr += m_part_size;
        }
    }
  return tail_ptr;
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
      if (m_head == m_tail && !m_empty)
        used = m_size;
      else if (!m_empty)
        {
          used = m_tail - m_head;
          if (used < 0)
            used += m_current_size;
        }

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
          dyn_shrink (1);
        }
      else if (m_overflow_cnt > m_overflow_thresh)
        {
          // Grow buffer.
          dyn_grow (1);
        }

      m_overflow_cnt = 0;
      m_samples_taken = 0;
    }

  // If possible to turn buffer marked TO_DISABLE off, or TO_ENABLE on, do so.
  for (int i = 0; i < m_nparts; i++)
    {
      if (m_parts[i].state == TO_DISABLE || m_parts[i].state == TO_ENABLE)
        {
          uint32_t part_start = i * m_part_size;
          uint32_t part_end = part_start + m_part_size;

          // State change if section is not in middle of used region.
          // 1) Used region is before partition.
          // 2) Used region is after partition.
          // 3) Used region starts after partition and ends before partition
          //    after wrap around.
          if ((m_head < m_tail && (m_tail < part_start || m_head >= part_end))
              || (m_head > m_tail && (m_head >= part_end && m_tail < part_start))
              || (m_head == m_tail && m_empty))
            {
              if (m_parts[i].state == TO_DISABLE)
                m_parts[i].state = DISABLED;
              else
                m_parts[i].state = ENABLED;
            }
        }
    }
}

// Mark PARTS partitions for disabling, if possible.
void ROB_Dyn::dyn_shrink (int parts)
{
  int disabled = 0;
  int i = m_nparts - 1;
  while (disabled < parts && i >= 0)
    {
      if (m_parts[i].state == ENABLED || m_parts[i].state == TO_ENABLE)
        {
          m_parts[i].state = TO_DISABLE;
          disabled++;
          m_current_size -= m_part_size;
        }
      i--;
    }
}

// Mark PART partitions for enabling, if possible.
void ROB_Dyn::dyn_grow (int parts)
{
  int enabled = 0;
  int i = 0;
  while (enabled < parts && i < m_nparts)
    {
      if (m_parts[i].state == DISABLED || m_parts[i].state == TO_DISABLE)
        {
          m_parts[i].state = TO_ENABLE;
          enabled++;
          m_current_size += m_part_size;
        }
      i++;
    }
}
