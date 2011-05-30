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
  p_bit_count = (69*m_size + 2*m_head_size);
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
      uint32_t old_head = m_head;
      bool old_empty = m_empty;

      p_perCycleBitTransitions = 0;
      p_perCycleBitTransitionsHigh = 0;
      p_perCycleBitTransitionsLow = 0;
      p_perCycleBitsRemainedLow = 0;
      p_perCycleBitsRemainedHigh = 0;

      update_entries ();
      write_to_arf ();
      ins_num = read_from_iq (old_head, old_empty, ins_num, ins);

      // TODO: Count the read ports being driven for operands.
      // Get percentage this happens from Henry.

      post_cycle_power_tabulation();

      cycles++;
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
      cout << "# Bits Remained Low: " << p_perCycleBitsRemainedLow << endl << endl;

      p_totalBitTransitions += p_perCycleBitTransitions;
      p_totalBitTransitionsHigh += p_perCycleBitTransitionsHigh;
      p_totalBitTransitionsLow += p_perCycleBitTransitionsLow;
      p_totalBitsRemainedLow += p_perCycleBitsRemainedLow;
      p_totalBitsRemainedHigh += p_perCycleBitsRemainedHigh;

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
  uint32_t m = m_n;
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
              if (m_buf[m_head].isfp)
								m++;
							cout << "Write from " << m_head << endl;
              m_head = (m_head + 1) % m_size;
              nwritten++;
            }
        }
      while (m_head != m_tail && nwritten < m && m_buf[m_head].valid);
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
  int ileft = m_in; 
  int fleft = m_fn;

  uint16_t reg_mask = 0xF;
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

					//fp ins
          if (ins[ins_num].type>=FADD)
          {
						//2slots left
						if( ((m_tail+1)%m_size)!= m_head  && fleft >0 )
						{
							 cout<<"FP ins processed"<<endl;
							 //first entry
         			 m_buf[m_tail].valid = false;
         			 m_buf[m_tail].cycles = ins_cyc[ins[ins_num].type];
      				 m_buf[m_tail].pc = ins[ins_num].pc;
          		 m_buf[m_tail].reg_id = ins[ins_num].regs&reg_mask;
							 m_buf[m_tail].isfp = true;
              
               fleft--;
               m_tail = (m_tail + 1) % m_size;

               if(isinROB( (ins[ins_num].regs>>4)&reg_mask  ))
                  m_nwdu++;
               if(isinROB( (ins[ins_num].regs>>8)&reg_mask   ))
                  m_nwdu++;

							 //will add second entry below
						}
						else{
							cout<<"no space for a FP.. should skip to next ins"<<endl;
						}
					}
          else
          {
            //fp used for X integer ins
            if(ileft <=0 && fleft> 0)
            {
              ileft += 2;
              fleft--;
             }
          
             //break if no more spots for interger ops
             if(ileft==0)
             {
                break;
              }
          }

					//see below
//					entry_t old_entry = m_buf[m_tail];

          m_buf[m_tail].valid = false;
          m_buf[m_tail].cycles = ins_cyc[ins[ins_num].type];
          m_buf[m_tail].pc = ins[ins_num].pc;
          m_buf[m_tail].reg_id = ins[ins_num].regs&reg_mask;
				  m_buf[m_tail].isfp=false;

         if(isinROB( (ins[ins_num].regs>>4)&reg_mask   ))
            m_nwdu++;
         if(isinROB( (ins[ins_num].regs>>8)&reg_mask  ))
            m_nwdu++;
					

          m_tail = (m_tail + 1) % m_size;
          ins_num++;
          nread++;

          // Count bits turned on.
					//TODO: we probably dont need this cause alex is
					//check pointing the states per cycle
//          m_nbiton += bits_on (m_buf[m_tail].pc, old_entry.pc);
//          m_nbiton += bits_on (m_buf[m_tail].reg_id, old_entry.reg_id);
        }
      while (m_tail != old_head && nread < m_n && ins[ins_num].type != -1 );

      m_empty = false;
      m_nriq += nread;
    }
  // Process tail pointer bit flips.
  m_nbiton += bits_on (m_tail, m_tail - nread);
  return ins_num;
}

