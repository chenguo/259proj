#include <iostream>

#include "rob_circ.h"
using namespace std;

extern int ins_cyc[];

ROB_Circ::ROB_Circ (int s, int in, int fn) : ROB (s, in, fn)
{
  m_head = 0;
  m_tail = 0;
  m_empty = true;
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
      p_perCycleBitsLow = 0;
      p_perCycleBitsHigh = 0;

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

      cout << "# Bit Transitions: " << p_perCycleBitTransitions << endl;
      cout << "# Bit Transitions to High: " << p_perCycleBitTransitionsHigh << endl;
      cout << "# Bit Transitions to Low: " << p_perCycleBitTransitionsLow << endl;
      cout << "# Bits: " << (69*m_size + 2*32) << endl;
      cout << "# Bits Low: " << p_perCycleBitsLow << endl;
      cout << "# Bits High: " << p_perCycleBitsHigh << endl << endl;

      p_totalBitTransitions += p_perCycleBitTransitions;
      p_totalBitTransitionsHigh += p_perCycleBitTransitionsHigh;
      p_totalBitTransitionsLow += p_perCycleBitTransitionsLow;
      p_totalBitsLow += p_perCycleBitsLow;
      p_totalBitsHigh += p_perCycleBitsHigh;

      // Break when we've written all the instructions.
      if (m_empty && ins[ins_num].type == -1)
        break;
    }

  print_power_stats();

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
  bool skipped = false;
  int exec_ins_num = ins_num;
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

					//this ins has already been but into the rob
					if(ins[exec_ins_num].exec)
					{
						exec_ins_num++;
						if(!skipped)
							ins_num++;
						continue;
					}
					//fp ins
          if (ins[exec_ins_num].type>=FADD)
          {
						//2slots left
						if( ((m_tail+1)%m_size)!= m_head)
						{
							 cout<<"FP ins processed"<<endl;
							 //first entry
         			 m_buf[m_tail].valid = false;
         			 m_buf[m_tail].cycles = ins_cyc[ins[exec_ins_num].type];
      				 m_buf[m_tail].pc = ins[exec_ins_num].pc;
          		 m_buf[m_tail].reg_id = ins[exec_ins_num].regs&0xFF;
							 m_buf[m_tail].isfp = true;

               m_tail = (m_tail + 1) % m_size;
								cout<<"end of first 1/2"<<endl;
							 //will add second entry below
						}
						else{
							cout<<"no space for a FP.. should skip to next ins"<<endl;
							skipped = true;
						}
					}
					//see below
//					entry_t old_entry = m_buf[m_tail];
          m_buf[m_tail].valid = false;
          m_buf[m_tail].cycles = ins_cyc[ins[exec_ins_num].type];
          m_buf[m_tail].pc = ins[exec_ins_num].pc;
          m_buf[m_tail].reg_id = ins[exec_ins_num].regs&0xFF;
				  m_buf[m_tail].isfp=false;
					ins[exec_ins_num].exec = true;
					
    			cout <<"end of second"<<endl;
          m_tail = (m_tail + 1) % m_size;

					if(!skipped)
          	ins_num++;
					exec_ins_num++;
          nread++;

					cout<<"exe_ins_num is "<<exec_ins_num<<endl;
          // Count bits turned on.
					//TODO: we probably dont need this cause alex is
					//check pointing the states per cycle
//          m_nbiton += bits_on (m_buf[m_tail].pc, old_entry.pc);
//          m_nbiton += bits_on (m_buf[m_tail].reg_id, old_entry.reg_id);
        }
      while (m_tail != old_head && nread < m_n && ins[exec_ins_num].type != -1);

      m_empty = false;
      m_nriq += nread;
    }
  // Process tail pointer bit flips.
  m_nbiton += bits_on (m_tail, m_tail - nread);
  return ins_num;
}

void ROB_Circ::post_cycle_power_tabulation () {
  p_perCycleBitTransitions += num_trans(m_prev_head, m_head, 32);
  p_perCycleBitTransitionsHigh += num_hi_trans(m_prev_head, m_head, 32);
  p_perCycleBitTransitionsLow += num_lo_trans(m_prev_head, m_head, 32);
  p_perCycleBitsHigh += num_hi(m_head,32);
  p_perCycleBitsLow += (uint32_t)32 - num_hi(m_head,32);

  p_perCycleBitTransitions += num_trans(m_prev_tail, m_tail, 32);
  p_perCycleBitTransitionsHigh += num_hi_trans(m_prev_tail, m_tail, 32);
  p_perCycleBitTransitionsLow += num_lo_trans(m_prev_tail, m_tail, 32);
  p_perCycleBitsHigh += num_hi(m_tail, 32);
  p_perCycleBitsLow += (uint32_t)32 - num_hi(m_tail, 32);

  uint32_t i;
  for(i = 0; i < m_size; i++) {  
    p_perCycleBitTransitions += num_trans(m_prev_buf[i].valid, m_buf[i].valid, 1);
    p_perCycleBitTransitionsHigh += num_hi_trans(m_prev_buf[i].valid, m_buf[i].valid, 1);
    p_perCycleBitTransitionsLow += num_lo_trans(m_prev_buf[i].valid, m_buf[i].valid, 1);
    p_perCycleBitsHigh += num_hi(m_buf[i].valid, 1);
    p_perCycleBitsLow += (uint32_t)1 - num_hi(m_buf[i].valid, 1);

    p_perCycleBitTransitions += num_trans(m_prev_buf[i].pc, m_buf[i].pc, 32);
    p_perCycleBitTransitionsHigh += num_hi_trans(m_prev_buf[i].pc, m_buf[i].pc, 32);
    p_perCycleBitTransitionsLow += num_lo_trans(m_prev_buf[i].pc, m_buf[i].pc, 32);
    p_perCycleBitsHigh += num_hi(m_buf[i].pc, 32);
    p_perCycleBitsLow += (uint32_t)32 - num_hi(m_buf[i].pc, 32);

    p_perCycleBitTransitions += num_trans(m_prev_buf[i].reg_id, m_buf[i].reg_id, 4);
    p_perCycleBitTransitionsHigh += num_hi_trans(m_prev_buf[i].reg_id, m_buf[i].reg_id, 4);
    p_perCycleBitTransitionsLow += num_lo_trans(m_prev_buf[i].reg_id, m_buf[i].reg_id, 4);
    p_perCycleBitsHigh += num_hi(m_buf[i].reg_id, 4);
    p_perCycleBitsLow += (uint32_t)4 - num_hi(m_buf[i].reg_id, 4);

    p_perCycleBitTransitions += num_trans(m_prev_buf[i].result, m_buf[i].result, 32);
    p_perCycleBitTransitionsHigh += num_hi_trans(m_prev_buf[i].result, m_buf[i].result, 32);
    p_perCycleBitTransitionsLow += num_lo_trans(m_prev_buf[i].result, m_buf[i].result, 32);
    p_perCycleBitsHigh += num_hi(m_buf[i].result, 32);
    p_perCycleBitsLow += (uint32_t)32 - num_hi(m_buf[i].result, 32);
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

void ROB_Circ::print_power_stats () {
  cout << endl;
  cout << "Power statistics from the run:" << endl;
  cout << "Total # bit transitions: " << p_totalBitTransitions << endl;
  cout << "Total # bit transitions to high: " << p_totalBitTransitionsHigh << endl;
  cout << "Total # bit transitions to low: " << p_totalBitTransitionsLow << endl;
  cout << "Total # bits low: " << p_totalBitsLow << endl;
  cout << "Total # bits high: " << p_totalBitsHigh << endl;
  cout << endl;
}


