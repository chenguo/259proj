#ifndef ROB_DYN_H
#define ROB_DYN_H

#include "rob_circ.h"
using namespace std;

typedef struct partition {
  bool use;
  entry_t *buf;
} part_t;


class ROB_Dyn : public ROB_Circ {
 public:
  ROB_Dyn  (int s, int in, int fn, int overflow, int parts);
  virtual ~ROB_Dyn ();
  virtual void run (ins_t instructions[]);

 protected:
  part_t *m_parts;
  int m_nparts;
  uint32_t m_part_size;
 private:
  uint32_t m_update_period;
  uint32_t m_sample_period;
  uint32_t m_samples_taken;
  uint32_t m_current_size;
  uint32_t m_active_size;
  uint32_t m_overflow_cnt;
  uint32_t m_overflow_thresh;
};

#endif // ROB_DYN_H
