#include <iostream>
#include <math.h>
#include <stdlib.h>

#include "rob_latch.h"

using namespace std;

extern int ins_cyc[];

ROB_Latch::ROB_Latch (int s, int in, int fn, int lsize, int pflags)
  : ROB_Circ (s, in, fn, pflags)
{
  fwdcnt = 0;
  m_lhead = lsize-1 ;
  m_ltail = 0;
  m_lsize = lsize;

  lbuf = new lentry[lsize];
  lbuf_prev = new lentry[lsize];
  Afwd = new bool[s];
  Afwd_prev = new bool[s];
}


ROB_Latch::~ROB_Latch() {
  delete [] lbuf;
  delete [] lbuf_prev;
  delete [] Afwd;
  delete [] Afwd_prev;

}

//chage add to latch
void ROB_Latch::update_entries ()
{
  if (m_empty)
    return;

  uint32_t i = m_head;
  do
    {
      entry_t *entry = get_entry (i);
      if (entry->cycles && --entry->cycles == 0)
        {
//          cout << "Got result " << i << endl;
          entry->result = rand ();
          entry->valid = true;
          m_nbiton = bits_on (true, false);
          addToLatch(entry->reg_id, entry->result, entry->isfp);

          m_nrex++;
        }
      i = head_incr (i);
    }
  while (i != m_tail);
}

//change fwdcnt
// From the head pointer, commit up to m_n instructions to the ARF.
void ROB_Latch::write_to_arf ()
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
              if (Afwd[m_head])
                fwdcnt++;

              if (m_print & DBG_FLAG)
              Afwd[m_head] = false;

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

//changed read from tolatch
// Read instructions from issue, up to m_n instructions.
void ROB_Latch::write_entry (entry *entry, ins_t ins)
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
		uint32_t operandRegA = ((ins.regs >> 4) & reg_mask);
		uint32_t operandRegB = ((ins.regs >> 8) & reg_mask);
		uint32_t extraCyclesA = 0;
		uint32_t extraCyclesB = 0;
		if(ReadinROB (operandRegA)) {
			extraCyclesA = getCyclesToCompletion(operandRegA);
			//cout << "Forwarding ins.pc=" << ins.pc << " operandA in " << extraCyclesA << " cycles" << endl;
			//m_nwdu++;
		}
	if(ReadinROB (operandRegB)) {
		extraCyclesB = getCyclesToCompletion(operandRegB);
		//cout << "Forwarding ins.pc=" << ins.pc << " operandB in " << extraCyclesB << " cycles" << endl;
		//m_nwdu++;
	}
	uint32_t maxExtraCycles = max(extraCyclesA, extraCyclesB);
	if(maxExtraCycles)
		//cout << "Increasing ins.pc=" << ins.pc << " cycle count by " << maxExtraCycles << " to allow forwarding" << endl;
		entry->cycles += maxExtraCycles;
		}






void ROB_Latch::post_cycle_power_tabulation() {
  ROB_Circ::post_cycle_power_tabulation();

  uint32_t i;
  for(i = 0; i < m_size; i++) {
    p_perCycleBitTransitions += num_trans(Afwd_prev[i], Afwd[i], 1);
    p_perCycleBitTransitionsHigh += num_hi_trans(Afwd_prev[i], Afwd[i], 1);
    p_perCycleBitTransitionsLow += num_lo_trans(Afwd_prev[i],Afwd[i], 1);
    p_perCycleBitsRemainedHigh += num_hi(Afwd[i], 1) - num_hi_trans(Afwd_prev[i], Afwd[i], 1);
    p_perCycleBitsRemainedLow += (uint32_t)1 - num_hi(Afwd[i], 1) - num_lo_trans(Afwd_prev[i], Afwd[i], 1);
  }


  for(i = 0; i < m_lsize; i++) {
    p_perCycleBitTransitions += num_trans(lbuf_prev[i].data, lbuf[i].data, 32);
    p_perCycleBitTransitionsHigh += num_hi_trans(lbuf_prev[i].data, lbuf[i].data, 32);
    p_perCycleBitTransitionsLow += num_lo_trans(lbuf_prev[i].data,lbuf[i].data, 32);
    p_perCycleBitsRemainedHigh += num_hi(lbuf[i].data, 32) - num_hi_trans(lbuf_prev[i].data, lbuf[i].data, 32);
    p_perCycleBitsRemainedLow += (uint32_t)32 - num_hi(lbuf[i].data, 32) - num_lo_trans(lbuf_prev[i].data, lbuf[i].data, 32);

    p_perCycleBitTransitions += num_trans(lbuf_prev[i].reg_id, lbuf[i].reg_id, 4);
    p_perCycleBitTransitionsHigh += num_hi_trans(lbuf_prev[i].reg_id, lbuf[i].reg_id, 4);
    p_perCycleBitTransitionsLow += num_lo_trans(lbuf_prev[i].reg_id, lbuf[i].reg_id, 4);
    p_perCycleBitsRemainedHigh += num_hi(lbuf[i].reg_id, 4) - num_hi_trans(lbuf_prev[i].reg_id, lbuf[i].reg_id, 4);
    p_perCycleBitsRemainedLow += (uint32_t)4 - num_hi(lbuf[i].reg_id, 4) - num_lo_trans(lbuf_prev[i].reg_id, lbuf[i].reg_id, 4);

    p_perCycleBitTransitions += num_trans(lbuf_prev[i].isfp, lbuf[i].isfp, 1);
    p_perCycleBitTransitionsHigh += num_hi_trans(lbuf_prev[i].isfp, lbuf[i].isfp, 1);
    p_perCycleBitTransitionsLow += num_lo_trans(lbuf_prev[i].isfp, lbuf[i].isfp, 1);
    p_perCycleBitsRemainedHigh += num_hi(lbuf[i].isfp, 1) - num_hi_trans(lbuf_prev[i].isfp, lbuf[i].isfp, 1);
    p_perCycleBitsRemainedLow += (uint32_t)1 - num_hi(lbuf[i].isfp, 1) - num_lo_trans(lbuf_prev[i].isfp, lbuf[i].isfp, 1);

  }
}

