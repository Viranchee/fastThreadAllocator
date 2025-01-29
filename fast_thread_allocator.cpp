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

Instead of 1024*1024 new / malloc calls, just make a batched new call, and
divide the addresses. That way it's a slab allocation. We save space in ptr
metadata.
*/
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

using namespace std;

#define NUMTHREADS 64
#define BLOCKSIZE 64
#define MAXBLOCKS 1048576
#define NUMBLOCKS_IN_ALLOCATION 2048

class FlashBlockAllocator {
public:
  FlashBlockAllocator(int blockSize, int numBlocks)
      : blockSize(blockSize), numBlocks(numBlocks) {}

  void setup() { get2kFreeList(); }

  void *allocate() {
    if (thread_freeList.empty()) {
      get2kFreeList();
    }
    void *last = thread_freeList.back();
    thread_freeList.pop_back();
    return last;
  }

  void deallocate(void *block) {
    thread_freeList.push_back(block);
    if (thread_freeList.size() > NUMBLOCKS_IN_ALLOCATION * 2) {
      vector<void *> freePool;
      for (int i = 0; i < NUMBLOCKS_IN_ALLOCATION; i++) {
        freePool.push_back(thread_freeList.back());
        thread_freeList.pop_back();
      }
      free2kBlocks(freePool);
    }
  }

  ~FlashBlockAllocator() {
    for (auto &batch : global_freeListBatch) {
      free(batch[0]); // Free the entire allocated slab
    }
  }

private:
  const int numBlocks;
  const int blockSize;
  int allocations = 0;
  static thread_local vector<void *> thread_freeList;
  vector<vector<void *>> global_freeListBatch;
  mutex global_freeListMutex;

  bool get2kFreeList() {
    lock_guard<mutex> lock(global_freeListMutex);
    if (global_freeListBatch.empty() && !allocate2kBlocks()) {
      throw runtime_error("Failed to allocate 2k blocks");
    }
    auto last = move(global_freeListBatch.back());
    global_freeListBatch.pop_back();
    thread_freeList.insert(thread_freeList.end(), last.begin(), last.end());
    return true;
  }

  bool allocate2kBlocks() {
    if (allocations >= MAXBLOCKS / NUMBLOCKS_IN_ALLOCATION) {
      return false;
    }
    vector<void *> newBlockList;
    void *bigBlock = malloc(NUMBLOCKS_IN_ALLOCATION * blockSize);
    if (!bigBlock) {
      return false;
    }
    for (int i = 0; i < NUMBLOCKS_IN_ALLOCATION; i++) {
      newBlockList.push_back(static_cast<char *>(bigBlock) + i * blockSize);
    }
    global_freeListBatch.push_back(move(newBlockList));
    allocations++;
    return true;
  }

  bool free2kBlocks(vector<void *> &blocks) {
    lock_guard<mutex> lock(global_freeListMutex);
    global_freeListBatch.push_back(move(blocks));
    return true;
  }
};

thread_local vector<void *> FlashBlockAllocator::thread_freeList;

int main() {
  FlashBlockAllocator allocator(BLOCKSIZE, MAXBLOCKS);
  vector<thread> threads;

  for (int i = 0; i < NUMTHREADS; i++) {
    threads.emplace_back([&allocator]() {
      allocator.setup();
      void *ptr = allocator.allocate();
      allocator.deallocate(ptr);
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  return 0;
}
