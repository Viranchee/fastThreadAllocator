/*
Write a C++ class (or pseudo code) that implements a fixed-size block
allocator to optimize the frequent allocation of blocks in a
multi-threaded process.  Assume it must support up to 1M blocks with 64
parallel threads fighting for allocation.

The goal is to increase the efficiency of fixed-block size allocations
and frees during runtime, since malloc/new tend to be slow in a highly
parallel application.

*/

/*
1M blocks
64 threads
Solution:
- Each thread has cached free list
- There is a global free list
- When local thread's free list size is 0, get more blocks from global free list
- When local thread's free-count exceeds a limit, return blocks to global free
list

Synchronization:
- Make it flexible to swap the synchronization mechanism
*/

#include <cstddef>
#include <sys/types.h>
class Allocator {
public:
  const uint blockSize, maxBlocks;
  Allocator(uint blockSize, uint maxBlocks)
      : blockSize(blockSize), maxBlocks(maxBlocks) {}
  virtual void *allocate() = 0;
  virtual void deallocate(void *block) = 0;
  virtual ~Allocator() = 0;
};
