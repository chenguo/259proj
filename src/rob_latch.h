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
  ROB_Latch (int s, int in, int fn, int lsize);
  virtual ~ROB_Latch();
  virtual void run (ins_t instructions[]);

 protected:
    int m_lhead;
    int m_ltail;
    int m_lsize;
    
    bool* Afwd;
    lentry* lbuf;
    lentry* lbuf_prev;


  void write_to_arf ();
  int read_from_iq (uint32_t, bool, int, ins_t[]);

  void pre_cycle_power_snapshot();
  void post_cycle_power_tabulation();

};

#endif /* #ifndef ROB_LATCH_H */
