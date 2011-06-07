#include <iostream>

#include "rob.h"
using namespace std;

extern int ins_cyc[];

ROB::ROB (int s, int in, int fn, int pflags)
  : m_size(s)
  , m_in(in)
  , m_fn(fn)
  , m_print(pflags)
{
  m_n = m_in + m_fn;
  m_fp_delay = 0;
  m_buf = new entry_t [m_size];
  m_prev_buf = new entry_t [m_size];
  entry_t empty_entry = {false, 0, 0, 0, 0, false};
  for (uint32_t i = 0; i < m_size; i++)
    {
      m_buf[i] = empty_entry;
      m_prev_buf[i] = empty_entry;
    }

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
  p_bit_count = (ENTRY_BIT_COUNT*m_size); // 70 = PC[32] + RESULT[32] + REG_ID[4] + VALID[1] + ISFP[1]
}

entry_t *ROB::get_entry (uint32_t ptr)
{
  return &m_buf[ptr];
}

entry_t *ROB::get_prev_buf_entry (uint32_t ptr)
{
  return &m_prev_buf[ptr];
}

void ROB::post_cycle_power_tabulation () {
uint32_t i;
  for(i = 0; i < m_size; i++) {
    entry_t *entry = get_entry (i);
    entry_t *prev_ent = get_prev_buf_entry (i);

    if ((m_print & DBG_FLAG) && prev_ent->valid != entry->valid)
      cout << "*TRANSITION* m_buf[" << i << "].valid: " << prev_ent->valid << "->" << entry->valid << endl;
    p_perCycleBitTransitions += num_trans(prev_ent->valid, entry->valid, 1);
    p_perCycleBitTransitionsHigh += num_hi_trans(prev_ent->valid, entry->valid, 1);
    p_perCycleBitTransitionsLow += num_lo_trans(prev_ent->valid, entry->valid, 1);
    p_perCycleBitsRemainedHigh += num_hi(entry->valid, 1) - num_hi_trans(prev_ent->valid, entry->valid, 1);
    p_perCycleBitsRemainedLow += (uint32_t)1 - num_hi(entry->valid, 1) - num_lo_trans(prev_ent->valid, entry->valid, 1);

    if ((m_print & DBG_FLAG) && prev_ent->pc != entry->pc)
      cout << "*TRANSITION* m_buf[" << i << "].pc: " << prev_ent->pc << "->" << entry->pc << endl;
    p_perCycleBitTransitions += num_trans(prev_ent->pc, entry->pc, 32);
    p_perCycleBitTransitionsHigh += num_hi_trans(prev_ent->pc, entry->pc, 32);
    p_perCycleBitTransitionsLow += num_lo_trans(prev_ent->pc, entry->pc, 32);
    p_perCycleBitsRemainedHigh += num_hi(entry->pc, 32) - num_hi_trans(prev_ent->pc, entry->pc, 32);
    p_perCycleBitsRemainedLow += (uint32_t)32 - num_hi(entry->pc, 32) - num_lo_trans(prev_ent->pc, entry->pc, 32);

    if ((m_print & DBG_FLAG) && prev_ent->reg_id != entry->reg_id)
      cout << "*TRANSITION* m_buf[" << i << "].reg_id: " << prev_ent->reg_id << "->" << entry->reg_id << endl;
    p_perCycleBitTransitions += num_trans(prev_ent->reg_id, entry->reg_id, 4);
    p_perCycleBitTransitionsHigh += num_hi_trans(prev_ent->reg_id, entry->reg_id, 4);
    p_perCycleBitTransitionsLow += num_lo_trans(prev_ent->reg_id, entry->reg_id, 4);
    p_perCycleBitsRemainedHigh += num_hi(entry->reg_id, 4) - num_hi_trans(prev_ent->reg_id, entry->reg_id, 4);
    p_perCycleBitsRemainedLow += (uint32_t)4 - num_hi(entry->reg_id, 4) - num_lo_trans(prev_ent->reg_id, entry->reg_id, 4);

    if ((m_print & DBG_FLAG) && prev_ent->result != entry->result)
      cout << "*TRANSITION* m_buf[" << i << "].result: " << prev_ent->result << "->" << entry->result << endl;
    p_perCycleBitTransitions += num_trans(prev_ent->result, entry->result, 32);
    p_perCycleBitTransitionsHigh += num_hi_trans(prev_ent->result, entry->result, 32);
    p_perCycleBitTransitionsLow += num_lo_trans(prev_ent->result, entry->result, 32);
    p_perCycleBitsRemainedHigh += num_hi(entry->result, 32) - num_hi_trans(prev_ent->result, entry->result, 32);
    p_perCycleBitsRemainedLow += (uint32_t)32 - num_hi(entry->result, 32) - num_lo_trans(prev_ent->result, entry->result, 32);

    if((m_print & DBG_FLAG) && prev_ent->isfp != entry->isfp)
      cout << "*TRANSITION* m_buf[" << i << "].isfp: " << prev_ent->isfp << "->" << entry->isfp << endl;
    p_perCycleBitTransitions += num_trans(prev_ent->isfp, entry->isfp, 1);
    p_perCycleBitTransitionsHigh += num_hi_trans(prev_ent->isfp, entry->isfp, 1);
    p_perCycleBitTransitionsLow += num_lo_trans(prev_ent->isfp, entry->isfp, 1);
    p_perCycleBitsRemainedHigh += num_hi(entry->isfp, 1) - num_hi_trans(prev_ent->isfp, entry->isfp, 1);
    p_perCycleBitsRemainedLow += (uint32_t)1 - num_hi(entry->isfp, 1) - num_lo_trans(prev_ent->isfp, entry->isfp, 1);
  }
}