//TODO CHANGE
void ROB_Latch::pre_cycle_power_snapshot() {
  ROB_Circ::pre_cycle_power_snapshot();
  for (uint32_t i = 0; i<m_lsize; i++)
    {
      lbuf_prev[i].data = lbuf[i].data;
      lbuf_prev[i].reg_id = lbuf[i].reg_id;
      lbuf_prev[i].isfp = lbuf[i].isfp;
    }

  for (uint32_t i = 0; i<m_size; i++)
    {
      Afwd_prev[i] = Afwd[i];
    }

/*
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
*/
}

bool ROB_Latch::ReadinROB(uint16_t reg)
{

  if (!m_empty)
    {
      uint32_t i = m_head;
      do
        {
          p_reg_comp_used++;
          if ( (m_buf[i].reg_id == reg)  && ! ReadinLatch(reg)  )
		  {
            Afwd[i] = true;
		   return true;
		  }  
		i = (i + 1) % m_size;
        }
      while (i != m_tail);
    }
	return false;
}


bool ROB_Latch::ReadinLatch(uint16_t reg)
{
  if (m_lhead!= m_ltail )
    {
      uint32_t i = m_lhead;
      do
        {
          p_reg_comp_used++;
          if (lbuf[i].reg_id == reg )
          {
            if (lbuf[i].isfp)
              m_nwdu++;
            m_nwdu++;
            return true;
          }          
          i = (i + 1) % m_lsize;
        }
      while (i != m_ltail);
    }
    return false;
}

void ROB_Latch::addToLatch(uint16_t reg, uint32_t data, bool fp)
{

  lbuf[m_ltail].data = data;
  lbuf[m_ltail].reg_id = reg;
  lbuf[m_ltail].isfp = fp;

  m_ltail = ( m_ltail+1)%m_lsize;
  m_lhead = ( m_lhead+1)%m_lsize;	

}



void ROB_Latch::run (ins_t ins[])
{
  cout << "In Latch class run method: " << endl;

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
      if (m_print & DBG_FLAG)
        print_msgs (cycles);


      // Break when we've written all the instructions.
      if (m_empty && ins[ins_num].type == -1)
        break;
    }

  print_power_stats(cycles);

//  cout << "Simulation complete." << endl;
}

void ROB_Latch::print_power_stats (int cycles)
{
	ROB_Circ::print_power_stats(cycles);
	cout <<"Total # times reforwarding is done: " << fwdcnt << endl<<endl ;
	cout << "*NOTE: Written to DU count is from latches to the DU"<<endl<<endl;
	
}



uint32_t ROB_Latch::getCyclesToCompletion(uint32_t reg) {
	uint32_t i = m_head;
	if (!m_empty)
		do {
			entry_t *entry = get_entry(i);
			if(entry->reg_id == reg)
				return entry->cycles;
			i = ((i+1)%m_size);
		} while(i != m_tail);	
	return 0;
}
