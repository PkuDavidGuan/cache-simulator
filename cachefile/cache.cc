#include "cache.h"
#include "def.h"
#include "memory.h"
#include "string.h"
#include <stdlib.h>
#include <time.h>

extern Memory VMEM;

// ---- cache constructor ----
void Cache::init(int _level, uint64_t _size, int _ass, int _setnum, 
  int _wt, int _wa, int _buscyc, int _hitcyc, Storage *_low, int _PrefetchPolicy)
{
  config_.size = _size;
  config_.associativity = _ass;
  config_.set_num = _setnum;
  config_.write_through = _wt;
  config_.write_allocate = _wa;
  latency_.hit_latency = _hitcyc;
  latency_.bus_latency = _buscyc;
  level = _level;
  lower_ = _low;
  ReplacePolicy = REPLACEPOLICY;
  PrefetchPolicy = _PrefetchPolicy;

  for(int i = 0; i < _setnum; ++i)
    for(int j = 0; j < _ass; ++j)
    {
      store[i][j].valid = false;
      store[i][j].flag = 0;
      store[i][j].recent = 0;
      store[i][j].frequency = 0;
      store[i][j].dirty = false;
      store[i][j].reused = false;
      store[i][j].rd = PD;
      memset(store[i][j].c, 0, sizeof(store[i][j].c));
    }

  s = b = -1;
  int linesize = _size/_ass/_setnum;
  config_.line_size = linesize;

  ShowConfig();

  while(_setnum != 0)
  {
    s++;
    _setnum = _setnum >> 1;
  }
  while(linesize != 0)
  {
    b++;
    linesize = linesize >> 1;
  }

  timestamp = 0;
  if(REPLACEPOLICY == RANDOM)
    srand(time(0));
  for(int i = 0; i < PARTITIONNUM; ++i)
  {
    partCtrl[i].access_counter = partCtrl[i].miss_num = 0;
  }
}

