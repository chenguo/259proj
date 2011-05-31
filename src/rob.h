#ifndef ROB_H
#define ROB_H

#include "common.h"

using namespace std;

class ROB {
 public:
  ROB (int s, int in, int fn);
  virtual ~ROB ();
  virtual void run (ins_t instructions[]);

  int get_max_size() {return m_size;};
  int get_n() {return m_n;};
  void default_pre_cycle_power_snapshot();
  void default_post_cycle_power_tabulation();
  void default_print_power_stats (int cycles);
  void default_update_power_totals ();

 protected:
  uint32_t m_size;
  entry_t *m_buf;
  entry_t *m_prev_buf;
  uint32_t m_in; // m_in int units
  uint32_t m_fn; // m_fn fp units
  uint32_t m_fp_delay; // Models "backed-up"-ness of FP units.
  uint32_t m_n;  // n ways super scalar
  uint32_t m_nbiton;

  virtual void write_entry (entry_t *entry, ins_t ins) {};
  virtual void print_msgs (int cycles) {};

/*
  n ports FROM issue queue
  n ports FROM ex units, assume no double precision
  2*n ports TO dispatch (forwarding), no double
  n ports TO ARF (commits) // again assume no double
*/

  //sizes of ports
  int FROM_IQ;
  int FROM_EX;
  int TO_DU;
  int TO_ARF;

  // Drive counts.
  int m_nriq;    // Num reads from IQ
  int m_nrex;    // Num reads from EX
  int m_nwdu;    // Num writes to DU
  int m_nwarf;   // Num writes to ARF

  // Power tabulation
  int p_perCycleBitTransitions;
  int p_perCycleBitTransitionsHigh;
  int p_perCycleBitTransitionsLow;
  int p_perCycleBitsRemainedHigh;
  int p_perCycleBitsRemainedLow;

  int p_totalBitTransitions;
  int p_totalBitTransitionsHigh;
  int p_totalBitTransitionsLow;
  int p_totalBitsRemainedHigh;
  int p_totalBitsRemainedLow;

  int p_bit_count;

  int p_reg_comp_used; 
};

#endif
