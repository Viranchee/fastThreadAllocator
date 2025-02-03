#include "AtomicAllocator.h"
#include <sys/types.h>
// #include <vector>

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
  // populate local free list
  auto head = static_cast<Block *>(block);
  head->next = freeListLocal;
  freeListLocal = head;
  countLocal++;
  // flush local free list if greater than threshold
}
AtomicAllocator::~AtomicAllocator() {
  if (!freeListLocal)
    return;
  auto head = freeListLocal;
  auto tail = head;
  while (tail) {
    tail = tail->next;
  }
}
/*

#include <atomic>
#include <cstddef>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

constexpr int BLOCK_SIZE = 64;
constexpr size_t MAX_BLOCKS = 1'000'000;
constexpr size_t THREAD_COUNT = 64;
constexpr size_t RELOAD_COUNT = 64;

class AtomicBlockAllocator {
private:
  struct Block {
    Block *next;
  };
  struct FreeList {
    int count;
    Block *freeList;
  };
  std::unique_ptr<Block[]> blocks;
  std::atomic<Block *> freeList;
  std::atomic<int> count;
  static thread_local Block *localFreeList;

public:
  AtomicBlockAllocator() {
    blocks = std::make_unique<Block[]>(MAX_BLOCKS);
    for (size_t i = 0; i < MAX_BLOCKS - 1; i++) {
      blocks[i].next = &blocks[i + 1];
    }
    blocks[MAX_BLOCKS - 1].next = nullptr;
    freeList.store(&blocks[0], std::memory_order_release);
  }

  std::weak_ptr<void *> allocate() {
    Block *head = nullptr;
    if (!localFreeList) {
      // get more blocks from Global freeList
      int reloadsLeft = RELOAD_COUNT;
    }
    if (localFreeList) {
      head = localFreeList;
      localFreeList = localFreeList->next;
    }
    return std::weak_ptr<void *>(head);
  }

  void deallocate(void *ptr) {
    if (!ptr)
      return;
    Block *head = static_cast<Block *>(ptr);
    head->next = localFreeList;
    localFreeList = head;
  }

  void flush_local_free_list() {
    auto head = localFreeList;
    while (head) {
      Block *next = head->next;
      deallocate_to_global(head);
      head = next;
    }
    localFreeList = nullptr;
  }

private:
  void deallocate_to_global(Block *block) {
    Block *head;
    do {
      head = freeList.load(std::memory_order_acquire);
      block->next = head;
    } while (!freeList.compare_exchange_weak(
        head, block, std::memory_order_release,
std::memory_order_relaxed));
  }
};

thread_local AtomicBlockAllocator::Block
*AtomicBlockAllocator::localFreeList = nullptr;

int main() {
  AtomicBlockAllocator allocator;

  auto worker = [&allocator]() {
    void *ptr = allocator.allocate();
    if (ptr) {
      std::this_thread::sleep_for(std::chrono::microseconds(10));
      allocator.deallocate(ptr);
    }
  };

  std::vector<std::thread> threads;
  for (size_t i = 0; i < THREAD_COUNT; ++i) {
    threads.emplace_back(worker);
  }

  for (auto &thread : threads) {
    thread.join();
  }

  allocator.flush_local_free_list();

  std::cout << "Memory allocation test completed successfully." <<
std::endl; return 0;
}
*/