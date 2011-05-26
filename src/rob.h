#ifndef ROB_H
#define ROB_H

#include "common.h"

using namespace std;

class ROB {
 public:
  ROB (int s, int n);
  ~ROB ();
  virtual void run (ins_t instructions[]);

  int get_max_size() {return m_size;};
  int get_n() {return m_n;};
 protected:
  uint32_t m_size;
  entry_t *m_buf;
  uint32_t m_n;  // n ways super scalar
  uint32_t m_nbiton;

/*
  n ports FROM issue queue
  n ports FRROM ex units, assume no double precision
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
};

#endif
