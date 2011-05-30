#ifndef ROB_CIRC_H
#define ROB_CIRC_H

#include "rob.h"
using namespace std;

class ROB_Circ : public ROB {
 public:
  ROB_Circ (int s, int in, int fn);
  virtual ~ROB_Circ();
  virtual void run (ins_t instructions[]);
  bool isinROB( uint16_t reg);

  int get_head() {return m_head;};
  int get_tail() {return m_tail;};

 protected:
  void update_entries ();
  void write_to_arf ();
  int read_from_iq (bool, int, ins_t[]);

  virtual uint32_t ptr_incr (uint32_t ptr);
  virtual entry_t *get_entry (uint32_t ptr);
  virtual void write_entry (entry_t *entry, ins_t ins);
  virtual void print_msgs (int cycles);

  void pre_cycle_power_snapshot();
  void post_cycle_power_tabulation();
  void print_power_stats (int cycles);



  uint32_t m_head;
  uint32_t m_tail;

  uint32_t m_prev_head;
  uint32_t m_prev_tail;

  uint32_t p_bit_count;
  uint32_t m_head_size;


  bool m_empty;  // When m_head == m_tail, denote if buffer is empty/full.
};

#endif /* #ifndef ROB_CIRC_H */
