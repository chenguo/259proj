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
  MODE_DYNOPT,
  MODE_DIST,
  MODE_LATCH
};

void help ();
ins_t *generate_instruction_stream(int benchmark, int print_flags);
int generate_type(ins_profile input, int gen);
uint32_t generate_register(uint32_t history, float forwardRate);

int main (int argc, char *argv[])
{
  char opt;
  int mode = MODE_BASE;
  int bench = 0;
  ROB *rob;
  ins_t *instructions;
  int rob_size = 256;
  int print_flags = 0;

  while ((opt = getopt (argc, argv, "dhm:b:")) != -1)
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
          else if (strcmp (optarg, "dyn-opt") == 0)
            mode = MODE_DYNOPT;
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
        case 'b':
          if (strcmp(optarg, "0") == 0)
            bench = 0;
          else if (strcmp(optarg, "1") == 0)
            bench = 1;
          else if (strcmp(optarg, "2") == 0)
            bench = 2;
          else if (strcmp(optarg, "3") == 0)
            bench = 3;
          else {
            cerr << "Unrecognized benchmark." << endl;
            return 1;
          }
          break;

        default: break;

        }
    }
  srand (10);

  // For now, generate arbitrary mix of instructions.
  instructions = generate_instruction_stream(bench, print_flags);

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
      // Overflow = 512, partitions = 16
      rob = new ROB_Dyn (rob_size, 3, 1, 512, 16, print_flags, false);
      break;

    case MODE_DYNOPT:
      cout << "Dynamic Optimized" << endl;
      rob = new ROB_Dyn (rob_size, 3, 1, 512, 16, print_flags, true);
      break;

    case MODE_DIST:
      cout << "Distributed" << endl;
      rob = new ROB_Dist (rob_size, 2, 3, 1, print_flags);
      break;

    case MODE_LATCH:
      cout << "Retention latches" << endl;
      rob = new ROB_Latch (rob_size, 3, 1, 16, print_flags);
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
  cout << "  -b         Set benchmark. See \"Benchmarks\"" << endl;
  cout << "Modes:" << endl;
  cout << "  circular   Circular ROB." << endl;
  cout << "  dynamic    Dynamically resized ROB." << endl;
  cout << "  dyn-opt  Optimized dynamic ROB." << endl;
  cout << "  dist       Distributed ROB." << endl;
  cout << "  latch      ROB with retention latches." << endl;
  cout << "Benchmarks:" << endl;
  cout << "  0          [Default] All ins types equally likely with above average operand forwarding rate (10%)." << endl;
  cout << "  1          173.applu SPEC-FP Parabolic/Elliptic Partial Diff. Equations benchmark. Mostly FP ops." << endl;
  cout << "  2          191.fma3d SPEC-FP Finite-element Crash Simulation benchmark. Mostly INT ops." << endl;
  cout << "  3          183.equake SPEC-FP Seismic Wave Propagation benchmark.  Mix of FP and INT ops." << endl;
}

ins_t *generate_instruction_stream(int benchmark, int print_flags) {
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
  default_profile.instCount = 100000;

  ins_profile applu173_fp;
  applu173_fp.loadProc = 0.32;
  applu173_fp.storeProc = 0.12;
  applu173_fp.addProc = 0.01;
  applu173_fp.subProc = 0.01;
  applu173_fp.multProc = 0.01;
  applu173_fp.divProc = 0.01;
  applu173_fp.faddProc = 0.13;
  applu173_fp.fsubProc = 0.13;
  applu173_fp.fmultProc = 0.13;
  applu173_fp.fdivProc = 0.13;
  applu173_fp.forwardingRate = 0.06;
  applu173_fp.instCount = 20;

  ins_profile fma3d191_int;
  fma3d191_int.loadProc = 0.12;
  fma3d191_int.storeProc = 0.24;
  fma3d191_int.addProc = 0.15;
  fma3d191_int.subProc = 0.15;
  fma3d191_int.multProc = 0.15;
  fma3d191_int.divProc = 0.15;
  fma3d191_int.faddProc = 0.01;
  fma3d191_int.fsubProc = 0.01;
  fma3d191_int.fmultProc = 0.01;
  fma3d191_int.fdivProc = 0.01;
  fma3d191_int.forwardingRate = 0.06;
  fma3d191_int.instCount = 100000;

  ins_profile equake183_mix;
  equake183_mix.loadProc = 0.37;
  equake183_mix.storeProc = 0.06;
  equake183_mix.addProc = 0.08;
  equake183_mix.subProc = 0.08;
  equake183_mix.multProc = 0.08;
  equake183_mix.divProc = 0.08;
  equake183_mix.faddProc = 0.07;
  equake183_mix.fsubProc = 0.06;
  equake183_mix.fmultProc = 0.06;
  equake183_mix.fdivProc = 0.06;
  equake183_mix.forwardingRate = 0.06;
  equake183_mix.instCount = 100000;


  ins_profile input;
  switch(benchmark) {
    case 0:
      cout << "Initializing default instruction profile." << endl;
      input = default_profile;
      break;
    case 1:
      cout << "Initilizing 173.applu SPEC-FP Parabolic/Elliptic Partial Diff. Equations benchmark. Emphasis on FP ops." << endl;
      input = applu173_fp;
      break;
    case 2:
      cout << "Initializing 191.fma3d SPEC-FP Finite-element Crash Simulation benchmark. Emphasis on INT ops." << endl;
      input = fma3d191_int;
      break;
    case 3:
      cout << "Initializing 183.equake SPEC-FP Seismic Wave Propagation benchmark.  Mix of both FP and INT ops." << endl;
      input = equake183_mix;
      break;
    default:
      input = default_profile;
      break;
  }

  uint32_t reg_history_buffer = 0x0;
  uint32_t regS1 = 0x0;
  uint32_t regS2 = 0x0;
  uint32_t regD = 0x0;

ins_t *instructions = new ins_t [input.instCount+1];
  int pc_start = 0;
  if (print_flags & DBG_FLAG)
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
//      instructions[i].exec = false;
      pc_start += 4;
      if (print_flags & DBG_FLAG) {
          cout << "Type=" << instructions[i].type << " PC=" << instructions[i].pc << " REGS S1 S2 D= " << ((instructions[i].regs >> 8)&0xF) << " " << ((instructions[i].regs >> 4)&0xF) << " " << (instructions[i].regs&0xF) << endl;
      }
    }
  instructions[input.instCount].type = -1;
  instructions[input.instCount].pc = -1;
  if (print_flags & DBG_FLAG) {
    cout << "Type=" << instructions[input.instCount].type << " PC=" << instructions[input.instCount].pc << " REGS S1 S2 D= " << ((instructions[input.instCount].regs >> 8)&0xF) << " " << ((instructions[input.instCount].regs >> 4)&0xF) << " " << (instructions[input.instCount].regs&0xF) << endl;
  }

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
