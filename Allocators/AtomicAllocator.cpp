#include "AtomicAllocator.h"
#include <sys/types.h>

AtomicAllocator::AtomicAllocator(uint blockSize, uint maxBlocks)
    : Allocator(blockSize, maxBlocks) {
  AtomicAllocator(blockSize, maxBlocks, Setup());
}

AtomicAllocator::AtomicAllocator(uint blockSize, uint maxBlocks, Setup setup)
    : Allocator(blockSize, maxBlocks), setup(setup) {
  // FIXME: maxBlocks * blockSize ? Keep low while testing.
  // But Cache false sharing occurs resulting in many cache misses.
  auto totalStorageArea = maxBlocks;
  blocks = std::make_unique<Block[]>(totalStorageArea);
  for (uint i = 0; i < totalStorageArea - 1; i++) {
    blocks[i].next = &blocks[i + 1];
  }
  blocks[totalStorageArea - 1].next = nullptr;
  freeListGlobal.store(&blocks[0], std::memory_order_release);
}

void *AtomicAllocator::allocate() {
  if (freeListLocal == nullptr) {
    // Refill from global free list
    int reloads = setup.threadLocalFill;
    Block *head = freeListGlobal.load(std::memory_order_acquire);
    // FIXME: Fix these Atomic operations
    while (reloads && head != nullptr &&
           freeListGlobal.compare_exchange_weak(head, head->next,
                                                std::memory_order_acquire,
                                                std::memory_order_relaxed)) {
      reloads--;
      countLocal++;
      head->next = freeListLocal;
      freeListLocal = head;
      head = freeListGlobal.load(std::memory_order_acquire);
    }
  }
  auto head = freeListLocal;
  if (head) {
    freeListLocal = freeListLocal->next;
    countLocal--;
  }
  return head;
}
void AtomicAllocator::deallocate(void *block) {
  // flush local free list if greater than threshold
  int threshold = setup.threadLocalFill * setup.giveBackToGlobalRatio;
  // if countLocal > threshold, flush local free list to global free list
  if (countLocal > threshold) {
    // FIXME: Returning back to Global not coded correctly.
    auto headL = freeListLocal;
    freeListLocal = freeListLocal->next;
    auto tailG = freeListTailGlobal.load(std::memory_order_acquire);
    countLocal--;
  }
  // populate local free list
  auto headL = static_cast<Block *>(block);
  headL->next = freeListLocal;
  freeListLocal = headL;
  countLocal++;
}
AtomicAllocator::~AtomicAllocator() { blocks.release(); }