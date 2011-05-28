#ifndef ROB_CIRC_H
#define ROB_CIRC_H

#include "rob.h"
using namespace std;

class ROB_Circ : public ROB {
 public:
  ROB_Circ (int s, int n);
  virtual void run (ins_t instructions[]);

  int get_head() {return m_head;};
  int get_tail() {return m_tail;};

 protected:
  void update_entries ();
  void write_to_arf ();
  int read_from_iq (uint32_t, bool, int, ins_t[]);

  uint32_t m_head;
  uint32_t m_tail;
  bool m_empty;  // When m_head == m_tail, denote if buffer is empty/full.
};

#endif /* #ifndef ROB_CIRC_H */
