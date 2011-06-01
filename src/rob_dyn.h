#ifndef ROB_DYN_H
#define ROB_DYN_H

#include "rob_circ.h"
using namespace std;

enum {
  // ENABLED and TO_DISABLE denotes partition is in use.
  ENABLED,
  TO_DISABLE,
  // DISABLED and TO_ENABLE denotes partition is not in use.
  DISABLED,
  TO_ENABLE
};

typedef struct partition {
  int state;
  entry_t *buf;
} part_t;


class ROB_Dyn : public ROB_Circ {
 public:
  ROB_Dyn  (int s, int in, int fn, int overflow, int parts, int pflags);
  virtual ~ROB_Dyn ();
  virtual void run (ins_t instructions[]);

 protected:
  part_t *m_parts;
  part_t *m_prev_parts;
  int m_nparts;
  uint32_t m_part_size;

  virtual uint32_t head_incr (uint32_t ptr);
  virtual uint32_t tail_incr (uint32_t ptr);
  virtual entry_t *get_entry (uint32_t ptr);
  virtual entry_t *get_prev_buf_entry (uint32_t ptr);
  virtual void error_diag ();

  void pre_cycle_power_snapshot ();
  void post_cycle_power_tabulation ();
  void print_power_stats (int cycles);

 private:
  void dyn_process (int cycles);
  void dyn_shrink (int parts);
  void dyn_grow (int parts);

  uint32_t m_update_period;
  uint32_t m_sample_period;
  uint32_t m_samples_taken;
  uint32_t m_current_size;
  uint32_t m_active_size;
  uint32_t m_overflow_cnt;
  uint32_t m_overflow_thresh;
  uint32_t m_partition_cycles;
};

#endif // ROB_DYN_H