// ---- Main access process ----
// [in]  addr: access address
// [in]  bytes: target number of bytes
// [in]  read: 0|1 for write|read
//             3|4 for write|read in prefetch
// [i|o] content: in|out data
// [out] hit: 0|1 for miss|hit
// [out] cycle: total access cycle
// -- should be good now
void Cache::HandleRequest(uint64_t addr, int bytes, int read,
                          unsigned char *content, int &hit, int &cycle, int evicted) 
{
  // every time handle a request, add timestamp
  #ifdef DEBUG
  printf("\n\ncache level %d, handle request at addr: %lx", level, addr);
  if(read == READ_) printf("  READ\n");
  else if(read == WRITE_) printf("  WRITE\n");
  else printf("  READ_PREFETCH\n");
  #endif

  // time stamp update
  timestamp++;
  // bug detection
  if(timestamp > TIMESTAMP_LIMIT)
  {
    printf("alert: too many access time!\n");
    while(1);
  }

  // total cycle update
  // NOTE: if prefetch, do not need to update this guy
  if(read != READ_PREFETCH && evicted != EVICTED)
  {
    current_addr = addr;
    cycle += latency_.hit_latency + latency_.bus_latency;
    // storage status update
    stats_.access_cycle += latency_.hit_latency + latency_.bus_latency;
    (stats_.access_counter)++;
    #ifdef DEBUG
    printf("accessing cache level %d, cycle = %d\n", level, cycle);
    #endif
  }

  // Decide on whether a bypass should take place.
  if (!BypassDecision(addr, read)) 
  {

    // Generate some infomation for the data partition.
    // PartitionAlgorithm();

    // Check whether the required data already exist.
    int targetaddr;
    uint64_t vpn = (addr << (64-s-b)) >> (64-s);
    uint64_t vpo = (addr << (64-b)) >> (64-b);

    // hit or miss
    // tested
    if (ReplaceDecision(addr, targetaddr, read)) 
    {
      // Hit!
      // update the storage status
      #ifdef DEBUG
      printf("cache level %d, ", level);
      if(read == READ_) printf("READ hit\n");
      else if(read == WRITE_) printf("WRITE hit\n");
      else printf("READ_PREFETCH hit\n");
      #endif    
      hit = 1;

      if (PrefetchDecision(read, evicted)) 
      {
        // Fetch the other data via HandleRequest() recursively.
        // To distinguish from the normal requests,
        // the 2|3 is employed for prefetched write|read data
        // while the 0|1 for normal ones.
        #ifdef DEBUG_PREFETCH
        printf("cache level %d, decide to do prefetch....\n", level);
        #endif
        stats_.prefetch_num++;
        PrefetchAlgorithm();
      }

      // read the cache
      if(read == READ_)
      {
        ;
        // memcpy(content, store[vpn][targetaddr].c+vpo, bytes);
      }
      else if(read == WRITE_)
      {
        // write back
        // tested
        if(config_.write_through == WRITEBACK) 
        {
          // printf("%lx write back.\n", addr);
          store[vpn][targetaddr].dirty = true;
          // memcpy(store[vpn][targetaddr].c+vpo, content, bytes);
        }
        // have not tested this branch
        else //write through
        {
          // printf("%lx write through.\n", addr);
          // memcpy(store[vpn][targetaddr].c+vpo, content, bytes);
          (stats_.fetch_num)++;
          lower_->HandleRequest(addr, bytes, read, content, hit, cycle);
        }
      }
      // have not tested
      else//prefetch
      {
        ;
      }
      return;
    }

    // not hit
    // Choose a victim for the current request.
    #ifdef DEBUG
    printf("cache level %d, ", level);
    if(read == READ_) printf("READ miss\n");
    else if(read == WRITE_) printf("WRITE miss\n");
    else printf("READ_PREFETCH miss\n");
    #endif 
    hit = 0;
    if(read != READ_PREFETCH)
    {
      (stats_.miss_num)++;
      (stats_.fetch_num)++;
    }

    partCtrl[addr>>38].miss_num++;
    partCtrl[addr>>38].access_counter++;

    // write not allocate
    // do not load data into current level of cache, directly write next level
    // have not tested, should be good
    if(read == WRITE_ && config_.write_allocate == WRITENLLOC)
    {
      // printf("%lx Miss. Write not allocated.\n", addr);
      lower_->HandleRequest(addr, bytes, read, content, hit, cycle);
    }
    // write allocate or read or read prefetch
    else
    {
      // choose a victim to replace, and do not write to the next level of the cache 
      ReplaceAlgorithm(addr, cycle, read);
      // if prefetch load it from VM
      if(read == READ_PREFETCH) VMEM.HandleRequest(addr, bytes, READ_PREFETCH, content, hit, cycle);
      // else, read from lower cache
      else lower_->HandleRequest(addr, bytes, READ_, content, hit, cycle);
      // put the new cache line into cache(shall we do this in ReplaceAlgorithm? -- I think so)
    }
    
    // Decide on whether a prefetch should take place.
    // printf(" -----------------------------------  fx\n");
    if (PrefetchDecision(read, evicted)) 
    {
      // Fetch the other data via HandleRequest() recursively.
      // To distinguish from the normal requests,
      // the 2|3 is employed for prefetched write|read data
      // while the 0|1 for normal ones.
      #ifdef DEBUG_PREFETCH
      printf("cache level %d, decide to do prefetch....\n", level);
      #endif
      stats_.prefetch_num++;
      PrefetchAlgorithm();
    }
  }

  // bypass here 
  // Fetch from the lower layer
  else
  {
    // If a victim is selected, replace it.
    // Something else should be done here
    // according to your replacement algorithm.
    lower_->HandleRequest(addr, bytes, read, content, hit, cycle);
  }
}

// ---- decide whether bypass ----
int Cache::BypassDecision(uint64_t addr, int read) 
{
  return false;
  /*
  if(read == READ_PREFETCH)  return false;
  uint64_t vpn = addr >> 38; //assumed that the addr space is 48 bit
  float result = (float)partCtrl[vpn].miss_num / partCtrl[vpn].access_counter;
  if(partCtrl[vpn].miss_num>BYPASS_MISS && result>BYPASS_THRESHOLD)
    return true;
  else
    return false;
*/
}

// -- obsolete
void Cache::PartitionAlgorithm() 
{
}

// ---- whether hit or miss ----
// Hit, return true; miss, return false.
bool Cache::ReplaceDecision(uint64_t addr, int &target, int read) 
{
  #ifdef DEBUG
  printf("cache level %d, replacement decision\n", level);
  #endif

  uint64_t vpn = (addr << (64-s-b)) >> (64-s);
  uint64_t vpo = (addr << (64-b)) >> (64-b);
  uint64_t flag = addr >> (s+b);
  target = -1;
  bool ishit = false;
  for(int j = 0; j < config_.associativity; ++j)
  {
    // hit the address
    if(store[vpn][j].valid && store[vpn][j].flag == flag)
    {
      target = j;
      ishit = true;
      // if this is a prefetch hit
      if(store[vpn][j].prefetch && read != READ_PREFETCH)
      {
        stats_.useful_prefetch++;
        #ifdef DEBUG
        printf("hit because of useful prefetch\n");
        #endif
      }
      break;
    }
  }

  // detect harmful prefetch
  // addr not hit because of evicted by prefetch
  if(!ishit && evict_queue.IsMember(ROUND_TO_BASE(addr)))
  {
    #ifdef DEBUG
    printf("rounded address %lx\n", ROUND_TO_BASE(addr));
    printf("miss because of harmful prefetch\n");
    #endif
    stats_.harmful_prefetch++;
  }

  ReplaceUpdate(ishit, addr, target);
  return ishit;
}

