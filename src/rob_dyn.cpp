#include <iostream>

#include "rob_dyn.h"
using namespace std;

extern int ins_cyc[];

ROB_Dyn::ROB_Dyn (int s, int in, int fn, int overflow, int parts, int pflags)
  : ROB_Circ (s, in, fn, pflags)
{
  m_update_period = 512;
  m_sample_period = 64;
  m_samples_taken = 0;
  m_current_size = s;
  m_active_size = 0;
  m_overflow_cnt = 0;
  m_overflow_thresh = overflow;
  m_partition_cycles = 0;

  m_nparts = parts;
  m_part_size = (uint32_t) (s / m_nparts);
  m_parts = new part_t [m_nparts];
  m_prev_parts = new part_t [m_nparts];

  entry_t empty_entry = {false, 0, 0, 0, 0, false};
  for (int i = 0; i < m_nparts; i++)
    {
      m_parts[i].state = ENABLED;
      m_parts[i].buf = new entry_t [m_part_size];
      m_prev_parts[i].state = ENABLED;
      m_prev_parts[i].buf = new entry_t [m_part_size];

      for (uint32_t j = 0; j < m_part_size; j++)
        {
          m_parts[i].buf[j] = empty_entry;
          m_prev_parts[i].buf[j] = empty_entry;
        }
    }

  /*
  int tmp = 0;
  for (int i = 0; i < m_nparts; i++)
    for (int j = 0; j < m_part_size; j++)
      {
        cout << tmp << ": Part " << i << "  Off " << j << "  Addr " << &m_parts[i].buf[j] << endl;
        get_entry (tmp++);
      }
  */
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

      bool old_empty = m_empty;
      int old_ins_num = ins_num;
      update_entries ();
      write_to_arf ();
      ins_num = read_from_iq (old_empty, ins_num, ins);

      post_cycle_power_tabulation ();
      update_power_totals ();

      // Check overflow count:
      // 1) If less than m_n instructions were processed
      // 2) Buffer is full
      if (old_ins_num + (int) m_n > ins_num && (m_head == m_tail && !m_empty))
        m_overflow_cnt++;

      cycles++;
      dyn_process (cycles);

      if (m_print & DBG_FLAG)
        print_msgs (cycles);

      // Break when we've written all the instructions.
      if (m_empty && ins[ins_num].type == -1)
        break;
    }

  print_power_stats ();
}

// Take samples, and check if the buffer needs to be resized.
void ROB_Dyn::dyn_process (int cycles)
{
  // Update sampled average active size.
  if (cycles % m_sample_period == 0)
    {
      // Get entries used count.
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
      //cout << "Cur: " << m_current_size;
      //cout << "  Active: " << m_active_size;
      //cout << "  overflow " << m_overflow_cnt << endl;

      if (m_current_size > m_active_size && m_current_size - m_active_size > m_part_size)
        {
          // Shrink buffer.
          dyn_shrink (1);
        }
      if (m_overflow_cnt > m_overflow_thresh)
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
                {
                  m_parts[i].state = DISABLED;
                  m_current_size -= m_part_size;
                }
              else
                {
                  m_parts[i].state = ENABLED;
                  m_current_size += m_part_size;
                }
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
      if (m_parts[i].state == ENABLED)
        {
          m_parts[i].state = TO_DISABLE;
          disabled++;
        }
      else if (m_parts[i].state == TO_ENABLE)
        {
          m_parts[i].state = DISABLED;
          disabled++;
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
      if (m_parts[i].state == DISABLED)
        {
          m_parts[i].state = TO_ENABLE;
          enabled++;
        }
      else if (m_parts[i].state == TO_DISABLE)
        {
          m_parts[i].state = ENABLED;
          enabled++;
        }
      i++;
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
      while (m_parts[part_num].state != ENABLED)
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
  int part = ptr / m_part_size;
  int off = ptr % m_part_size;
  return &m_parts[part].buf[off];
}

entry_t *ROB_Dyn::get_prev_buf_entry (uint32_t ptr)
{
  int part = ptr / m_part_size;
  int off = ptr % m_part_size;
  return &m_prev_parts[part].buf[off];
}

void ROB_Dyn::error_diag ()
{

  cerr << "Head: " << m_head << endl;
  cerr << "Tail: " << m_tail << endl;
  int ptr = 0;
  for (int i = 0; i < m_nparts; i++)
    {
      cerr << "Partition " << i << ": ";
      for (uint32_t j = 0; j < m_part_size; j++)
        cerr << "[" << ptr++ << "]";

      cerr<< ": " << m_parts[i].state << endl;
    }
}

void ROB_Dyn::pre_cycle_power_snapshot ()
{
  for (int i = 0; i < m_nparts; i++)
    m_prev_parts[i].state = m_parts[i].state;

  ROB_Circ::pre_cycle_power_snapshot ();
}

void ROB_Dyn::post_cycle_power_tabulation ()
{
  m_partition_cycles += (m_current_size / m_part_size);

  for (int i = 0; i < m_nparts; i++)
    {
      if (m_parts[i].state == ENABLED &&
          (m_prev_parts[i].state == DISABLED ||
           m_prev_parts[i].state == TO_ENABLE))
        {
          if (m_print & DBG_FLAG)
            cout << "*TRANSITION* partition[" << i << "].state: 0->1" << endl;

          p_perCycleBitTransitions += 1;
          p_perCycleBitTransitionsHigh += 1;
        }
      else if (m_parts[i].state == DISABLED &&
               (m_prev_parts[i].state == ENABLED ||
                m_prev_parts[i].state == TO_DISABLE))
        {
          if (m_print & DBG_FLAG)
            cout << "*TRANSITION* partition[" << i << "].state: 1->0" << endl;
          p_perCycleBitTransitions += 1;
          p_perCycleBitTransitionsLow += 1;
        }
      m_prev_parts[i].state = m_parts[i].state;
    }

  ROB_Circ::post_cycle_power_tabulation ();
}

void ROB_Dyn:: print_power_stats ()
{
  cout << endl;
  cout << "Power statistics from the run: " << endl;
  cout << "Total # bits = num_cycles * # enabled partitions * bits per partition = " <<
    m_partition_cycles * m_part_size * 70 << endl;
  cout << "Total # bit transitions to high: " << p_totalBitTransitionsHigh << endl;
  cout << "Total # bit transitions to low: " << p_totalBitTransitionsLow << endl;
  cout << "Total # bits remained low: " << p_totalBitsRemainedLow << endl;
  cout << "Total # bits remained high: " << p_totalBitsRemainedHigh << endl;
  cout << "Total # times reg comparator is used: "<< p_reg_comp_used << endl << endl;
}
