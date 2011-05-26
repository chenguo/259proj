#ifndef ENTRY_H
#define ENTRY_H

#include <stdint.h>

#define MASK4 0b1111
#define PC_SIZE 32
#define DATA_SIZE 32

// ROB entry.
typedef struct entry {
  bool valid; //1 is valid
  uint32_t cycles;
  uint32_t result;
  uint32_t pc;
  uint16_t flags;  // TODO: figure out what these flags are.
  uint16_t reg_id;  // only lower bits are used
} entry_t;

typedef struct instruction {
  uint32_t type;
  uint32_t pc;
} ins_t;

// Enumerated instruction types.
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

// Instruction cycle cost look up table.
// TODO: make this exhaustive, and fill in actual numbers.
extern int ins_cyc[];

// Hamming distance calculator.
int ham_dist (uint32_t, uint32_t);

#endif  // ENTRY_H
