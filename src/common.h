#ifndef ENTRY_H
#define ENTRY_H

#include <stdint.h>

#define MASK4 0b1111
#define PC_SIZE 32
#define DATA_SIZE 32
#define ENTRY_BIT_COUNT 70 // 1+32+32+4+1

#define P_DEBUG

// ROB entry.
typedef struct entry {
  bool valid; // 1 bit
  // More flags?
  uint32_t cycles;
  uint32_t result; // 32 bits
  uint32_t pc; // 32 bits
  uint16_t reg_id;  // only 4  lower bits are used
  bool isfp; // 1 bit
} entry_t;

typedef struct fifo_entry {
  bool valid; // 1
  uint32_t pc;   // 32 bits
  uint32_t rob_id;  // only log(c)/log(2) bits
  uint16_t reg_id; // only 4 bits 
  uint32_t rob_index;  // simulation purposes only, not modeled
} fifo_entry_t;

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
