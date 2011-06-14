#include <iostream>
#include <math.h>
#include <stdlib.h>

#include "rob_dist.h"
using namespace std;

extern int ins_cyc[];

ROB_Dist::ROB_Dist (int s, int c, int in, int fn, int pflags) : ROB (s, in, fn, pflags)
{
  p_bit_count = 0;
  m_cluster_size = m_size/c;
  m_cluster_count = c;
  m_cc_bits = (uint32_t)(ceil(log(c)/log(2)));
  m_buf_array = new entry_t *[m_cluster_count];
  m_prev_buf_array = new entry_t *[m_cluster_count];

  m_head = new uint32_t [m_cluster_count];
  m_tail = new uint32_t [m_cluster_count];
  m_prev_head = new uint32_t [m_cluster_count];
  m_prev_tail = new uint32_t [m_cluster_count];
  m_empty = new bool [m_cluster_count];
  m_prev_empty = new bool [m_cluster_count];

  for(uint32_t i = 0; i < m_cluster_count; i++) {
    m_buf_array[i] = new entry_t [m_cluster_size];
    m_prev_buf_array[i] = new entry_t [m_cluster_size];
    m_head[i] = 0;
    m_tail[i] = 0;
    m_prev_head[i] = 0;
    m_prev_tail[i] = 0;
    m_empty[i] = true;
  }

  m_head_size = (uint32_t)(ceil(log(m_cluster_size) / log(2)));
  p_bit_count += (m_cluster_count*(m_cluster_size*ENTRY_BIT_COUNT));
  p_bit_count += (m_cluster_count*(2*m_head_size));

  central_fifo = new fifo_entry_t [m_size];
  prev_central_fifo = new fifo_entry_t [m_size];
  fifo_head = 0;
  fifo_tail = 0;
  fifo_prev_head = 0;
  fifo_prev_tail = 0;
  fifo_empty = true;
  fifo_head_size = (uint32_t)(ceil(log(m_size)/log(2)));
  fifo_entry_bit_count = (uint32_t)((1+32+4)+m_cc_bits);

  p_bit_count += (2*(fifo_head_size));
  p_bit_count += (m_size*fifo_entry_bit_count);

  cout << "New ROB_Dist:" << endl;
  cout << "Width: " << m_n << endl;
  cout << "# FIFO Queue entries: " << m_size << endl;
  cout << "FIFO entry size: " << fifo_entry_bit_count << " bits" << endl;
  cout << "FIFO head and tail pointer bit length (2 in the fifo): " << fifo_head_size << endl;
  cout << "# Clusters: " << m_cluster_count << endl;
  cout << "# ROB_ID bits: " << m_cc_bits << endl;
  cout << "# Entries per cluster: " << m_cluster_size << endl;
  cout << "Cluster entry size: " << ENTRY_BIT_COUNT << " bits" << endl;
  cout << "Cluster head and tail pointer bit length (2 per cluster): " << m_head_size << endl;
  cout << "Total bits in ROB: " << p_bit_count << endl;
}


ROB_Dist::~ROB_Dist() {
  delete[] central_fifo;
  delete[] prev_central_fifo;
  for(uint32_t clusterId = 0; clusterId < m_cluster_count; clusterId++) {
    delete[] m_buf_array[clusterId];
    delete[] m_prev_buf_array[clusterId];
  }
  delete[] m_buf_array;
  delete[] m_prev_buf_array;
  delete[] m_head;
  delete[] m_tail;
  delete[] m_prev_head;
  delete[] m_prev_tail;
  delete[] m_empty;
  delete[] m_prev_empty;
}

void ROB_Dist::run (ins_t ins[])
{
  cout << "In distributed class run method: " << endl;

  int cycles = 0;
  int ins_num = 0;

  // NOTE: During each cycle, everything happens in parallel. So in this loop,
  // be careful of how variable changes may affect this.
  while (1)
    {
      dist_pre_cycle_power_snapshot();

      update_entries ();
      write_to_arf ();
      ins_num = read_from_iq (ins_num, ins);

      // TODO: Count the read ports being driven for operands.
      // Get percentage this happens from Henry.

      dist_post_cycle_power_tabulation();
      update_power_totals();

      cycles++;
      if (m_print & DBG_FLAG)
        print_msgs (cycles);

      // Break when we've written all the instructions.
      if (fifo_empty && ins[ins_num].type == -1)
        break;
    }

  print_power_stats(cycles);

  cout << "Simulation complete." << endl;
}

