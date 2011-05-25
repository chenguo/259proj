#ifndef ROB_H
#define ROB_H

#include "entry.h"
#include <vector>

using namespace std;

class ROB {
 public:
        ROB (int s, int n);
        virtual void run (int instructions[]);

        int get_max_size() {return m_size;};
        int get_n() {return m_n;};
 protected:
        int m_size;
        vector<entry> m_buff;

        int m_n; // n ways super scalar
/*
        n ports FROM issue queue
        n ports FRROM ex units, assume no double precision
        2*n ports TO dispatch (forwarding), no double
        n ports TO ARF (commits) // again assume no double
*/

    //sizes of ports
        int FROM_IQ;
        int FROM_EX;
        int TO_DU;
        int TO_ARF;



};

#endif
