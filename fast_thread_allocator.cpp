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