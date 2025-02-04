#include "Allocator.h"
#include <atomic>
#include <cstddef>
#include <memory>
#include <sys/types.h>

class AtomicAllocator : public Allocator {
public:
  struct Setup {
    int threadLocalFill = 1024;
    float giveBackToGlobalRatio = 1.50;
  };

  AtomicAllocator(uint blockSize, uint maxBlocks);
  AtomicAllocator(uint blockSize, uint maxBlocks, Setup setup);
  void *allocate() override;
  void deallocate(void *block) override;
  ~AtomicAllocator() override;

private:
  std::unique_ptr<Block[]> blocks; // memory pool
  std::atomic<Block *> freeListGlobal;
  std::atomic<Block *> freeListTailGlobal;
  std::atomic<int> countGlobal;

  const Setup setup;
};
