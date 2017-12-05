#ifndef _MAIN_MEMORY_H
#define _MAIN_MEMORY_H

#include <vector>

#include "Config.h"
#include "Tuple.h"

using namespace std;

class Block;

/* The simulated memory holds NUM_OF_BLOCKS_IN_MEMORY blocks numbered with 0,1,2,... 
 * You can get total number of blocks in the memory by calling MainMemory::getMemorySize(). 
 * Before accessing data of a relation, you have to copy the disk blocks of a relation 
 * to the simulated main memory. 
 * Then, access the tuples in the simulated main memory. Or in the other direction, 
 * you will copy the memory blocks to disk blocks of a relation when writing data to the relation. 
 * Because the size of memory is limited, you have to do the database operations wisely. 
 * We assume there is no latency in accessing memory.
 * Usage: At the beginning of your program, you need to initialize a memory.
 *       If you need to access the tuples in a single memory block, get the pointer to the block first
 *         and then get the tuples inside using the Block class.
 *       If you need to access tuples in consecutive memory blocks, get a vector of tuples
 *         directly from here.
 */

class MainMemory {
  private:
    Block blocks[NUM_OF_BLOCKS_IN_MEMORY]; // an array of blocks
    bool setBlock(int memory_block_index, const vector<Block>::const_iterator first,
                  const vector<Block>::const_iterator last);
  public:
    friend class Relation;

    MainMemory();
    int getMemorySize() const; // returns total number of blocks in the memory (including empty ones)

    Block* getBlock(int memory_block_index); //returns NULL if out of bound
    //can be used to copy a memory block to another memory block
    //returns false if out of bound or tuples do not match the schema
    bool setBlock(int memory_block_index, const Block& b);
							   
    //Gets tuples from consecutive blocks from memory
    //   [ memory_block_begin, memory_block_begin+num_blocks-1 ]
    //NOTE: The output tuples must all belong to the same relation/table.
    //IMPORTANT NOTE: Only the valid tuples in the blocks are returned
    vector<Tuple> getTuples(int memory_block_begin,int num_blocks) const;

    //Writes tuples consecutively starting from memory block memory_block_begin;
    //returns false if out of bound in memory
    //NOTE: The input tuples must all belong to the same relation/table.
    bool setTuples(int memory_block_begin,const vector<Tuple>& tuples);

    void dumpMemory() const; // print memory content
    void dumpMemory(ostream &out) const;
    friend ostream &operator<<(ostream &out, const MainMemory &m);
};

#endif
