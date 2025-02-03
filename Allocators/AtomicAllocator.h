#include "Allocator.h"
#include <atomic>
#include <cstddef>
#include <memory>
#include <sys/types.h>

class AtomicAllocator : public Allocator {
public:
  struct Setup {
    int threadLocalFill = 1024;
  };

  AtomicAllocator(uint blockSize, uint maxBlocks);
  AtomicAllocator(uint blockSize, uint maxBlocks, Setup setup);
  void *allocate() override;
  void deallocate(void *block) override;
  ~AtomicAllocator() override;

private:
  struct Block {
    Block *next;
  };

  std::unique_ptr<Block[]> blocks; // memory pool
  std::atomic<Block *> freeListGlobal;
  std::atomic<int> countGlobal;

  static thread_local Block *freeListLocal;
  static thread_local int countLocal;
  const Setup setup;
};
