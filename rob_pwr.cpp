#include <cstring>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
using namespace std;

enum {
  MODE_NONE,
  MODE_DYN,
  MODE_DIST,
  MODE_LATCH
};


void help ();

int main (int argc, char *argv[])
{
  char opt;
  int mode = MODE_NONE;

  while ((opt = getopt (argc, argv, "hm:")) != -1)
    {
      switch (opt)
        {
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
    case MODE_NONE:
      return 1;

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
