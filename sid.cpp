#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

struct Block {
  Block *next;
};

class BlockAllocator {
private:
  std::vector<char> memory_pool; // The main memory pool
  std::mutex global_mutex; // Mutex for synchronizing global free list access
  Block *global_free_list = nullptr; // Global free list (protected by mutex)

  static thread_local Block *cache; // Thread-local cache
  static thread_local size_t cache_count;

  static constexpr size_t CACHE_LIMIT = 15625;    // 1M / 64 threads
  static constexpr size_t BLOCK_SIZE = 64;        // Example block size
  static constexpr size_t TOTAL_BLOCKS = 1048576; // 1M blocks

public:
  BlockAllocator() {
    memory_pool.resize(BLOCK_SIZE * TOTAL_BLOCKS);

    // Initialize the global free list
    Block *current = reinterpret_cast<Block *>(memory_pool.data());
    for (size_t i = 0; i < TOTAL_BLOCKS - 1; ++i) {
      current->next =
          reinterpret_cast<Block *>(memory_pool.data() + (i + 1) * BLOCK_SIZE);
      current = current->next;
    }
    current->next = nullptr;
    global_free_list = reinterpret_cast<Block *>(memory_pool.data());
  }

  Block *Allocate() {
    if (cache_count == 0) {
      RefillCache();
    }

    if (cache_count > 0) {
      Block *block = cache;
      cache = cache->next;
      cache_count--;
      return block;
    }

    return nullptr; // No memory left
  }

  void Deallocate(Block *block) {
    if (!block)
      return;

    if (cache_count < CACHE_LIMIT) {
      // Return block to thread-local cache
      block->next = cache;
      cache = block;
      cache_count++;
    } else {
      // Return block to global free list
      std::lock_guard<std::mutex> lock(global_mutex);
      block->next = global_free_list;
      global_free_list = block;
    }
  }

private:
  void RefillCache() {
    std::lock_guard<std::mutex> lock(global_mutex);

    if (!global_free_list) {
      std::cerr << "Out of memory!" << std::endl;
      return;
    }

    size_t count = 0;
    while (global_free_list && count < CACHE_LIMIT) {
      Block *block = global_free_list;
      global_free_list = global_free_list->next;

      block->next = cache;
      cache = block;
      count++;
    }
    cache_count = count;
  }
};

// Thread-local storage definition
thread_local Block *BlockAllocator::cache = nullptr;
thread_local size_t BlockAllocator::cache_count = 0;

// Test the allocator
void TestThread(BlockAllocator &allocator, int num_allocs) {
  for (int i = 0; i < num_allocs; i++) {
    Block *block = allocator.Allocate();
    if (block) {
      allocator.Deallocate(block);
    }
  }
}

int main() {
  BlockAllocator allocator;
  std::vector<std::thread> threads;

  // Simulate 64 threads allocating and deallocating memory
  for (int i = 0; i < 64; i++) {
    threads.emplace_back(TestThread, std::ref(allocator), 20000);
  }

  for (auto &t : threads) {
    t.join();
  }

  std::cout << "Test complete!" << std::endl;
  return 0;
}