void ROB::update_power_totals() {
  p_totalBitTransitions += p_perCycleBitTransitions;
  p_totalBitTransitionsHigh += p_perCycleBitTransitionsHigh;
  p_totalBitTransitionsLow += p_perCycleBitTransitionsLow;
  p_totalBitsRemainedLow += p_perCycleBitsRemainedLow;
  p_totalBitsRemainedHigh += p_perCycleBitsRemainedHigh;
}

void ROB::pre_cycle_power_snapshot() {
  uint32_t i;
  for(i = 0; i < m_size; i++) {
    entry_t *entry = get_entry (i);
    entry_t *prev_ent = get_prev_buf_entry (i);
    prev_ent->valid = entry->valid;
    prev_ent->cycles = entry->cycles;
    prev_ent->pc = entry->pc;
    prev_ent->reg_id = entry->reg_id;
    prev_ent->result = entry->result;
    prev_ent->isfp = entry->isfp;
  }

  p_perCycleBitTransitions = 0;
  p_perCycleBitTransitionsHigh = 0;
  p_perCycleBitTransitionsLow = 0;
  p_perCycleBitsRemainedLow = 0;
  p_perCycleBitsRemainedHigh = 0;

}

void ROB::print_power_stats (int cycles) {
  cout << endl;
  cout << "Power statistics from the run: (" << cycles << " cycles)" << endl;
  cout << "Total # bits = num_cycles * # bits in ROB = " << cycles << "*" << p_bit_count << " = " << ((uint32_t)cycles)*p_bit_count << endl;
  cout << "Total # bit transitions to high: " << p_totalBitTransitionsHigh << endl;
  cout << "Total # bit transitions to low: " << p_totalBitTransitionsLow << endl;
  cout << "Total # bits remained low: " << p_totalBitsRemainedLow << endl;
  cout << "Total # bits remained high: " << p_totalBitsRemainedHigh << endl;
  cout << "Total # times reg comparator is used: "<< p_reg_comp_used << endl << endl;
	
  cout << "Total # times written to DU: " << m_nwdu << endl;
  cout << "Total # times written to ARF: " << m_nwarf << endl;

  cout << "Total # times read from IQ: " << m_nriq << endl ;
  cout << "Total # times read from EX: " << m_nrex << endl ;
  
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