// For each instruction between the head and tail buffers, decrement their
// cycles to completion count. When it reaches 0, drive the FROM_EX port.
void ROB_Dist::update_entries ()
{
  if (fifo_empty) 
    return;

  uint32_t i = fifo_head;
  do {
    fifo_entry_t *fifo_entry = return_fifo_entry (i);
    entry_t *entry = get_entry(fifo_entry->rob_id, fifo_entry->rob_index);
    if (m_print & DBG_FLAG)
      cout << "Decrementing central_fifo[" << i << "]: " << entry->cycles << "->" << entry->cycles-1 << endl;
    if (entry->cycles > 0 && --entry->cycles == 0) {
      if (m_print & DBG_FLAG)
        cout << "Got result for central_fifo[" << i << "] -> rob[" << fifo_entry->rob_id << "][" << fifo_entry->rob_index << "]" << endl;
      entry->valid = true;
      fifo_entry->valid = true;
      m_nrex++;
    }
    i = fifo_ptr_incr (i);
  }  while (i != fifo_tail);
}

// From the head pointer, commit up to m_n instructions to the ARF.
void ROB_Dist::write_to_arf ()
{
  if (fifo_empty) 
    return;

  uint32_t m = m_n;
  uint32_t nwritten = 0;
  bool isFirstFp = true;

  // Loop through entries. Stopping conditions:
  // 1. m_head == m_tail: buffer is empty
  // 2. nwritten == m_n: all commit ports used
  // 3. m_buf[m_head].valid == false: invalid entry
  fifo_entry_t *fifo_entry = return_fifo_entry(fifo_head);
  while((fifo_entry->valid && nwritten < m) && !fifo_empty) {
      // Get entry and check it for validity. If valid, commit it.
    entry_t *entry = get_entry(fifo_entry->rob_id, fifo_entry->rob_index);
    if (entry->isfp) {
      if(isFirstFp) {
        m++;
        //cout << "fp";
        isFirstFp = false;
      } else {
        isFirstFp = true;
      }
    }
    if (m_print & DBG_FLAG)
      cout << "Writing central_fifo[" << fifo_head << "] -> rob[" << fifo_entry->rob_id << "][" << fifo_entry->rob_index << "] to ARF" << endl;
    m_head[fifo_entry->rob_id] = ptr_incr (m_head[fifo_entry->rob_id]);
    if(m_head[fifo_entry->rob_id] == m_tail[fifo_entry->rob_id])
      m_empty[fifo_entry->rob_id] = true;
    fifo_head = fifo_ptr_incr(fifo_head);
    fifo_entry = return_fifo_entry(fifo_head);
    if(fifo_head == fifo_tail)
      fifo_empty = true;
    nwritten++;
  }
  // Update writes to ARF count.
  m_nwarf += nwritten;
}

