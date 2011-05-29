#ifndef ENTRY_H
#define ENTRY_H

#include <stdint.h>

#define MASK4 0b1111
#define PC_SIZE 32
#define DATA_SIZE 32

// ROB entry.
typedef struct entry {
  bool valid; //1 is valid
  // More flags?
  uint32_t cycles;
  uint32_t result;
  uint32_t pc;
  uint16_t reg_id;  // only 4  lower bits are used
  bool isfp;
} entry_t;

typedef struct instruction {
  int type;
  uint32_t pc;
  uint16_t regs; // 4 bits each reg id, -blank-reg1,-reg2-regdest
  bool exec; // true is already executed
} ins_t;

// Enumerated instruction types.
// TODO: make this exhaustive.
enum {
  LOAD = 0,
  STORE,
  ADD,
  SUB,
  MULT,
  DIV,
  FADD,
  FSUB,
  FMULT,
  FDIV,
  INS_TYPES
};


// Instruction cycle cost look up table.
// TODO: make this exhaustive, and fill in actual numbers.
extern int ins_cyc[];

// Hamming distance calculator.
int bits_on (uint32_t, uint32_t);
int num_trans(uint32_t oval, uint32_t nval, uint32_t num);

int num_hi_trans(uint32_t oval, uint32_t nval, uint32_t num);
int num_lo_trans(uint32_t oval, uint32_t nval, uint32_t num);

int num_hi(uint32_t val, uint32_t num);
#endif  // ENTRY_H
