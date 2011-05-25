#ifndef ROB_CIRC_H
#define ROB_CIRC_H

#include "entry.h"
#include "rob.h"
#include <vector>
using namespace std;

class ROB_Circ : public ROB {
 public:
  ROB_Circ (int s, int n) : ROB (s, n) { };
  virtual void run ();
};







#endif /* #ifndef ROB_CIRC_H */
