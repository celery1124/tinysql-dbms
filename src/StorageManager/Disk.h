#ifndef _DISK_H
#define _DISK_H

#include <vector>
using namespace std;

#define NUM_TRACKS 100
#define avg_seek_time 6.46
#define avg_rotation_latency 4.17
#define avg_transfer_time_per_block 64

/* Simplified assumptions are made for disks. A disk contains many tracks. 
 * We assume each relation reside on a single track of blocks on disk. 
 * Everytime to read or write blocks of a relation takes time:
 *
 * (AVG_SEEK_TIME + AVG_ROTATION_LATENCY + AVG_TRANSFER_TIME_PER_BLOCK * num_of_consecutive_blocks)
 *
 * The number of disk I/O is calculated by the number of blocks read or written.
 * Usage: At the beginning of your program, you need to initialize a disk.
 *       You don't need to access Disk directly except for getting disk I/O counts
 *       When you need to access a relation, use the Relation class
 */
class Disk {
  private:
    //Properties are defined based on the Megatron 747 disk sold in 2001.
    
    //One block holds 16384 bytes (although a block only holds 8 fields in here)
    //Thus, a relation of 60 tuples/blocks occupies as much as 960K
    //If memory has 1/6 of the relation size, then the memory has only 160K space
    //However, we want to simulate the speed of a 300M relation and a 50M memory
    //So we increase the transfer time of a block by 320 folds
    //static const double avg_seek_time=6.46;
    //static const double avg_rotation_latency=4.17;
    //static const double avg_transfer_time_per_block=0.20 * 320;

    vector<Block> tracks[NUM_TRACKS];
    unsigned long int diskIOs;
    double timer;
    
    // for internal use: extend the track to 'block_index'-1; no disk latency
    bool extendTrack(int schema_index, int block_index, const Tuple& t);
    // for internal use: shrink the track to 'block_index'-1; no disk latency
    bool shrinkTrack(int schema_index, int block_index);
    // for internal use: increment Disk I/O count
    void incrementDiskIOs(int count);
    void incrementDiskTimer(int num_blocks);
    
    Block getBlock(int schema_index, int block_index);
    vector<Block> getBlocks(int schema_index, int block_index, int num_blocks);
    bool setBlock(int schema_index, int block_index, const Block& b);
    bool setBlocks(int schema_index, int block_index, const vector<Block>& vb);
    
  public:
    friend class Relation;
    Disk();
    // Reset the disk I/O counter.
    // Every time before you do a SQL operation, reset the counter.
    void resetDiskIOs();
    // After the operation is done, get the accumulated number of disk I/Os
    unsigned long int getDiskIOs() const;
    // Reset the disk timmer.
    // Every time before you do a SQL operation, reset the timmer.
    void resetDiskTimer();
    // After the operation is done, get the elapse disk time in milliseconds
    double getDiskTimer() const;
};

#endif
