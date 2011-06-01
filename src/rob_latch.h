#ifndef ROB_LATCH_H
#define ROB_LATCH_H

#include "rob_circ.h"
using namespace std;

typedef struct lentry {
  uint32_t data;
  uint16_t reg_id;  // only 4  lower bits are used
  bool isfp;
} lentry_t;


class ROB_Latch : public ROB_Circ {
 public:
  ROB_Latch (int s, int in, int fn, int lsize, int pflags);
  virtual ~ROB_Latch();

 protected:
    uint32_t m_lhead;
    uint32_t m_ltail;
    uint32_t m_lsize;

    bool* Afwd;
    bool* Afwd_prev;
    lentry* lbuf;
    lentry* lbuf_prev;

    void ReadinROB(uint16_t reg);
    bool ReadinLatch(uint16_t reg);
    uint32_t fwdcnt;

    void  update_entries ();

    void addToLatch(uint16_t reg, uint32_t data, bool fp);

    void write_to_arf ();
    void write_entry (entry *entry, ins_t ins);

    void pre_cycle_power_snapshot();
    void post_cycle_power_tabulation();

};

#endif /* #ifndef ROB_LATCH_H */
