#ifndef _RELATION_H
#define _RELATION_H

#include <vector>

#define MAX_NUM_CREATING_RELATIONS 100

using namespace std;

class SchemaManager;  //must do forward declaration
class Disk;
class MainMemory;  //must do forward declaration
class Block;

/* Each relation is assumed to be stored in consecutive disk blocks on a single track of the disk 
 * (in clustered way). 
 * The disk blocks on the track are numbered by 0,1,2,... 
 * You have to copy the disk blocks of the relation to memory blocks before accessing the tuples 
 * inside the blocks.
 * To delete tuples in the disk blocks, you can invalidate the tuples and left "holes" 
 * in the original places. Be sure to deal with the holes when doing every SQL operation.
 * You can decide whether to remove trailing "holes" in a relation.
 * Usage: Create an empty relation through SchemaManager class, not throught here.
 *        Create a tuple of the relation through here.
 *        Basically you cannot access data or disk blocks through here directly.
 *        Use the Relation class to copy blocks from the disk to the memory or the other direction.
 *        The Relation class will handle the disk part.
 *        You are able to get schema of a particular relation through here.
 */
class Relation {
  private:
    SchemaManager* schema_manager;
    Disk* disk;
    int schema_index; // points to the schema of the relation
    MainMemory* mem; // a pointer to the main memory
    //vector<Block> data; // consecutive blocks; no limit on the nubmer of blocks
    string relation_name; // name of the relation

    // for internal use only: DO NOT use this constructor. Create a relation through schema manager
    Relation();
    // for internal use only: DO NOT use this constructor. Create a relation through schema manager
    Relation(SchemaManager* schema_manager, int schema_index, string relation_name,
             MainMemory* mem, Disk* disk);

    void null();

  public:
    friend class SchemaManager; // creates Relation; accesses clear()

    string getRelationName() const;
    Schema getSchema() const; // returns the schema of the tuple    
    int getNumOfBlocks() const;
    int getNumOfTuples() const;
    bool isNull() const;
    
    Tuple createTuple() const; //creates an empty tuple of the schema

    //NOTE: Every call to each of the following 3 functions has a simulated disk delay

    //reads one block from the relation (the disk) and stores in the memory
    //returns false if the index is out of bound
    bool getBlock(int relation_block_index, int memory_block_index) const;
    bool getBlocks(int relation_block_index, int memory_block_index, int num_blocks) const;

    //reads one block from the memory and stores in the relation (on the disk)
    // returns false if the index is out of bound
    bool setBlock(int relation_block_index, int memory_block_index);
    bool setBlocks(int relation_block_index, int memory_block_index, int num_blocks);

    //delete the block from [starting_block_index] to the last block
    bool deleteBlocks(int starting_block_index); // return false if out of bound

    void printRelation() const; // print all tuples
    void printRelation(ostream &out) const;
    friend ostream &operator<<(ostream &out, const Relation &r);
};
#endif
