#include <iostream>

#include "rob.h"
using namespace std;

extern int ins_cyc[];

ROB::ROB (int s, int in, int fn)
  : m_size(s)
  , m_in(in)
  , m_fn(fn)
{
  m_n = m_in + m_fn;
  m_buf = new entry_t [m_size];
  m_prev_buf = new entry_t [m_size];
  FROM_IQ = PC_SIZE;
  FROM_EX = DATA_SIZE;
  TO_DU = DATA_SIZE;
  TO_ARF = DATA_SIZE;

  p_totalBitTransitions = 0;
  p_totalBitTransitionsHigh = 0;
  p_totalBitTransitionsLow = 0;
  p_totalBitsRemainedHigh = 0;
  p_totalBitsRemainedLow = 0;
  p_reg_comp_used = 0;
  p_bit_count = (70*m_size); // 70 = PC[32] + RESULT[32] + REG_ID[4] + VALID[1] + ISFP[1]
}

void ROB::default_post_cycle_power_tabulation() {
uint32_t i;
  for(i = 0; i < m_size; i++) {
#ifdef P_DEBUG
    if(m_prev_buf[i].valid != m_buf[i].valid)
      cout << "*TRANSITION* m_buf[" << i << "].valid: " << m_prev_buf[i].valid << "->" << m_buf[i].valid << endl;
#endif
    p_perCycleBitTransitions += num_trans(m_prev_buf[i].valid, m_buf[i].valid, 1);
    p_perCycleBitTransitionsHigh += num_hi_trans(m_prev_buf[i].valid, m_buf[i].valid, 1);
    p_perCycleBitTransitionsLow += num_lo_trans(m_prev_buf[i].valid, m_buf[i].valid, 1);
    p_perCycleBitsRemainedHigh += num_hi(m_buf[i].valid, 1) - num_hi_trans(m_prev_buf[i].valid, m_buf[i].valid, 1);
    p_perCycleBitsRemainedLow += (uint32_t)1 - num_hi(m_buf[i].valid, 1) - num_lo_trans(m_prev_buf[i].valid, m_buf[i].valid, 1);

#ifdef P_DEBUG
    if(m_prev_buf[i].pc != m_buf[i].pc)
      cout << "*TRANSITION* m_buf[" << i << "].pc: " << m_prev_buf[i].pc << "->" << m_buf[i].pc << endl;
#endif
    p_perCycleBitTransitions += num_trans(m_prev_buf[i].pc, m_buf[i].pc, 32);
    p_perCycleBitTransitionsHigh += num_hi_trans(m_prev_buf[i].pc, m_buf[i].pc, 32);
    p_perCycleBitTransitionsLow += num_lo_trans(m_prev_buf[i].pc, m_buf[i].pc, 32);
    p_perCycleBitsRemainedHigh += num_hi(m_buf[i].pc, 32) - num_hi_trans(m_prev_buf[i].pc, m_buf[i].pc, 32);
    p_perCycleBitsRemainedLow += (uint32_t)32 - num_hi(m_buf[i].pc, 32) - num_lo_trans(m_prev_buf[i].pc, m_buf[i].pc, 32);

#ifdef P_DEBUG
    if(m_prev_buf[i].reg_id != m_buf[i].reg_id)
      cout << "*TRANSITION* m_buf[" << i << "].reg_id: " << m_prev_buf[i].reg_id << "->" << m_buf[i].reg_id << endl;
#endif
    p_perCycleBitTransitions += num_trans(m_prev_buf[i].reg_id, m_buf[i].reg_id, 4);
    p_perCycleBitTransitionsHigh += num_hi_trans(m_prev_buf[i].reg_id, m_buf[i].reg_id, 4);
    p_perCycleBitTransitionsLow += num_lo_trans(m_prev_buf[i].reg_id, m_buf[i].reg_id, 4);
    p_perCycleBitsRemainedHigh += num_hi(m_buf[i].reg_id, 4) - num_hi_trans(m_prev_buf[i].reg_id, m_buf[i].reg_id, 4);
    p_perCycleBitsRemainedLow += (uint32_t)4 - num_hi(m_buf[i].reg_id, 4) - num_lo_trans(m_prev_buf[i].reg_id, m_buf[i].reg_id, 4);

#ifdef P_DEBUG
    if(m_prev_buf[i].result != m_buf[i].result)
      cout << "*TRANSITION* m_buf[" << i << "].result: " << m_prev_buf[i].result << "->" << m_buf[i].result << endl;
#endif
    p_perCycleBitTransitions += num_trans(m_prev_buf[i].result, m_buf[i].result, 32);
    p_perCycleBitTransitionsHigh += num_hi_trans(m_prev_buf[i].result, m_buf[i].result, 32);
    p_perCycleBitTransitionsLow += num_lo_trans(m_prev_buf[i].result, m_buf[i].result, 32);
    p_perCycleBitsRemainedHigh += num_hi(m_buf[i].result, 32) - num_hi_trans(m_prev_buf[i].result, m_buf[i].result, 32);
    p_perCycleBitsRemainedLow += (uint32_t)32 - num_hi(m_buf[i].result, 32) - num_lo_trans(m_prev_buf[i].result, m_buf[i].result, 32);

#ifdef P_DEBUG
    if(m_prev_buf[i].isfp != m_buf[i].isfp)
      cout << "*TRANSITION* m_buf[" << i << "].isfp: " << m_prev_buf[i].isfp << "->" << m_buf[i].isfp << endl;
#endif
    p_perCycleBitTransitions += num_trans(m_prev_buf[i].isfp, m_buf[i].isfp, 1);
    p_perCycleBitTransitionsHigh += num_hi_trans(m_prev_buf[i].isfp, m_buf[i].isfp, 1);
    p_perCycleBitTransitionsLow += num_lo_trans(m_prev_buf[i].isfp, m_buf[i].isfp, 1);
    p_perCycleBitsRemainedHigh += num_hi(m_buf[i].isfp, 1) - num_hi_trans(m_prev_buf[i].isfp, m_buf[i].isfp, 1);
    p_perCycleBitsRemainedLow += (uint32_t)1 - num_hi(m_buf[i].isfp, 1) - num_lo_trans(m_prev_buf[i].isfp, m_buf[i].isfp, 1);
  }
}

