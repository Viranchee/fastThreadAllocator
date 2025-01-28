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

1 M blocks
64 Threads
If equal division, 16k blocks per Thread

Hybrid way:
Global pool of free list
64 threads have their own local free list, maybe 2k Blocks each. Like a cache.
Every time a Thread exhausts it's free list, it asks for more 2k blocks slab

64 * 2k blocks = 128k blocks used. 1024*1024 - 128 * 1024 = (1024 - 128) * 1024
= 917.5k more blocks 2k block allocation on every renewal (when FL gets empty)
917.5k blocks, 2k per alloc from Global thread
448 more allocation calls

Looks like a Good tradeoff.

Those global memory allocation calls can be synced via either Atomics, Locks,
Semaphore/Mutexes, etc
*/

#include <thread>
#include <vector>
using namespace std;

#define NUMTHREADS 64
// 1024*1024 Threads
#define BLOCKSIZE 1
#define NUMBLOCKS 1048576
#define NUMTHREADS_IN_ALLOCATION 2048

class FlashBlockAllocator {
public:
  FlashBlockAllocator(int blockSize, int numBlocks)
      : blockSize(blockSize), numBlocks(numBlocks) {
    // Allocate 2k blocks for each thread
  }

  void setup() { get2kFreeList(); }

  void *allocate() {
    // If no blocks left, get 2k more blocks
    thread_freeList.empty() && get2kFreeList();
    void *last = thread_freeList.back();
    thread_freeList.pop_back();
    return last;
  }

  void deallocate(void *block) {
    thread_freeList.push_back(block);
    if (thread_freeList.size() > NUMTHREADS_IN_ALLOCATION * 2) {
      vector<void *> freePool;
      for (int i = 0; i < NUMTHREADS_IN_ALLOCATION; i++) {
        freePool.push_back(thread_freeList.back());
        thread_freeList.pop_back();
      }
      free2kBlocks(freePool);
    }
  }

private:
  const int numBlocks;
  const int blockSize;
  int allocations = 0;
  // Thread Local Free List
  // FIXME: Remove below line before submission
  // static thread_local
  vector<void *> thread_freeList;
  vector<vector<void *>> global_freeListBatch; // Use atomics here

  bool get2kFreeList() {
    auto last = global_freeListBatch.back();
    global_freeListBatch.pop_back();
    thread_freeList.insert(thread_freeList.end(), last.begin(), last.end());
    return true;
  }
  // get 2k more blocks
  bool get2kBlocks() {
    vector<void *> freeList;
    for (int i = 0; i < NUMTHREADS_IN_ALLOCATION; i++) {
      // Allocate 2k blocks
      void *block = malloc(blockSize);
      if (!block) {
        return false;
      }
      freeList.push_back(block);
    }
    global_freeListBatch.push_back(freeList);
    return true;
  }

  bool free2kBlocks(vector<void *> &blocks) {
    global_freeListBatch.push_back(blocks);
    return true;
  }
};

int main() {
  // 64 threads
  auto allocator = FlashBlockAllocator(BLOCKSIZE, NUMBLOCKS);
  vector<thread> threads;
  for (int i = 0; i < NUMTHREADS; i++) {
    threads.push_back(thread([&allocator]() {
      allocator.setup();
      // Do some work
    }));
  }
  for (auto &t : threads) {
    t.join();
  }
}