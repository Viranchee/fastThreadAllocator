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
  // Allocates from the head of threadLocal list. Refills from head of Global
  // list.
  void *allocate() override;
  // Deallocates to head of threadLocal list. Refills to tail of Global list.
  void deallocate(void *block) override;
  ~MutexAllocator() override;

private:
  std::unique_ptr<Block[]> blocks; // memory pool
  Block *freeListGlobal, *freeListTailGlobal;
  int countGlobal;
  std::mutex mtx;

  const Setup setup;
};
