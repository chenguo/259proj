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


void ROB_Circ::run (ins_t ins[])
{
  cout << "In circular class run method: " << endl;

  int cycles = 0;
  int ins_num = 0;

  // NOTE: During each cycle, everything happens in parallel. So in this loop,
  // be careful of how variable changes may affect this.
  while (1)
    {
      uint32_t old_head = m_head;
      bool old_empty = m_empty;

      update_entries ();
      write_to_arf ();
      ins_num = read_from_iq (old_head, old_empty, ins_num, ins);

      // TODO: Count the read ports being driven for operands.
      // Get percentage this happens from Henry.

      cycles++;
      cout << "Cycles: " << cycles << endl;
      cout << "Head: " << m_head << endl;
      cout << "Tail: " << m_tail << endl;
      cout << "Empty: " << m_empty << endl;
      cout << "Write to ARF: " << m_nwarf << endl;
      cout << "Read from IQ: " << m_nriq << endl;
      cout << "Read from EX: " << m_nrex << endl << endl;

      // Break when we've written all the instructions.
      if (m_empty && ins[ins_num].type == -1)
        break;
    }

  cout << "Simulation complete." << endl;
}

// For each instruction between the head and tail buffers, decrement their
// cycles to completion count. When it reaches 0, drive the FROM_EX port.
void ROB_Circ::update_entries ()
{
  if (!m_empty)
    {
      uint32_t i = m_head;
      do
        {
          if (m_buf[i].cycles)
            {
              m_buf[i].cycles--;

              if (m_buf[i].cycles == 0)
                {
                  cout << "Got result " << i << endl;
                  m_buf[i].valid = true;
                  m_nbiton = bits_on (true, false);
                  m_nrex++;
                }
            }
          i = (i + 1) % m_size;
        }
      while (i != m_tail);
    }
}

// From the head pointer, commit up to m_n instructions to the ARF.
void ROB_Circ::write_to_arf ()
{
  uint32_t nwritten = 0;
  if (!m_empty)
    {
      // Loop through entries. Stopping conditions:
      // 1. m_head == m_tail: buffer is empty
      // 2. nwritten == m_n: all commit ports used
      // 3. m_buf[m_head].valid == false: invalid entry
      do
        {
          // Check entry for validity.
          if (m_buf[m_head].valid)
            {
              // If valid, commit it.
              cout << "Write from " << m_head << endl;
              m_head = (m_head + 1) % m_size;
              nwritten++;
            }
        }
      while (m_head != m_tail && nwritten < m_n && m_buf[m_head].valid);
      // Update writes to ARF count.
      m_nwarf += nwritten;

      // If buffer is emty now, set the flag.
      if (m_head == m_tail && nwritten)
        m_empty = true;
    }
  // Process head pointer bit flips.
  m_nbiton = bits_on (m_head, m_head - nwritten);
}

// Read instructions from issue, up to m_n instructions.
int ROB_Circ::read_from_iq (uint32_t old_head, bool old_empty,
                            int ins_num, ins_t ins[])
{
  uint32_t nread = 0;

  // Read instructions from issue, if
  // 1. Buffer is empty
  // 2. Buffer is not empty, but head != tail
  //    (buffer is full if !empty and head == tail)
  // 3. Instruction sequence isn't over (denoted by -1 type)
  if ((old_empty || (old_head != m_tail))
      && ins[ins_num].type != -1)
    {
      do
        {
          cout << "Read into " << m_tail << endl;
          // Write entry.
          entry_t old_entry = m_buf[m_tail];
          m_buf[m_tail].valid = false;
          m_buf[m_tail].cycles = ins_cyc[ins[ins_num].type];
          m_buf[m_tail].pc = ins[ins_num].pc;
          m_buf[m_tail].reg_id = 0;  // TODO: ???
          ins_num++;
          nread++;

          // Count bits turned on.
          m_nbiton += bits_on (m_buf[m_tail].pc, old_entry.pc);
          m_nbiton += bits_on (m_buf[m_tail].reg_id, old_entry.reg_id);
          m_tail = (m_tail + 1) % m_size;
        }
      while (m_tail != old_head && nread < m_n && ins[ins_num].type != -1);

      m_empty = false;
      m_nriq += nread;
    }
  // Process tail pointer bit flips.
  m_nbiton += bits_on (m_tail, m_tail - nread);
  return ins_num;
}
