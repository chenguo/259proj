#include <cstring>
#include <stdio.h>
#include <unistd.h>
#include <iostream>

#include "rob_base.h"
using namespace std;

enum {
  MODE_BASE,
  MODE_DYN,
  MODE_DIST,
  MODE_LATCH
};


void help ();

int main (int argc, char *argv[])
{
  char opt;
  int mode = MODE_BASE;
  bool circular = false;

  while ((opt = getopt (argc, argv, "chm:")) != -1)
    {
      switch (opt)
        {
          // Run a circular buffer.
        case 'c':
          circular = true;
          break;

          // Set mode of ROB simulation.
        case 'm':
          if (strcmp (optarg, "dynamic") == 0)
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
    case MODE_BASE: {
      cout << "Basic buffer" << endl;
      rob_base rob (0,0);
      rob.run ();
      break;
    }

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

  return 0;
}

void help ()
{
  cout << "ROB Power Simulator usage information:" << endl;
  cout << "  $ rob_pwr -m [MODE]" << endl;
  cout << "Modes:" << endl;
  cout << "  dynamic    Dynamically resized ROB." << endl;
  cout << "  dist       Distributed ROB." << endl;
  cout << "  latch      ROB with retention latches." << endl;
}
