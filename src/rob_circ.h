#ifndef ROB_CIRC_H
#define ROB_CIRC_H

#include "rob.h"
using namespace std;

class ROB_Circ : public ROB {
 public:
  ROB_Circ (int s, int n);
  virtual void run (int instructions[]);

  int get_head() {return m_head;};
  int get_tail() {return m_tail;};

 protected:
  int m_head;
  int m_tail;
  bool empty;  // When m_head == m_tail, denote if buffer is empty/full.
};

#endif /* #ifndef ROB_CIRC_H */
