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
      uint32_t old_tail = m_tail;
      uint32_t old_head = m_head;
      bool old_empty = m_empty;

      // For each instruction currently in the buffer, decrement
      // their cycles to complete count.
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

      // Write instructions to the ARF.
      if (!m_empty)
        {
          uint32_t nwritten = 0;
          do
            {
              // If entry is valid, write it.
              if (m_buf[m_head].valid)
                {
                  cout << "Write from " << m_head << endl;
                  m_head = (m_head + 1) % m_size;
                  nwritten++;
                }
              else
                break;
             }
          while (m_head != m_tail && nwritten < m_n);
          m_nwarf += nwritten;

          if (m_head == m_tail && nwritten)
            m_empty = true;
        }
      // Process head pointer.
      m_nbiton = bits_on (m_head, old_head);

      // Read instructions from issue
      if ((old_empty || (old_head != m_tail))
          && ins[ins_num].type != -1)
        {
          uint32_t nread = 0;
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
          while (m_tail != old_head && nread < m_n
                 && ins[ins_num].type != -1);
          m_empty = false;
          m_nriq += nread;
        }
      // Process tail pointer.
      m_nbiton += bits_on (m_tail, old_tail);

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
