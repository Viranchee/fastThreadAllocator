#include "Allocator.h"
#include <cstddef>
#include <memory>
#include <mutex>
#include <sys/types.h>

class MutexAllocator : public Allocator {
public:
  struct Setup {
    int threadLocalFill = 1024;
    float giveBackToGlobalRatio = 1.50;
  };

  MutexAllocator(uint blockSize, uint maxBlocks);
  MutexAllocator(uint blockSize, uint maxBlocks, Setup setup);
  void *allocate() override;
  void deallocate(void *block) override;
  ~MutexAllocator() override;

private:
  struct Block {
    Block *next;
  };

  std::unique_ptr<Block[]> blocks; // memory pool
  Block *freeListGlobal, *freeListTailGlobal;
  int countGlobal;
  std::mutex mtx;

  static thread_local Block *freeListLocal;
  static thread_local int countLocal;
  const Setup setup;
};
