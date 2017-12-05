#ifndef _CONFIG_H
#define _CONFIG_H

#define FIELDS_PER_BLOCK 8 // Therefore, a block can hold 1-8 tuples depending on the relation schema.
#define MAX_NUM_OF_FIELDS_IN_RELATION 8
#define NUM_OF_BLOCKS_IN_MEMORY 10 // Starts with small memory to test one-pass and two-pass algorithms
//#define NUM_OF_BLOCKS_IN_MEMORY 300 // To measure algorithm performance on 1000 tuples, use this value
#define SIMULATED_DISK_LATENCY_ON 1 // Setting to 1 turns on the simulated disk latency
#define DISK_I_O_DEBUG 0 // Setting to 1 turns on the debug message of disk I/O incrementation

#endif
