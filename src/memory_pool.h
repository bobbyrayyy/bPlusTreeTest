#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <vector>
#include <unordered_map>
#include <tuple>

// Defines a single movie record (read from data file).
struct Record
{
  float averageRating; // Average rating of this movie.
  int numVotes;        // Number of votes of this movie.
  char tconst[10];     // ID of the movie.
};

class MemoryPool
{
public:
  // =============== Methods ================ //

  // Creates a new memory pool with the following parameters:
  // maxPoolSize: Maximum size of the memory pool.
  // blockSize: The fixed size of each block in the pool.
  MemoryPool(std::size_t maxPoolSize, std::size_t blockSize);

  // Allocate a new block from the memory pool. Returns false if error.
  bool allocateBlock();

  // Allocates a new chunk to the memory pool.
  // Creates a new block if chunk is unable to fit in current free block.
  // Returns a tuple with blockID and the record's offset within the block.
  std::tuple<int, int> allocate(Record record);

  // Deallocates an existing record and block if block becomes empty. Returns false if error.
  bool deallocate(int blockID, int offset);

  // Access a record given a blockID and offset. Returns a record.
  Record read(int blockID, int offset) const;

  // Returns the maximum size of this memory pool.
  std::size_t getMaxPoolSize() const
  {
    return maxPoolSize;
  }

  // Returns the size of a block in memory pool.
  std::size_t getBlockSize() const
  {
    return blockSize;
  };

  // Returns current size used in memory pool (total blocks size).
  std::size_t getSizeUsed() const
  {
    return sizeUsed;
  }

  // Returns actual size of all records stored in memory pool.
  std::size_t getActualSizeUsed() const
  {
    return actualSizeUsed;
  }

  // Returns number of currently allocated blocks.
  int getAllocated() const
  {
    return allocated;
  };

  // Destructor
  ~MemoryPool();

private:
  // =============== Data ================ //

  std::size_t maxPoolSize;    // Maximum size allowed for pool.
  std::size_t blockSize;      // Size of each block in pool in bytes.
  std::size_t sizeUsed;       // Current size used up for storage (total block size).
  std::size_t actualSizeUsed; // Actual size used based on records stored in storage.

  int allocated; // Number of currently allocated blocks.

  std::unordered_map<int, std::vector<Record>> pool; // Memory pool reference (implemented as a map).
  int block;                                         // Current blockID we are inserting to.
};

#endif