void ROB::default_update_power_totals() {
  p_totalBitTransitions += p_perCycleBitTransitions;
  p_totalBitTransitionsHigh += p_perCycleBitTransitionsHigh;
  p_totalBitTransitionsLow += p_perCycleBitTransitionsLow;
  p_totalBitsRemainedLow += p_perCycleBitsRemainedLow;
  p_totalBitsRemainedHigh += p_perCycleBitsRemainedHigh;
}

void ROB::default_pre_cycle_power_snapshot() {
  uint32_t i;
  for(i = 0; i < m_size; i++) {
    m_prev_buf[i].valid = m_buf[i].valid;
    m_prev_buf[i].cycles = m_buf[i].cycles;
    m_prev_buf[i].pc = m_buf[i].pc;
    m_prev_buf[i].reg_id = m_buf[i].reg_id;
    m_prev_buf[i].result = m_buf[i].result;
    m_prev_buf[i].isfp = m_buf[i].isfp;
  }
}

void ROB::default_print_power_stats(int cycles) {
  cout << endl;
  cout << "Power statistics from the run:" << endl;
  cout << "Total # bits = num_cycles * # bits in ROB = " << cycles << "*" << p_bit_count << " = " << ((uint32_t)cycles)*p_bit_count << endl;
  cout << "Total # bit transitions to high: " << p_totalBitTransitionsHigh << endl;
  cout << "Total # bit transitions to low: " << p_totalBitTransitionsLow << endl;
  cout << "Total # bits remained low: " << p_totalBitsRemainedLow << endl;
  cout << "Total # bits remained high: " << p_totalBitsRemainedHigh << endl;
  cout << endl;
}


ROB::~ROB ()
{
  delete [] m_buf;
  delete [] m_prev_buf;
}

void ROB::run (ins_t instructions[])
{
  cout << "In base class run method: " << endl;
}
