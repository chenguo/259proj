#include <iostream>
#include <math.h>

#include "rob_circ.h"
using namespace std;

extern int ins_cyc[];

ROB_Circ::ROB_Circ (int s, int in, int fn) : ROB (s, in, fn)
{
  m_head = 0;
  m_tail = 0;
  m_empty = true;
  m_head_size = (uint32_t)((log(m_size) / log(2))+1);
  p_bit_count += (2*m_head_size);

  cout << "New ROB_Circ:" << endl;
  cout << "# Entries: " << m_size << endl;
  cout << "Entry size: 70 bits" << endl;
  cout << "Head and tail pointer bits: " << m_head_size << endl;
  cout << "Total bits in ROB: " << p_bit_count << endl;
}


ROB_Circ::~ROB_Circ() {
}

void ROB_Circ::run (ins_t ins[])
{
  cout << "In circular class run method: " << endl;

  int cycles = 0;
  int ins_num = 0;

  // NOTE: During each cycle, everything happens in parallel. So in this loop,
  // be careful of how variable changes may affect this.
  while (1)
    {
      pre_cycle_power_snapshot();
      bool old_empty = m_empty;

      update_entries ();
      write_to_arf ();
      ins_num = read_from_iq (old_empty, ins_num, ins);

      // TODO: Count the read ports being driven for operands.
      // Get percentage this happens from Henry.

      post_cycle_power_tabulation();
      update_power_totals();

      cycles++;
      print_msgs (cycles);


      // Break when we've written all the instructions.
      if (m_empty && ins[ins_num].type == -1)
        break;
    }

  print_power_stats(cycles);

  cout << "Simulation complete." << endl;
}

// For each instruction between the head and tail buffers, decrement their
// cycles to completion count. When it reaches 0, drive the FROM_EX port.
void ROB_Circ::update_entries ()
{
  if (m_empty)
    return;

  uint32_t i = m_head;
  do
    {
      entry_t *entry = get_entry (i);
      if (entry->cycles && --entry->cycles == 0)
        {
          cout << "Got result " << i << endl;
          entry->valid = true;
          m_nbiton = bits_on (true, false);
          m_nrex++;
        }
      i = head_incr (i);
    }
  while (i != m_tail);
}

// From the head pointer, commit up to m_n instructions to the ARF.
void ROB_Circ::write_to_arf ()
{
  if (m_empty)
    return;

  uint32_t m = m_n;
  uint32_t nwritten = 0;
  entry_t *entry = NULL;

  // Loop through entries. Stopping conditions:
  // 1. m_head == m_tail: buffer is empty
  // 2. nwritten == m_n: all commit ports used
  // 3. m_buf[m_head].valid == false: invalid entry
  do
    {
      // Get entry and check it for validity. If valid, commit it.
      entry = get_entry (m_head);
      if (entry->valid)
        {
          if (entry->isfp)
            m++;
          cout << "Write from " << m_head << endl;
          m_head = head_incr (m_head);
          nwritten++;
        }
    }
  while (m_head != m_tail && nwritten < m && entry->valid);
  // Update writes to ARF count.
  m_nwarf += nwritten;

  // If buffer is emty now, set the flag.
  if (m_head == m_tail && nwritten)
    m_empty = true;
}

// Read instructions from issue, up to m_n instructions.
int ROB_Circ::read_from_iq (bool old_empty, int ins_num, ins_t ins[])
{
  uint32_t nread = 0;
  uint32_t nentries = 0;
  uint32_t f_issued = 0;

  // Read instructions from issue, if
  // 1. Buffer is empty
  // 2. Buffer is not empty, but head != tail
  //    (buffer is full if !empty and head == tail)
  // 3. Instruction sequence isn't over (denoted by -1 type)
  if ((old_empty || (m_prev_head != m_tail))
      && ins[ins_num].type != -1)
    {
      do
        {
          cout << "Read into " << m_tail << endl;

          // Write entry.
          // FP instruction uses 2 slots.
          if (ins[ins_num].type >= FADD)
            {
              if (tail_incr (m_tail) != m_head)
                {
                  cout << "FP ins processed" << endl;

                  // First half of entry.
                  write_entry (get_entry (m_tail), ins[ins_num]);
                  nentries++;
                  f_issued++;
                }
              else
                {
                  cout<<"No space for a FP, stall."<<endl;
                  break;
                }
            }
          write_entry (get_entry (m_tail), ins[ins_num]);
          nentries++;
          nread++;
          ins_num++;
        }
      while (m_tail != m_prev_head && nentries < m_n && ins[ins_num].type > -1);

      m_empty = false;
      m_nriq += nread;

      if (f_issued > m_fn)
        m_fp_delay += f_issued - m_fn;
      else if (m_fp_delay >= m_fn - f_issued)
        m_fp_delay -= m_fn - f_issued;
    }

  return ins_num;
}

