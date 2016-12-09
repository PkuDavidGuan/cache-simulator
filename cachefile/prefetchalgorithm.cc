#include "cache.h"
#include "def.h"
#include <stdlib.h>
#include <queue>
#include <iostream>
#include <time.h>

using namespace std;


/* ------------------------------------------------------------------------

  Prefetch algorithm:
  if decide_do_prefetch:                                -- implemented
    get the address of the next four lines
    call HandleRequest recursively in this cache level
      in HandleRequest-recursion:
      do not bypass
      if hit, do not mark the cache line as prefetch, as if nothing happened
      else, miss
        use replace algorithm
        fetch from memory
        update evict queue
  
  Prefetch evaluation algorithm:
  when handle request:
    if hit && prefetched: useful_prefetch ++            -- implemented
    NOTE: update prefetch bit when after replace
    if miss && prefetch_evicted: harmful_prefetch ++    -- implemented
    NOTE: put evicted addr into evict queue after replace
  note that we do not count useless prefetch, since it is useless
  [harmful prefetch] := visit an addr, miss, the addr was evited by last prefetch

------------------------------------------------------------------------ */

uint64_t Cache::GetPrefetchAddr(uint64_t current_addr, int line_size, int next_line)
{
  return ( (uint64_t) (current_addr / line_size + next_line) ) * ( (uint64_t) line_size );
}

void Cache::AlwaysPrefetch(int prefetch_num)
{
  printf("always prefetch, prefetch number: %d\n", prefetch_num);
  int i;
  uint64_t prefetch_addr;
  char tmpcontent[100];
  int hit;
  int cycle;
  for(i = 0; i < prefetch_num; i++)
  {
    prefetch_addr = GetPrefetchAddr(current_addr, config_.line_size, (i + 1));
    HandleRequest(prefetch_addr, config_.line_size, READ_PREFETCH, (unsigned char *)tmpcontent, hit, cycle);
  }
}

bool Cache::PrefetchDecision_TAGGED()
{
  return false;
}

void Cache::TaggedPrefetch(int prefetch_num)
{}

bool Cache::PrefetchDecision_LEARNED()
{
  return false;
}

void Cache::LearnedPrefetch(int prefetch_num)
{}

