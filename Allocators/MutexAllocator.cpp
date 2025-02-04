#include "MutexAllocator.h"

MutexAllocator::MutexAllocator(uint blockSize, uint maxBlocks)
    : Allocator(blockSize, maxBlocks) {
  MutexAllocator(blockSize, maxBlocks, Setup());
}

MutexAllocator::MutexAllocator(uint blockSize, uint maxBlocks, Setup setup)
    : Allocator(blockSize, maxBlocks), setup(setup) {
  auto totalStorageArea = maxBlocks;
  blocks = std::make_unique<Block[]>(totalStorageArea);
  for (uint i = 0; i < totalStorageArea - 1; i++) {
    blocks[i].next = &blocks[i + 1];
  }
  blocks[totalStorageArea - 1].next = nullptr;
  freeListGlobal = &blocks[0];
  freeListTailGlobal = &blocks[totalStorageArea - 1];
}

void *MutexAllocator::allocate() {
  if (!freeListLocal) {
    Block *head, *tail;
    {
      int blocksToFill = setup.threadLocalFill;
      std::lock_guard<std::mutex> lock(mtx);
      if (blocksToFill < countGlobal)
        blocksToFill = countGlobal;
      head = freeListGlobal;
      tail = head;
      countGlobal -= blocksToFill;
      while (blocksToFill--) {
        tail = tail->next;
      }
      freeListGlobal = tail->next;
    }
    tail->next = freeListLocal;
    freeListLocal = head;
  }
  auto head = freeListLocal;
  if (freeListLocal)
    freeListLocal = freeListLocal->next;
  return head;
}

void MutexAllocator::deallocate(void *block) {
  int threshold = setup.threadLocalFill * setup.giveBackToGlobalRatio;
  if (countLocal > threshold) {
    Block *headL = freeListLocal;
    freeListLocal = freeListLocal->next;
    {
      std::lock_guard<std::mutex> lock(mtx);
      freeListTailGlobal->next = headL;
      freeListTailGlobal = headL;
      countGlobal++;
    }
    countLocal--;
  }
  auto headL = static_cast<Block *>(block);
  headL->next = freeListLocal;
  freeListLocal = headL;
  countLocal++;
}

MutexAllocator::~MutexAllocator() = default;