void ROB_Circ::post_cycle_power_tabulation () {
  if(m_prev_head != m_head)
    cout << "*TRANSITION* m_head: " << m_prev_head << "->" << m_head << endl;
  p_perCycleBitTransitions += num_trans(m_prev_head, m_head, m_head_size);
  p_perCycleBitTransitionsHigh += num_hi_trans(m_prev_head, m_head, m_head_size);
  p_perCycleBitTransitionsLow += num_lo_trans(m_prev_head, m_head, m_head_size);
  p_perCycleBitsRemainedHigh += num_hi(m_head,m_head_size) - num_hi_trans(m_prev_head, m_head, m_head_size);
  p_perCycleBitsRemainedLow += m_head_size - num_hi(m_head,m_head_size) - num_lo_trans(m_prev_head, m_head, m_head_size);

  if(m_prev_tail != m_tail)
    cout << "*TRANSITION* m_tail: " << m_prev_tail << "->" << m_tail << endl;
  p_perCycleBitTransitions += num_trans(m_prev_tail, m_tail, m_head_size);
  p_perCycleBitTransitionsHigh += num_hi_trans(m_prev_tail, m_tail, m_head_size);
  p_perCycleBitTransitionsLow += num_lo_trans(m_prev_tail, m_tail, m_head_size);
  p_perCycleBitsRemainedHigh += num_hi(m_tail, m_head_size) - num_hi_trans(m_prev_tail, m_tail, m_head_size);
  p_perCycleBitsRemainedLow += m_head_size - num_hi(m_tail, m_head_size) - num_lo_trans(m_prev_tail, m_tail, m_head_size);

  uint32_t i;
  for(i = 0; i < m_size; i++) {  
    if(m_prev_buf[i].valid != m_buf[i].valid)
      cout << "*TRANSITION* m_buf[" << i << "].valid: " << m_prev_buf[i].valid << "->" << m_buf[i].valid << endl;
    p_perCycleBitTransitions += num_trans(m_prev_buf[i].valid, m_buf[i].valid, 1);
    p_perCycleBitTransitionsHigh += num_hi_trans(m_prev_buf[i].valid, m_buf[i].valid, 1);
    p_perCycleBitTransitionsLow += num_lo_trans(m_prev_buf[i].valid, m_buf[i].valid, 1);
    p_perCycleBitsRemainedHigh += num_hi(m_buf[i].valid, 1) - num_hi_trans(m_prev_buf[i].valid, m_buf[i].valid, 1);
    p_perCycleBitsRemainedLow += (uint32_t)1 - num_hi(m_buf[i].valid, 1) - num_lo_trans(m_prev_buf[i].valid, m_buf[i].valid, 1);

    if(m_prev_buf[i].pc != m_buf[i].pc)
      cout << "*TRANSITION* m_buf[" << i << "].pc: " << m_prev_buf[i].pc << "->" << m_buf[i].pc << endl;
    p_perCycleBitTransitions += num_trans(m_prev_buf[i].pc, m_buf[i].pc, 32);
    p_perCycleBitTransitionsHigh += num_hi_trans(m_prev_buf[i].pc, m_buf[i].pc, 32);
    p_perCycleBitTransitionsLow += num_lo_trans(m_prev_buf[i].pc, m_buf[i].pc, 32);
    p_perCycleBitsRemainedHigh += num_hi(m_buf[i].pc, 32) - num_hi_trans(m_prev_buf[i].pc, m_buf[i].pc, 32);
    p_perCycleBitsRemainedLow += (uint32_t)32 - num_hi(m_buf[i].pc, 32) - num_lo_trans(m_prev_buf[i].pc, m_buf[i].pc, 32);

    if(m_prev_buf[i].reg_id != m_buf[i].reg_id)
      cout << "*TRANSITION* m_buf[" << i << "].reg_id: " << m_prev_buf[i].reg_id << "->" << m_buf[i].reg_id << endl;
    p_perCycleBitTransitions += num_trans(m_prev_buf[i].reg_id, m_buf[i].reg_id, 4);
    p_perCycleBitTransitionsHigh += num_hi_trans(m_prev_buf[i].reg_id, m_buf[i].reg_id, 4);
    p_perCycleBitTransitionsLow += num_lo_trans(m_prev_buf[i].reg_id, m_buf[i].reg_id, 4);
    p_perCycleBitsRemainedHigh += num_hi(m_buf[i].reg_id, 4) - num_hi_trans(m_prev_buf[i].reg_id, m_buf[i].reg_id, 4);
    p_perCycleBitsRemainedLow += (uint32_t)4 - num_hi(m_buf[i].reg_id, 4) - num_lo_trans(m_prev_buf[i].reg_id, m_buf[i].reg_id, 4);

    if(m_prev_buf[i].result != m_buf[i].result)
      cout << "*TRANSITION* m_buf[" << i << "].result: " << m_prev_buf[i].result << "->" << m_buf[i].result << endl; 
    p_perCycleBitTransitions += num_trans(m_prev_buf[i].result, m_buf[i].result, 32);
    p_perCycleBitTransitionsHigh += num_hi_trans(m_prev_buf[i].result, m_buf[i].result, 32);
    p_perCycleBitTransitionsLow += num_lo_trans(m_prev_buf[i].result, m_buf[i].result, 32);
    p_perCycleBitsRemainedHigh += num_hi(m_buf[i].result, 32) - num_hi_trans(m_prev_buf[i].result, m_buf[i].result, 32);
    p_perCycleBitsRemainedLow += (uint32_t)32 - num_hi(m_buf[i].result, 32) - num_lo_trans(m_prev_buf[i].result, m_buf[i].result, 32);
  }
}

void ROB_Circ::pre_cycle_power_snapshot () {
  m_prev_head = m_head;
  m_prev_tail = m_tail;
  uint32_t i;
  for(i = 0; i < m_size; i++) {
    m_prev_buf[i].valid = m_buf[i].valid;
    m_prev_buf[i].cycles = m_buf[i].cycles;
    m_prev_buf[i].pc = m_buf[i].pc;
    m_prev_buf[i].reg_id = m_buf[i].reg_id;
    m_prev_buf[i].result = m_buf[i].result;
  }
}

void ROB_Circ::print_power_stats (int cycles) {
  cout << endl;
  cout << "Power statistics from the run:" << endl;
  cout << "Total # bits = num_cycles * # bits in ROB = " << cycles << "*" << p_bit_count << " = " << ((uint32_t)cycles)*p_bit_count << endl;
  cout << "Total # bit transitions to high: " << p_totalBitTransitionsHigh << endl;
  cout << "Total # bit transitions to low: " << p_totalBitTransitionsLow << endl;
  cout << "Total # bits remained low: " << p_totalBitsRemainedLow << endl;
  cout << "Total # bits remained high: " << p_totalBitsRemainedHigh << endl;
  cout << endl;
}


bool ROB_Circ::isinROB( uint16_t reg)
{
  if (!m_empty)
    {
      uint32_t i = m_head;
      do
        {
          p_reg_comp_used++;
          if (m_buf[i].reg_id == reg)
            return true;
          i = (i + 1) % m_size;
        }
      while (i != m_tail);
    }
  return false;
}


