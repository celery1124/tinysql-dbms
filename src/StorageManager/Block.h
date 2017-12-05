#ifndef _BLOCK_H
#define _BLOCK_H

#include <vector>

using namespace std;

class Tuple;

/* A disk or memory block contains a number of records/tuples that belong to the same relation. 
 * A tuple CANNOT be splitted and stored in more than one blocks. 
 * Each block is defined to hold as most FIELDS_PER_BLOCK fields. 
 * Therefore, the max number of tuples held in a block can be calculated from the size of a tuple, 
 *   which is the number of fields in a tuple.
 * You can get the number by calling Schema::getTuplesPerBlock().
 *
 * The max number of tuples held in a block = FIELDS_PER_BLOCK / num_of_fields_in_tuple
 *
 * Usage: Blocks already reside in the memory and disk. You don't need to create blocks manually.
 *        Most time when you need to use the Block class is to access a block of the main memory
 *        and to get or modify the tuples in the memory block.
 *        First be sure to get a pointer to a memory block from the Memory class.
 */
class Block {
  private:
    vector<Tuple> tuples;
    Block(); // for internal use only: DO NOT use this constructor. Use the blocks in Memory or Relation
    static Block getDummyBlock(); // for internal use: returns an empty block

  public:
    friend class MainMemory;  // allocates blocks
    //friend class Relation; // allocates blocks
    friend class Disk;

    bool isFull() const;
    bool isEmpty() const;
    void clear(); //empty the block
    int getNumTuples() const; // returns current number of tuples inside this block
    Tuple getTuple(int tuple_offset) const; // gets the tuple value at tuple_index;
                                            //returns empty Tuple if tuple_index out of bound
    vector<Tuple> getTuples() const; // returns all the tuples inside this block
    bool setTuple(int tuple_offset, const Tuple& tuple); // sets new tuple value at tuple_offset;
                                                         //returns false if tuple_offset out of bound
    // remove all the tuples; sets new tuples for the block;
    // returns false if number of input tuples exceeds the space limit
    bool setTuples(const vector<Tuple>& tuples);
    // remove all the tuples; sets new tuples for the block;
    // returns false if number of input tuples exceeds the space limit
    bool setTuples(const vector<Tuple>::const_iterator first, const vector<Tuple>::const_iterator last);
    bool appendTuple(const Tuple& tuple);  // appends one tuple to the end of the block;
                                           //returns false if total number of tuples exceeds the space limit
    bool nullTuple(int tuple_offset); // invalidates the tuple at the offset
    bool nullTuples(); // invalidates all the tuples in the block
    
    void printBlock() const;
    void printBlock(ostream &out) const;
    friend ostream &operator<<(ostream &out, const Block &b);

};

#endif
