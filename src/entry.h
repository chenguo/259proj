#ifndef ENTRY_H
#define ENTRY_H

#include <stdint.h>

#define MASK4 0b1111
#define PC_SIZE 32
#define DATA_SIZE 32
struct entry {
        bool valid; //1 is valid
        uint32_t result;
        uint32_t pc;
        uint8_t reg_id;  // only lower bits are used
};



#endif
