#include "memory.h"
#include "string.h"

void Memory::HandleRequest(uint64_t addr, int bytes, int read,
                          unsigned char *content, int &hit, int &cycle) 
{
  #ifdef DEBUG
  printf("\n\nmemory, handle request at addr: %lx",  addr);
  if(read == READ_) printf("  READ\n");
  else printf("  WRITE\n");
  #endif
  // stats update
  // never update on prefetch
  if(read != READ_PREFETCH)
  {
    cycle += latency_.hit_latency;
    stats_.access_counter++;
    stats_.access_cycle += latency_.hit_latency;
  }
  hit = 1;
}

// ---- show the access state ----
void Memory::ShowStat()
{
  printf("memory\n");
  // printf("access counter: every time we access this level of cache\n");
  printf("access counter:   %d\n", stats_.access_counter);
  // printf("access cycle: total latency happened in this level of cache\n");
  printf("access cycle:     %d\n", stats_.access_cycle);
}