// Read instructions from issue, up to m_n instructions.
int ROB_Dist::read_from_iq (int ins_num, ins_t ins[])
{
  uint32_t nread = 0;
  uint32_t nentries = 0;
  uint32_t f_issued = 0;

  // Read instructions from issue, if
  // 1. Buffer is empty
  // 2. Buffer is not empty, but head != tail
  //    (buffer is full if !empty and head == tail)
  // 3. Instruction sequence isn't over (denoted by -1 type)
  uint16_t mask=0xF;
  int clusterId = ((ins[ins_num].regs)&mask)%m_cluster_count;
  while ((m_empty[clusterId] || (m_head[clusterId] != m_tail[clusterId]))
         && (ins[ins_num].type != -1) && (nentries < m_n)) {
    if (m_print & DBG_FLAG)
      cout << "Read ins[" << ins_num << "] into rob[" << clusterId << "][" << m_tail[clusterId] << "]" << endl;

    // Write entry.
    // FP instruction uses 2 slots.
    if (ins[ins_num].type >= FADD) {
      if (ptr_incr (m_tail[clusterId]) != m_head[clusterId]) {
        if (m_print & DBG_FLAG)
          cout << "FP ins processed" << endl;
        // First half of entry.
        write_entry (clusterId, ins[ins_num]);
        nentries++;
        f_issued++;
      } else if (m_print & DBG_FLAG) {
        cout << "No space for a FP, stall." << endl;
        break;
      }
    }

    write_entry (clusterId, ins[ins_num]);
    nentries++;

    nread++;
    ins_num++;
    clusterId  = ((ins[ins_num].regs)&mask)%m_cluster_count;
  }

  if(m_tail[clusterId] == m_head[clusterId] && m_print & DBG_FLAG)
    cout << "Stopped reading due to full rob[" << clusterId << "]" << endl;
  else if(nentries >= m_n && m_print & DBG_FLAG)
    cout << "Stopped reading due to full width" << endl;
  else if(ins[ins_num].type == -1 && m_print & DBG_FLAG)
    cout << "Stopped reading due to end of ins stream" << endl;

  m_nriq += nread;

  if (f_issued > m_fn)
    m_fp_delay += f_issued - m_fn;
  else if (m_fp_delay >= m_fn - f_issued)
    m_fp_delay -= m_fn - f_issued;

  return ins_num;
}

uint32_t ROB_Dist::ptr_incr (uint32_t ptr)
{
  return ((ptr + 1) % m_cluster_size);
}

uint32_t ROB_Dist::fifo_ptr_incr (uint32_t ptr)
{
  return ((ptr + 1) % m_size);
}

entry_t *ROB_Dist::get_entry (uint32_t clusterId, uint32_t ptr)
{
  return &m_buf_array[clusterId][ptr];
}

fifo_entry_t *ROB_Dist::get_fifo_entry (uint32_t ptr) {
  return &central_fifo[ptr];
}


fifo_entry_t *ROB_Dist::return_fifo_entry(uint32_t index) {
  return &central_fifo[index];
}

uint32_t ROB_Dist::getCyclesToCompletion(uint32_t reg) {
  uint32_t i = fifo_head;
  if(!fifo_empty)
    do {
      fifo_entry_t *fifo_entry = return_fifo_entry(i);
      entry_t *entry = get_entry(fifo_entry->rob_id, fifo_entry->rob_index);
      if(entry->reg_id == reg) {
        //cout << "Found reg: " << reg << "in entry.pc:" << entry->pc << endl;
        return entry->cycles;
      }
      i = fifo_ptr_incr(i);
    } while(i != fifo_tail);
  if (m_print & DBG_FLAG)
    cout << "Couldnt find cycles to completion when expected!" << endl;
  exit(1);
}

void ROB_Dist::write_entry(uint32_t clusterId, ins_t ins)
{
  uint16_t reg_mask = 0xF;

  entry_t *entry = get_entry(clusterId, m_tail[clusterId]);
  entry->valid = false;
  fifo_entry_t *fifo_entry = return_fifo_entry(fifo_tail);
  fifo_entry->valid = false;
  entry->cycles = ins_cyc[ins.type];
  if (ins.type >= FADD)
    entry->cycles += m_fp_delay;
  uint32_t operandRegA = ((ins.regs >> 4) & reg_mask);
  uint32_t operandRegB = ((ins.regs >> 8) & reg_mask);
  uint32_t extraCyclesA = 0;
  uint32_t extraCyclesB = 0;

  if(isinROB (operandRegA)) {
    extraCyclesA = getCyclesToCompletion(operandRegA);
    //cout << "Forwarding ins.pc=" << ins.pc << " operandA in " << extraCyclesA << " cycles" << endl;
    m_nwdu++;
  }
  if(isinROB (operandRegB)) {
    extraCyclesB = getCyclesToCompletion(operandRegB);
    //cout << "Forwarding ins.pc=" << ins.pc << " operandB in " << extraCyclesB << " cycles" << endl;
    m_nwdu++;
  }
  uint32_t maxExtraCycles = max(extraCyclesA, extraCyclesB);
  if(maxExtraCycles)
    //cout << "Increasing ins.pc=" << ins.pc << " cycles cout by " << maxExtraCycles << " to allow forwarding" << endl;
  entry->cycles += maxExtraCycles;

  entry->pc = ins.pc;
  fifo_entry->pc = ins.pc;
  entry->reg_id = ins.regs & reg_mask;
  fifo_entry->reg_id = entry->reg_id;
  fifo_entry->rob_id = clusterId&reg_mask;
  fifo_entry->rob_index = m_tail[clusterId];
  entry->isfp = (ins.type >= FADD);

  m_empty[clusterId] = false;
  fifo_empty = false;

  m_tail[clusterId] = ptr_incr (m_tail[clusterId]);
  fifo_tail = fifo_ptr_incr(fifo_tail);
}

