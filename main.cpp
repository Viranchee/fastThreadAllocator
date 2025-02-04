
// #include "Allocators/Allocator.h"
#include "Allocators/AtomicAllocator.h"
#include <string>
#include <thread>
#include <vector>

#define NUMTHREADS 64
#define BLOCKSIZE 64
#define MAXBLOCKS 16384

// write tests
void test1(Allocator *allocator, std::string name) {}
int main() { return 0; }