struct Block {
  Block *next;
};

class BlockAllocator {
private:
  std::vector<char> memory_pool; //  The main memory pool
  thread_local Block *cache =
      nullptr; //  Pointer to the first free block in the thread-local cache
  thread_local size_t cache_count =
      0; // nteger to track the number of blocks in the thread-local cache

  static constexpr size_t CACHE_LIMIT = 15625; // 1M/64

public:
  void Initialize(size_t block_size, size_t total_blocks) {
    memory_pool.resize(block_size * total_blocks);

    // Link all blocks together as a free list
    Block *current = reinterpret_cast<Block *>(memory_pool.data());
    for (size_t i = 0; i < total_blocks - 1; ++i) {
      current->next =
          reinterpret_cast<Block *>(memory_pool.data() + (i + 1) * block_size);
      current = current->next;
    }
    current->next = nullptr;

    // Give each thread a small cache to start with
    cache = reinterpret_cast<Block *>(memory_pool.data());
    cache_count = CACHE_LIMIT;
  }

  Block *Allocate() {
    if (cache_count > 0) {
      Block *block = cache;
      cache = cache->next;
      cache_count--;
      return block;
    }
    return nullptr;
  }

  void Deallocate(Block *block) {
    if (!block)
      return;

    if (cache_count < CACHE_LIMIT) {
      block->next = cache;
      cache = block;
      cache_count++;
    }
  }
};