#ifndef INS_H
#define INS_H

// Enum of instruction types.
// TODO: make this exhaustive.
enum {
  LOAD,
  STORE,
  ADD,
  SUB,
  MULT,
  DIV,
  FMULT,
  FDIV
};

#define INS_TYPES 8

// Instruction look up table.
// TODO: make this exhaustive, and fill in actual numbers.
int ins_cyc[] = {
  1, 2, 3, 4, 5, 6, 7, 8, 9, 10
};


#endif  /* INS_H */
