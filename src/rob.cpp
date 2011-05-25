#include <iostream>

#include "rob.h"
using namespace std;

ROB::ROB (int s, int n)
        : m_size(s)
        , m_n(n)
{
        head = 0;
        tail = 0;
        FROM_IQ = PC_SIZE;
        FROM_EX = DATA_SIZE;
        TO_DU = DATA_SIZE;
        TO_ARF = DATA_SIZE;
}

void ROB::run ()
{
  cout << "In run method." << endl;
}