void ROB_Dist::dist_post_cycle_power_tabulation () {
#ifdef P_DEBUG
  if(fifo_prev_head != fifo_head && m_print & DBG_FLAG)
    cout << "*TRANSITION* fifo_head: " << fifo_prev_head << "->" << fifo_head << endl;
#endif
  p_perCycleBitTransitions += num_trans(fifo_prev_head, fifo_head, fifo_head_size);
  p_perCycleBitTransitionsHigh += num_hi_trans(fifo_prev_head, fifo_head, fifo_head_size);
  p_perCycleBitTransitionsLow += num_lo_trans(fifo_prev_head, fifo_head, fifo_head_size);
  p_perCycleBitsRemainedHigh += num_hi(fifo_head,fifo_head_size) - num_hi_trans(fifo_prev_head, fifo_head, fifo_head_size);
  p_perCycleBitsRemainedLow += fifo_head_size - num_hi(fifo_head,fifo_head_size) - num_lo_trans(fifo_prev_head, fifo_head, fifo_head_size);

#ifdef P_DEBUG
  if(fifo_prev_tail != fifo_tail && m_print & DBG_FLAG)
    cout << "*TRANSITION* fifo_tail: " << fifo_prev_tail << "->" << fifo_tail << endl;
#endif
  p_perCycleBitTransitions += num_trans(fifo_prev_tail, fifo_tail, fifo_head_size);
  p_perCycleBitTransitionsHigh += num_hi_trans(fifo_prev_tail, fifo_tail, fifo_head_size);
  p_perCycleBitTransitionsLow += num_lo_trans(fifo_prev_tail, fifo_tail, fifo_head_size);
  p_perCycleBitsRemainedHigh += num_hi(fifo_tail,fifo_head_size) - num_hi_trans(fifo_prev_tail, fifo_tail, fifo_head_size);
  p_perCycleBitsRemainedLow += fifo_head_size - num_hi(fifo_tail,fifo_head_size) - num_lo_trans(fifo_prev_tail, fifo_tail, fifo_head_size);

  for(uint32_t i = 0; i < m_size; i++) {
#ifdef P_DEBUG
    if(prev_central_fifo[i].valid != central_fifo[i].valid && m_print & DBG_FLAG)
      cout << "*TRANSITION* central_fifo[" << i << "].valid: " << prev_central_fifo[i].valid << "->" << central_fifo[i].valid << endl;
#endif
    p_perCycleBitTransitions += num_trans(prev_central_fifo[i].valid, central_fifo[i].valid, 1);
    p_perCycleBitTransitionsHigh += num_hi_trans(prev_central_fifo[i].valid, central_fifo[i].valid, 1);
    p_perCycleBitTransitionsLow += num_lo_trans(prev_central_fifo[i].valid, central_fifo[i].valid, 1);
    p_perCycleBitsRemainedHigh += num_hi(central_fifo[i].valid,1) - num_hi_trans(prev_central_fifo[i].valid, central_fifo[i].valid, 1);
    p_perCycleBitsRemainedLow += (uint32_t)1 - num_hi(central_fifo[i].valid,1) - num_lo_trans(prev_central_fifo[i].valid, central_fifo[i].valid, 1);

#ifdef P_DEBUG
    if(prev_central_fifo[i].pc != central_fifo[i].pc && m_print & DBG_FLAG)
      cout << "*TRANSITION* central_fifo[" << i << "].pc: " << prev_central_fifo[i].pc << "->" << central_fifo[i].pc << endl;
#endif
    p_perCycleBitTransitions += num_trans(prev_central_fifo[i].pc, central_fifo[i].pc, 32);
    p_perCycleBitTransitionsHigh += num_hi_trans(prev_central_fifo[i].pc, central_fifo[i].pc, 32);
    p_perCycleBitTransitionsLow += num_lo_trans(prev_central_fifo[i].pc, central_fifo[i].pc, 32);
    p_perCycleBitsRemainedHigh += num_hi(central_fifo[i].pc,32) - num_hi_trans(prev_central_fifo[i].pc, central_fifo[i].pc, 32);
    p_perCycleBitsRemainedLow += (uint32_t)32 - num_hi(central_fifo[i].pc,32) - num_lo_trans(prev_central_fifo[i].pc, central_fifo[i].pc, 32);

#ifdef P_DEBUG
    if(prev_central_fifo[i].rob_id != central_fifo[i].rob_id && m_print & DBG_FLAG)
      cout << "*TRANSITION* central_fifo[" << i << "].rob_id: " << prev_central_fifo[i].rob_id << "->" << central_fifo[i].rob_id << endl;
#endif
    p_perCycleBitTransitions += num_trans(prev_central_fifo[i].rob_id, central_fifo[i].rob_id, m_cc_bits);
    p_perCycleBitTransitionsHigh += num_hi_trans(prev_central_fifo[i].rob_id, central_fifo[i].rob_id, m_cc_bits);
    p_perCycleBitTransitionsLow += num_lo_trans(prev_central_fifo[i].rob_id, central_fifo[i].rob_id, m_cc_bits);
    p_perCycleBitsRemainedHigh += num_hi(central_fifo[i].rob_id,m_cc_bits) - num_hi_trans(prev_central_fifo[i].rob_id, central_fifo[i].rob_id, m_cc_bits);
    p_perCycleBitsRemainedLow += m_cc_bits - num_hi(central_fifo[i].rob_id,m_cc_bits) - num_lo_trans(prev_central_fifo[i].rob_id, central_fifo[i].rob_id, m_cc_bits);

#ifdef P_DEBUG
    if(prev_central_fifo[i].reg_id != central_fifo[i].reg_id && m_print & DBG_FLAG)
      cout << "*TRANSITION* central_fifo[" << i << "].reg_id: " << prev_central_fifo[i].reg_id << "->" << central_fifo[i].reg_id << endl;
#endif
    p_perCycleBitTransitions += num_trans(prev_central_fifo[i].reg_id, central_fifo[i].reg_id, 4);
    p_perCycleBitTransitionsHigh += num_hi_trans(prev_central_fifo[i].reg_id, central_fifo[i].reg_id, 4);
    p_perCycleBitTransitionsLow += num_lo_trans(prev_central_fifo[i].reg_id, central_fifo[i].reg_id, 4);
    p_perCycleBitsRemainedHigh += num_hi(central_fifo[i].reg_id,4) - num_hi_trans(prev_central_fifo[i].reg_id, central_fifo[i].reg_id, 4);
    p_perCycleBitsRemainedLow += (uint32_t)4 - num_hi(central_fifo[i].reg_id,4) - num_lo_trans(prev_central_fifo[i].reg_id, central_fifo[i].reg_id, 4);
  }

  for(uint32_t clusterId = 0; clusterId < m_cluster_count; clusterId++) {
#ifdef P_DEBUG
    if(m_prev_head[clusterId] != m_head[clusterId] && m_print & DBG_FLAG)
      cout << "*TRANSITION* m_head[" << clusterId << "]: " << m_prev_head[clusterId] << "->" << m_head[clusterId] << endl;
#endif
    p_perCycleBitTransitions += num_trans(m_prev_head[clusterId], m_head[clusterId], m_head_size);
    p_perCycleBitTransitionsHigh += num_hi_trans(m_prev_head[clusterId], m_head[clusterId], m_head_size);
    p_perCycleBitTransitionsLow += num_lo_trans(m_prev_head[clusterId], m_head[clusterId], m_head_size);
    p_perCycleBitsRemainedHigh += num_hi(m_head[clusterId],m_head_size) - num_hi_trans(m_prev_head[clusterId], m_head[clusterId], m_head_size);
    p_perCycleBitsRemainedLow += m_head_size - num_hi(m_head[clusterId],m_head_size) - num_lo_trans(m_prev_head[clusterId], m_head[clusterId], m_head_size);

#ifdef P_DEBUG
    if(m_prev_tail[clusterId] != m_tail[clusterId] && m_print & DBG_FLAG)
      cout << "*TRANSITION* m_tail[" << clusterId << "]: " << m_prev_tail[clusterId] << "->" << m_tail[clusterId] << endl;
#endif
    p_perCycleBitTransitions += num_trans(m_prev_tail[clusterId], m_tail[clusterId], m_head_size);
    p_perCycleBitTransitionsHigh += num_hi_trans(m_prev_tail[clusterId], m_tail[clusterId], m_head_size);
    p_perCycleBitTransitionsLow += num_lo_trans(m_prev_tail[clusterId], m_tail[clusterId], m_head_size);
    p_perCycleBitsRemainedHigh += num_hi(m_tail[clusterId], m_head_size) - num_hi_trans(m_prev_tail[clusterId], m_tail[clusterId], m_head_size);
    p_perCycleBitsRemainedLow += m_head_size - num_hi(m_tail[clusterId], m_head_size) - num_lo_trans(m_prev_tail[clusterId], m_tail[clusterId], m_head_size);

    for(uint32_t i = 0; i < m_cluster_size; i++) {
#ifdef P_DEBUG
      if(m_prev_buf_array[clusterId][i].valid != m_buf_array[clusterId][i].valid && m_print & DBG_FLAG)
        cout << "*TRANSITION* m_buf_array[" << clusterId << "][" << i << "].valid: " << m_prev_buf_array[clusterId][i].valid << "->" << m_buf_array[clusterId][i].valid << endl;
#endif
      p_perCycleBitTransitions += num_trans(m_prev_buf_array[clusterId][i].valid, m_buf_array[clusterId][i].valid, 1);
      p_perCycleBitTransitionsHigh += num_hi_trans(m_prev_buf_array[clusterId][i].valid, m_buf_array[clusterId][i].valid, 1);
      p_perCycleBitTransitionsLow += num_lo_trans(m_prev_buf_array[clusterId][i].valid, m_buf_array[clusterId][i].valid, 1);
      p_perCycleBitsRemainedHigh += num_hi(m_buf_array[clusterId][i].valid, 1) - num_hi_trans(m_prev_buf_array[clusterId][i].valid, m_buf_array[clusterId][i].valid, 1);
      p_perCycleBitsRemainedLow += (uint32_t)1 - num_hi(m_buf_array[clusterId][i].valid, 1) - num_lo_trans(m_prev_buf_array[clusterId][i].valid, m_buf_array[clusterId][i].valid, 1);

#ifdef P_DEBUG
      if(m_prev_buf_array[clusterId][i].pc != m_buf_array[clusterId][i].pc && m_print & DBG_FLAG)
        cout << "*TRANSITION* m_buf_array[" << clusterId << "][" << i << "].pc: " << m_prev_buf_array[clusterId][i].pc << "->" << m_buf_array[clusterId][i].pc << endl;
#endif
      p_perCycleBitTransitions += num_trans(m_prev_buf_array[clusterId][i].pc, m_buf_array[clusterId][i].pc, 32);
      p_perCycleBitTransitionsHigh += num_hi_trans(m_prev_buf_array[clusterId][i].pc, m_buf_array[clusterId][i].pc, 32);
      p_perCycleBitTransitionsLow += num_lo_trans(m_prev_buf_array[clusterId][i].pc, m_buf_array[clusterId][i].pc, 32);
      p_perCycleBitsRemainedHigh += num_hi(m_buf_array[clusterId][i].pc, 32) - num_hi_trans(m_prev_buf_array[clusterId][i].pc, m_buf_array[clusterId][i].pc, 32);
      p_perCycleBitsRemainedLow += (uint32_t)32 - num_hi(m_buf_array[clusterId][i].pc, 32) - num_lo_trans(m_prev_buf_array[clusterId][i].pc, m_buf_array[clusterId][i].pc, 32);

#ifdef P_DEBUG
      if(m_prev_buf_array[clusterId][i].reg_id != m_buf_array[clusterId][i].reg_id && m_print & DBG_FLAG)
        cout << "*TRANSITION* m_buf_array[" << clusterId << "][" << i << "].reg_id: " << m_prev_buf_array[clusterId][i].reg_id << "->" << m_buf_array[clusterId][i].reg_id << endl;
#endif
      p_perCycleBitTransitions += num_trans(m_prev_buf_array[clusterId][i].reg_id, m_buf_array[clusterId][i].reg_id, 4);
      p_perCycleBitTransitionsHigh += num_hi_trans(m_prev_buf_array[clusterId][i].reg_id, m_buf_array[clusterId][i].reg_id, 4);
      p_perCycleBitTransitionsLow += num_lo_trans(m_prev_buf_array[clusterId][i].reg_id, m_buf_array[clusterId][i].reg_id, 4);
      p_perCycleBitsRemainedHigh += num_hi(m_buf_array[clusterId][i].reg_id, 4) - num_hi_trans(m_prev_buf_array[clusterId][i].reg_id, m_buf_array[clusterId][i].reg_id, 4);
      p_perCycleBitsRemainedLow += (uint32_t)4 - num_hi(m_buf_array[clusterId][i].reg_id, 4) - num_lo_trans(m_prev_buf_array[clusterId][i].reg_id, m_buf_array[clusterId][i].reg_id, 4);

#ifdef P_DEBUG
      if(m_prev_buf_array[clusterId][i].result != m_buf_array[clusterId][i].result && m_print & DBG_FLAG)
        cout << "*TRANSITION* m_buf_array[" << clusterId << "][" << i << "].result: " << m_prev_buf_array[clusterId][i].result << "->" << m_buf_array[clusterId][i].result << endl;
#endif
      p_perCycleBitTransitions += num_trans(m_prev_buf_array[clusterId][i].result, m_buf_array[clusterId][i].result, 32);
      p_perCycleBitTransitionsHigh += num_hi_trans(m_prev_buf_array[clusterId][i].result, m_buf_array[clusterId][i].result, 32);
      p_perCycleBitTransitionsLow += num_lo_trans(m_prev_buf_array[clusterId][i].result, m_buf_array[clusterId][i].result, 32);
      p_perCycleBitsRemainedHigh += num_hi(m_buf_array[clusterId][i].result, 32) - num_hi_trans(m_prev_buf_array[clusterId][i].result, m_buf_array[clusterId][i].result, 32);
      p_perCycleBitsRemainedLow += (uint32_t)32 - num_hi(m_buf_array[clusterId][i].result, 32) - num_lo_trans(m_prev_buf_array[clusterId][i].result, m_buf_array[clusterId][i].result, 32);

#ifdef P_DEBUG
      if(m_prev_buf_array[clusterId][i].isfp != m_buf_array[clusterId][i].isfp && m_print & DBG_FLAG)
        cout << "*TRANSITION* m_buf_array[" << clusterId << "][" << i << "].isfp: " << m_prev_buf_array[clusterId][i].isfp << "->" << m_buf_array[clusterId][i].isfp << endl;
#endif
      p_perCycleBitTransitions += num_trans(m_prev_buf_array[clusterId][i].isfp, m_buf_array[clusterId][i].isfp, 1);
      p_perCycleBitTransitionsHigh += num_hi_trans(m_prev_buf_array[clusterId][i].isfp, m_buf_array[clusterId][i].isfp, 1);
      p_perCycleBitTransitionsLow += num_lo_trans(m_prev_buf_array[clusterId][i].isfp, m_buf_array[clusterId][i].isfp, 1);
      p_perCycleBitsRemainedHigh += num_hi(m_buf_array[clusterId][i].isfp, 1) - num_hi_trans(m_prev_buf_array[clusterId][i].isfp, m_buf_array[clusterId][i].isfp, 1);
      p_perCycleBitsRemainedLow += (uint32_t)1 - num_hi(m_buf_array[clusterId][i].isfp, 1) - num_lo_trans(m_prev_buf_array[clusterId][i].isfp, m_buf_array[clusterId][i].isfp, 1);
    }
  }


  //default_post_cycle_power_tabulation();
}

