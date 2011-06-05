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

  ins_t *instructions = new ins_t [input.instCount+1];
  int pc_start = 0;
  for (int i = 0; i < input.instCount; i++)
    {
      instructions[i].type = generate_type(input,(rand()%100));
      instructions[i].pc = pc_start;
      instructions[i].regs = rand() %0xFFF;
      instructions[i].exec = false;
      pc_start += 4;
    }
  instructions[input.instCount-1].type = -1;
  instructions[input.instCount-1].pc = -1;

cout << "Instruction stream:" << endl;
  for(int i = 0; i < input.instCount; i++) {
    cout << "Type=" << instructions[i].type << " PC=" << instructions[i].pc << " REGS=" << instructions[i].regs << " EXEC=" << instructions[i].exec << endl;
  }
  return instructions;
}

int generate_type(ins_profile input, int gen) {
  float seed = (float)gen/100;
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
