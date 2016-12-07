#include "cache.h"
#include "def.h"
#include <stdlib.h>

// ---- LFU ----
void Cache::ReplaceAlgorithm_LFU(uint64_t addr, int &cycle, int read)
{
  // your code here
}

// if needed
void Cache::LFU_update(bool ishit, uint64_t addr, int target)
{}

// ---- FIFO ----
void Cache::ReplaceAlgorithm_FIFO(uint64_t addr, int &cycle, int read)
{
  // your code here
}

// if needed
void Cache::FIFO_update(bool ishit, uint64_t addr, int target)
{}

// ---- RANDOM ----
void Cache::ReplaceAlgorithm_RANDOM(uint64_t addr, int &cycle, int read)
{
  // your code here
}

// if needed
void Cache::RANDOM_update(bool ishit, uint64_t addr, int target)
{}