void ROB_Dist::dist_pre_cycle_power_snapshot () {
  fifo_prev_head = fifo_head;
  fifo_prev_tail = fifo_tail;
  for(uint32_t i = 0; i < m_size; i++) {
    prev_central_fifo[i].valid = central_fifo[i].valid;
    prev_central_fifo[i].pc = central_fifo[i].pc;
    prev_central_fifo[i].rob_id = central_fifo[i].rob_id;
    prev_central_fifo[i].reg_id = central_fifo[i].reg_id;
  }

  for(uint32_t clusterId = 0; clusterId < m_cluster_count; clusterId++) {
    m_prev_head[clusterId] = m_head[clusterId];
    m_prev_tail[clusterId] = m_tail[clusterId];
    for(uint32_t i = 0; i < m_cluster_size; i++) {
      m_prev_buf_array[clusterId][i].valid = m_buf_array[clusterId][i].valid;
      m_prev_buf_array[clusterId][i].cycles = m_buf_array[clusterId][i].cycles;
      m_prev_buf_array[clusterId][i].pc = m_buf_array[clusterId][i].pc;
      m_prev_buf_array[clusterId][i].reg_id = m_buf_array[clusterId][i].reg_id;
      m_prev_buf_array[clusterId][i].result = m_buf_array[clusterId][i].result;
      m_prev_buf_array[clusterId][i].isfp = m_buf_array[clusterId][i].isfp;
    }
    m_prev_empty[clusterId] = m_empty[clusterId];
  }
  pre_cycle_power_snapshot();
}