// Return the head pointer + 1.
uint32_t ROB_Circ::head_incr (uint32_t head_ptr)
{
  return (head_ptr + 1) % m_size;
}

// Return the tail pointer + 1.
uint32_t ROB_Circ::tail_incr (uint32_t tail_ptr)
{
  return (tail_ptr + 1) % m_size;
}

entry_t *ROB_Circ::get_entry (uint32_t ptr)
{
  return &m_buf[ptr];
}

void ROB_Circ::write_entry (entry *entry, ins_t ins)
{
  uint16_t reg_mask = 0xF;

  entry->valid = false;
  entry->cycles = ins_cyc[ins.type];
  if (ins.type >= FADD)
    entry->cycles += m_fp_delay;
  entry->pc = ins.pc;
  entry->reg_id = ins.regs & reg_mask;
  entry->isfp = (ins.type >= FADD);

  m_tail = tail_incr (m_tail);

  if(isinROB ((ins.regs >> 4) & reg_mask))
    m_nwdu++;
  if(isinROB ((ins.regs >> 8) & reg_mask))
    m_nwdu++;
}

void ROB_Circ::update_power_totals () {
  default_update_power_totals();
}

void ROB_Circ::post_cycle_power_tabulation () {
#ifdef P_DEBUG
  if(m_prev_head != m_head)
    cout << "*TRANSITION* m_head: " << m_prev_head << "->" << m_head << endl;
#endif
  p_perCycleBitTransitions += num_trans(m_prev_head, m_head, m_head_size);
  p_perCycleBitTransitionsHigh += num_hi_trans(m_prev_head, m_head, m_head_size);
  p_perCycleBitTransitionsLow += num_lo_trans(m_prev_head, m_head, m_head_size);
  p_perCycleBitsRemainedHigh += num_hi(m_head,m_head_size) - num_hi_trans(m_prev_head, m_head, m_head_size);
  p_perCycleBitsRemainedLow += m_head_size - num_hi(m_head,m_head_size) - num_lo_trans(m_prev_head, m_head, m_head_size);

#ifdef P_DEBUG
  if(m_prev_tail != m_tail)
    cout << "*TRANSITION* m_tail: " << m_prev_tail << "->" << m_tail << endl;
#endif
  p_perCycleBitTransitions += num_trans(m_prev_tail, m_tail, m_head_size);
  p_perCycleBitTransitionsHigh += num_hi_trans(m_prev_tail, m_tail, m_head_size);
  p_perCycleBitTransitionsLow += num_lo_trans(m_prev_tail, m_tail, m_head_size);
  p_perCycleBitsRemainedHigh += num_hi(m_tail, m_head_size) - num_hi_trans(m_prev_tail, m_tail, m_head_size);
  p_perCycleBitsRemainedLow += m_head_size - num_hi(m_tail, m_head_size) - num_lo_trans(m_prev_tail, m_tail, m_head_size);

  default_post_cycle_power_tabulation();
}

void ROB_Circ::pre_cycle_power_snapshot () {
  m_prev_head = m_head;
  m_prev_tail = m_tail;
  p_perCycleBitTransitions = 0;
  p_perCycleBitTransitionsHigh = 0;
  p_perCycleBitTransitionsLow = 0;
  p_perCycleBitsRemainedLow = 0;
  p_perCycleBitsRemainedHigh = 0;

  default_pre_cycle_power_snapshot();
}

void ROB_Circ::print_power_stats (int cycles) {
  default_print_power_stats(cycles);
}


bool ROB_Circ::isinROB( uint16_t reg)
{
  if (!m_empty)
    {
      uint32_t i = m_head;
      do
        {
          p_reg_comp_used++;
          entry_t *entry = get_entry (i);
          if (entry->reg_id == reg)
            return true;
          i = head_incr (i);
        }
      while (i != m_tail);
    }
  return false;
}

void ROB_Circ::print_msgs (int cycles)
{
  cout << "Cycles: " << cycles << endl;
  cout << "Head: " << m_head << endl;
  cout << "Tail: " << m_tail << endl;
  cout << "Empty: " << m_empty << endl;
  cout << "Write to ARF: " << m_nwarf << endl;
  cout << "Read from IQ: " << m_nriq << endl;
  cout << "Read from EX: " << m_nrex << endl;

  cout << "# Total Bits in ROB: " << p_bit_count << endl;
  cout << "# Bits Transitioned to High: " << p_perCycleBitTransitionsHigh << endl;
  cout << "# Bits Transitioned to Low: " << p_perCycleBitTransitionsLow << endl;
  cout << "# Bits Remained High: " << p_perCycleBitsRemainedHigh << endl;
  cout << "# Bits Remained Low: " << p_perCycleBitsRemainedLow << endl;
  cout << "# Times Reg Comparator is used: "<< p_reg_comp_used << endl << endl;
}
