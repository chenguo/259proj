#ifndef ROB_DIST_H
#define ROB_DIST_H

#include "rob.h"
using namespace std;

class ROB_Dist : public ROB {
 public:
  ROB_Dist (int s, int c, int in, int fn, int pflags);
  virtual ~ROB_Dist();
  virtual void run (ins_t instructions[]);
  bool isinROB(uint16_t reg);

  int get_head(uint32_t rob) {return m_head[rob];};
  int get_tail(uint32_t rob) {return m_tail[rob];};

 protected:
  void update_entries ();
  void write_to_arf ();
  int read_from_iq (int, ins_t[]);

  virtual uint32_t ptr_incr (uint32_t ptr);
  uint32_t fifo_ptr_incr (uint32_t ptr);
  virtual entry_t *get_entry (uint32_t clusterId, uint32_t ptr);
  fifo_entry_t *get_fifo_entry (uint32_t ptr);
  fifo_entry_t *return_fifo_entry(uint32_t index);
  virtual void write_entry (uint32_t clusterId, ins_t ins);
  virtual void print_msgs (int cycles);

  uint32_t getCyclesToCompletion(uint32_t reg);

  void dist_pre_cycle_power_snapshot();
  void dist_post_cycle_power_tabulation();

  uint32_t m_cluster_count;
  uint32_t m_cluster_size;
  uint32_t m_cc_bits;
  entry_t **m_buf_array;
  entry_t **m_prev_buf_array;

  fifo_entry_t *central_fifo;
  fifo_entry_t *prev_central_fifo;
  uint32_t fifo_head;
  uint32_t fifo_tail;
  bool fifo_empty;
  uint32_t fifo_prev_head;
  uint32_t fifo_prev_tail;
  uint32_t fifo_head_size;
  uint32_t fifo_entry_bit_count;

  uint32_t *m_head;
  uint32_t *m_tail;

  uint32_t *m_prev_head;
  uint32_t *m_prev_tail;

  uint32_t m_head_size;

  bool *m_empty;  // When m_head == m_tail, denote if buffer is empty/full.
  bool *m_prev_empty;
};

#endif /* #ifndef ROB_DIST_H */