void Cache::ReplaceUpdate(bool ishit, uint64_t addr, int target)
{
  if(ishit == false)
    return;
  //for bypassing
  uint64_t vpn = addr >> 38;
  partCtrl[vpn].access_counter++;
  switch(ReplacePolicy)
  {
  case LRU:
    return LRU_update(ishit, addr, target);
  case LFU:
    return LFU_update(ishit, addr, target);
  case FIFO:
    return FIFO_update(ishit, addr, target);
  case RANDOM:
    return RANDOM_update(ishit, addr, target);
  case PDP:
    PDP_update(ishit, addr, target);
    break;
  default:
    printf("error: replace algorithm have not implemented!\n");
    exit(1);
  }
}

// ---- dispatch the replace algorithm ----
void Cache::ReplaceAlgorithm(uint64_t addr, int &cycle, int read)
{
  uint64_t evicted_addr;
  switch(ReplacePolicy)
  {
  case LRU:
    evicted_addr = ReplaceAlgorithm_LRU(addr, cycle, read);
    break;
  case LFU:
    evicted_addr = ReplaceAlgorithm_LFU(addr, cycle, read);
    break;
  case FIFO:
    evicted_addr = ReplaceAlgorithm_FIFO(addr, cycle, read);
    break;
  case RANDOM:
    evicted_addr = ReplaceAlgorithm_RANDOM(addr, cycle, read);
    break;
  case PDP:
    evicted_addr = ReplaceAlgorithm_PDP(addr, cycle, read);
    break;
  default:
    printf("error: replace algorithm have not implemented!\n");
    exit(1);
  }

  // evicted by prefetch
  if(read == READ_PREFETCH) evict_queue.push(evicted_addr);
}

// ---- whether prefetch ----
int Cache::PrefetchDecision(int read, int evicted) 
{
  // if already prefetching, do not prefetch again
  if(read == READ_PREFETCH) return FALSE;
  // if evicted dirty write, do not prefetch 
  if(evicted == EVICTED) return FALSE;
  switch(PrefetchPolicy)
  {
  case ALWAYS:
    return TRUE;
  case TAGGED:
    return PrefetchDecision_TAGGED();
  case LEARNED:
    return PrefetchDecision_LEARNED();
  default:
    return FALSE;
  }
}


// ---- how to prefetch ----
void Cache::PrefetchAlgorithm() 
{
  switch(PrefetchPolicy)
  {
  case ALWAYS:
    AlwaysPrefetch(PREFETCHNUM);
    return;
  case TAGGED:
    TaggedPrefetch(PREFETCHNUM);
  case LEARNED:
    LearnedPrefetch(PREFETCHNUM);
  default:
    return;
  }
}


// ---- get cache configuration ----
void Cache::GetConfig(CacheConfig &cc)
{
  cc = config_;
}

// ---- show the access state ----
void Cache::ShowStat()
{
  printf("level %d\n", level);
  // printf("access counter: every time we access this level of cache\n");
  // printf("access cycle: total latency happened in this level of cache\n");
  printf("access cycle:     %d\n", stats_.access_cycle);
  printf("access counter:   %d\n", stats_.access_counter);
  // printf("hit number: the number we get a cache hit\n");
  printf("hit numnber:      %d\n", stats_.access_counter - stats_.miss_num);
  // printf("miss number: the number we get a cache miss\n");
  printf("miss numnber:     %d\n", stats_.miss_num);
  printf("warm up number:   %d\n", stats_.warmup_num);
  // printf("replace number: number of times that a old line is evicted\n");
  printf("replace number:   %d\n", stats_.replace_num);
  // printf("fetch number: number of times that we fetch from the next level of cache\n");
  printf("fetch number:     %d\n", stats_.fetch_num);
  // printf("fetch number - replace number = cold start number\n");
  // printf("prefetch number: numbers that we perform prefetch\n");
  printf("prefetch number:  %d\n", stats_.prefetch_num);
  printf("useful prefetch:  %d\n", stats_.useful_prefetch);
  printf("harmful prefetch: %d\n\n", stats_.harmful_prefetch);
  return;
}

void Cache::ShowConfig()
{
  printf("level:          %d\n", level);
  uint64_t size;
  int associativity;
  int set_num; // Number of cache sets
  int line_size;
  printf("size:           %lu\n", config_.size);
  printf("associativity:  %d\n", config_.associativity);
  printf("set number:     %d\n", config_.set_num);
  printf("line size:      %d\n\n", config_.line_size);
}