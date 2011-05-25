#include <cstring>
#include <stdio.h>
#include <unistd.h>
#include <iostream>

#include "rob.h"
#include "rob_circ.h"
using namespace std;

enum {
  MODE_BASE,
  MODE_CIRC,
  MODE_DYN,
  MODE_DIST,
  MODE_LATCH
};


void help ();

int main (int argc, char *argv[])
{
  char opt;
  int mode = MODE_BASE;
  ROB *rob;

  while ((opt = getopt (argc, argv, "chm:")) != -1)
    {
      switch (opt)
        {
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

          // Print help message.
        case 'h':
          help ();
          return 0;

        default: break;

        }
    }

  switch (mode)
    {
    case MODE_BASE:
      cout << "Basic buffer" << endl;
      rob = new ROB (0, 0);
      break;

    case MODE_CIRC:
      cout << "Circular buffer" << endl;
      rob = new ROB_Circ (0, 0);
      break;

    case MODE_DYN:
      cout << "Dynamic" << endl;
      break;

    case MODE_DIST:
      cout << "Distributed" << endl;
      break;

    case MODE_LATCH:
      cout << "Retention latches" << endl;
      break;

    default: break;
    }

  rob->run ();

  return 0;
}

void help ()
{
  cout << "ROB Power Simulator usage information:" << endl;
  cout << "  $ rob_pwr -m [MODE]" << endl;
  cout << "Modes:" << endl;
  cout << "  circular   Circular ROB." << endl;
  cout << "  dynamic    Dynamically resized ROB." << endl;
  cout << "  dist       Distributed ROB." << endl;
  cout << "  latch      ROB with retention latches." << endl;
}