bool ROB_Dist::isinROB( uint16_t reg)
{
  if (fifo_empty)
    return false;
    
  uint32_t i = fifo_head;
  while(i != fifo_tail) {
    p_reg_comp_used++;
    fifo_entry_t *fifo_entry = return_fifo_entry(i);
    if (fifo_entry->reg_id == reg)
      return true;
    i = fifo_ptr_incr(i);
  }
  return false;
}

void ROB_Dist::print_msgs (int cycles)
{
  cout << "Cycles: " << cycles << endl;
  cout << "FIFO head: " << fifo_head << endl;
  cout << "FIFO tail: " << fifo_tail << endl;

  cout << "Head (Cluster0, Cluster1, ...): ";
  for(uint32_t clusterId = 0; clusterId < m_cluster_count; clusterId++) {
    cout << m_head[clusterId] << " ";
  }
  cout << endl << "Tail (Cluster0, Cluster1, ...): ";
  for(uint32_t clusterId = 0; clusterId < m_cluster_count; clusterId++) {
    cout << m_tail[clusterId] << " ";
  }
  cout << endl << "Empty (Cluster0, Cluster1, ...): ";
  for(uint32_t clusterId = 0; clusterId < m_cluster_count; clusterId++) {
    cout << m_empty[clusterId] << " ";
  }
  cout << endl << "Write to ARF: " << m_nwarf << endl;
  cout << "Read from IQ: " << m_nriq << endl;
  cout << "Read from EX: " << m_nrex << endl;

  cout << "# Total Bits in ROB: " << p_bit_count << endl;
  cout << "# Bits Transitioned to High: " << p_perCycleBitTransitionsHigh << endl;
  cout << "# Bits Transitioned to Low: " << p_perCycleBitTransitionsLow << endl;
  cout << "# Bits Remained High: " << p_perCycleBitsRemainedHigh << endl;
  cout << "# Bits Remained Low: " << p_perCycleBitsRemainedLow << endl << endl;
}
