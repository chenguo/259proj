#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

#include "common.h"
#include "rob.h"
#include "rob_circ.h"
#include "rob_dyn.h"
#include "rob_latch.h"
#include "rob_dist.h"
using namespace std;

enum {
  MODE_BASE,
  MODE_CIRC,
  MODE_DYN,
  MODE_DIST,
  MODE_LATCH
};

void help ();
ins_t *generate_instruction_stream();
int generate_type(ins_profile input, int gen);
uint32_t generate_register(uint32_t history, float forwardRate);

int main (int argc, char *argv[])
{
  //srand ( time(NULL) );
  srand(2);
  char opt;
  int mode = MODE_BASE;
  ROB *rob;
  ins_t *instructions;
  int rob_size = 128;
  int print_flags = 1;

  while ((opt = getopt (argc, argv, "dhm:")) != -1)
    {
      switch (opt)
        {
          // Debug output.
        case 'd':
          print_flags |= DBG_FLAG;
          break;

          // Print help message.
        case 'h':
          help ();
          return 0;

          // Set mode of ROB simulation.
        case 'm':
          if (strcmp (optarg, "circular") == 0)
            mode = MODE_CIRC;
          else if (strcmp (optarg, "dynamic") == 0)
            mode = MODE_DYN;
          else if (strcmp (optarg, "dist") == 0)
            mode = MODE_DIST;
          else if (strcmp (optarg, "latch") == 0)
            mode = MODE_LATCH;
          else
            {
              cerr << "Unrecognized mode." << endl;
              return 1;
            }
          break;

        default: break;

        }
    }
  srand (10);

  // For now, generate arbitrary mix of instructions.
  instructions = generate_instruction_stream();

  switch (mode)
    {
    case MODE_BASE:
      cout << "Basic buffer" << endl;
      rob = new ROB (rob_size, 3, 1, print_flags);
      break;

    case MODE_CIRC:
      cout << "Circular buffer" << endl;
      rob = new ROB_Circ (rob_size, 3, 1, print_flags);
      break;

    case MODE_DYN:
      cout << "Dynamic" << endl;
      rob = new ROB_Dyn (rob_size, 3, 1, 256, 8, print_flags);
      break;

    case MODE_DIST:
      cout << "Distributed" << endl;
      rob = new ROB_Dist (rob_size, 2, 3, 1, print_flags);
      break;

    case MODE_LATCH:
      cout << "Retention latches" << endl;
      rob = new ROB_Latch (rob_size, 3, 1, 4, print_flags);
      break;

    default: break;
    }

  rob->run (instructions);
  cout << "Simulation complete." << endl;

  delete rob;
  delete [] instructions;
  return 0;
}

void help ()
{
  cout << "ROB Power Simulator usage information:" << endl;
  cout << "  $ rob_pwr [OPTIONS]..." << endl;
  cout << "Options" << endl;
  cout << "  -d         Enable debug messages." << endl;
  cout << "  -h         Display this help message." << endl;
  cout << "  -m         Set operating mode. See \"Modes\"" << endl;
  cout << "Modes:" << endl;
  cout << "  circular   Circular ROB." << endl;
  cout << "  dynamic    Dynamically resized ROB." << endl;
  cout << "  dist       Distributed ROB." << endl;
  cout << "  latch      ROB with retention latches." << endl;
}

ins_t *generate_instruction_stream() {
  ins_profile default_profile;
  default_profile.loadProc = 0.10;
  default_profile.storeProc = 0.10;
  default_profile.addProc = 0.10;
  default_profile.subProc = 0.10;
  default_profile.multProc = 0.10;
  default_profile.divProc = 0.10;
  default_profile.faddProc = 0.10;
  default_profile.fsubProc = 0.10;
  default_profile.fmultProc = 0.10;
  default_profile.fdivProc = 0.10;
  default_profile.forwardingRate = 0.10;
  default_profile.instCount = 10000;

  ins_profile input = default_profile;
  uint32_t reg_history_buffer = 0x0;
  uint32_t regS1 = 0x0;
  uint32_t regS2 = 0x0;
  uint32_t regD = 0x0;

ins_t *instructions = new ins_t [input.instCount+1];
  int pc_start = 0;
  cout << "Instruction stream:" << endl;
  for (int i = 0; i < input.instCount; i++)
    {
      instructions[i].type = generate_type(input,(rand()%100));
      instructions[i].pc = pc_start;
      regS1 = generate_register(reg_history_buffer,input.forwardingRate);
      regS2 = generate_register(reg_history_buffer,input.forwardingRate);
      regD = rand()%0xF;
      reg_history_buffer = (reg_history_buffer << 4) | (regD&0xF);
      instructions[i].regs = ((regS1&0xF) << 8) | ((regS2&0xF) << 4) | (regD&0xF);
      instructions[i].exec = false;
      pc_start += 4;
      cout << "Type=" << instructions[i].type << " PC=" << instructions[i].pc << " REGS S1 S2 D= " << ((instructions[i].regs >> 8)&0xF) << " " << ((instructions[i].regs >> 4)&0xF) << " " << (instructions[i].regs&0xF) << " EXEC=" << instructions[i].exec << endl;
    }
  instructions[input.instCount].type = -1;
  instructions[input.instCount].pc = -1;
  cout << "Type=" << instructions[input.instCount].type << " PC=" << instructions[input.instCount].pc << " REGS S1 S2 D= " << ((instructions[input.instCount].regs >> 8)&0xF) << " " << ((instructions[input.instCount].regs >> 4)&0xF) << " " << (instructions[input.instCount].regs&0xF) << " EXEC=" << instructions[input.instCount].exec << endl;

  return instructions;
}

uint32_t generate_register(uint32_t history, float forwardRate) {
  int seed = rand();
  if((float)(seed%100)/100.0 < forwardRate) { 
    //cout << "Dependency occurred" << endl;
    return (history>>(4*(seed%8))&(0xF));
  } else 
    return (seed%0xF);
}

int generate_type(ins_profile input, int gen) {
  float seed = ((float)gen)/100.0;
  float loadRange = input.loadProc;
  float storeRange = loadRange + input.storeProc;
  float addRange = storeRange + input.addProc;
  float subRange = addRange + input.subProc;
  float multRange = subRange + input.multProc;
  float divRange = multRange + input.divProc;
  float faddRange = divRange + input.faddProc;
  float fsubRange = faddRange + input.fsubProc;
  float fmultRange = fsubRange + input.fmultProc;
  float fdivRange = fmultRange + input.fdivProc;


  if(seed < loadRange)
    return LOAD;
  else if (seed < storeRange)
    return STORE;
  else if (seed < addRange)
    return ADD;
  else if (seed < subRange)
    return SUB;
  else if (seed < multRange)
    return MULT;
  else if (seed < divRange)
    return DIV;
  else if (seed < faddRange)
    return FADD;
  else if (seed < fsubRange)
    return FSUB;
  else if (seed < fmultRange)
    return FMULT;
  else if (seed < fdivRange)
    return FDIV;
  else {
    cout << "Instruction profile percentages dont total 100, check again." << endl;
    exit(1);
  }
}
