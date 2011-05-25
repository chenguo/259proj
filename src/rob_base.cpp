#include <iostream>

#include "rob_base.h"
using namespace std;

rob_base::rob_base(int s, int n)
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

void rob_base::run ()
{
  cout << "In run method." << endl;
}
