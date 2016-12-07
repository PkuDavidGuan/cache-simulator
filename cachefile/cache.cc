#include "cache.h"
#include "def.h"
#include "string.h"
#include <stdlib.h>

// ---- cache constructor ----
void Cache::init(int _level, uint64_t _size, int _ass, int _setnum, 
  int _wt, int _wa, int _buscyc, int _hitcyc, Storage *_low)
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

  for(int i = 0; i < _setnum; ++i)
    for(int j = 0; j < _ass; ++j)
    {
      store[i][j].valid = false;
      store[i][j].flag = 0;
      store[i][j].recent = 0;
      store[i][j].dirty = false;
      memset(store[i][j].c, 0, sizeof(store[i][j].c));
    }

  s = b = -1;
  int linesize = _size/_ass/_setnum;
  config_.line_size = linesize;
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
                          unsigned char *content, int &hit, int &cycle) 
{
  // every time handle a request, add timestamp
  #ifdef DEBUG
  printf("\n\ncache level %d, handle request at addr: %lx", level, addr);
  if(read == READ_) printf("  READ\n");
  else printf("  WRITE\n");
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
  cycle += latency_.hit_latency + latency_.bus_latency;

  // storage status update
  stats_.access_cycle += latency_.hit_latency + latency_.bus_latency;
  (stats_.access_counter)++;

  // Decide on whether a bypass should take place.
  if (!BypassDecision()) 
  {

    // Generate some infomation for the data partition.
    PartitionAlgorithm();

    // Check whether the required data already exist.
    int targetaddr;
    uint64_t vpn = (addr << (64-s-b)) >> (64-s);
    uint64_t vpo = (addr << (64-b)) >> (64-b);

    // hit or miss
    // tested
    if (ReplaceDecision(addr, targetaddr)) 
    {
      // Hit!
      // update the storage status
      #ifdef DEBUG
      printf("cache level %d, ", level);
      if(read == READ_) printf("READ hit\n");
      else printf("WRITE hit\n");
      #endif    
      hit = 1;

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
    else printf("WRITE miss\n");
    #endif 
    hit = 0;
    (stats_.miss_num)++;
    (stats_.fetch_num)++;

    // write not allocate
    // do not load data into current level of cache, directly write next level
    // have not tested, should be good
    if(read == WRITE_ && config_.write_allocate == WRITENLLOC)
    {
      // printf("%lx Miss. Write not allocated.\n", addr);
      lower_->HandleRequest(addr, bytes, read, content, hit, cycle);
    }
    // write allocate or read
    else
    {
      // choose a victim to replace, and do not write to the next level of the cache 
      ReplaceAlgorithm(addr, cycle, read);
      // load it from next level of cache 
      // note that here we use READ_, instead of previous read
      lower_->HandleRequest(addr, bytes, READ_, content, hit, cycle);
      // put the new cache line into cache(shall we do this in ReplaceAlgorithm? -- I think so)
    }
    
    // Decide on whether a prefetch should take place.
    if (PrefetchDecision()) 
    {
      // Fetch the other data via HandleRequest() recursively.
      // To distinguish from the normal requests,
      // the 2|3 is employed for prefetched write|read data
      // while the 0|1 for normal ones.
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
    ;
  }
}

// ---- decide whether bypass ----
int Cache::BypassDecision() 
{
  return FALSE;
}

// -- obsolete
void Cache::PartitionAlgorithm() 
{
}

// ---- whether hit or miss ----
// Hit, return true; miss, return false.
bool Cache::ReplaceDecision(uint64_t addr, int &target) 
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
      break;
    }
  }
  ReplaceUpdate(ishit, addr, target);
  return ishit;
}

void Cache::ReplaceUpdate(bool ishit, uint64_t addr, int target)
{
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
  default:
    printf("error: replace algorithm have not implemented!\n");
    exit(1);
  }
}

void Cache::LRU_update(bool ishit, uint64_t addr, int target)
{
  uint64_t vpn = (addr << (64-s-b)) >> (64-s);
  if(ishit) store[vpn][target].recent = timestamp;
  return;
}

// ---- dispatch the replace algorithm ----
void Cache::ReplaceAlgorithm(uint64_t addr, int &cycle, int read)
{
  switch(ReplacePolicy)
  {
  case LRU:
    return ReplaceAlgorithm_LRU(addr, cycle, read);
  case LFU:
    return ReplaceAlgorithm_LFU(addr, cycle, read);
  case FIFO:
    return ReplaceAlgorithm_FIFO(addr, cycle, read);
  case RANDOM:
    return ReplaceAlgorithm_RANDOM(addr, cycle, read);
  default:
    printf("error: replace algorithm have not implemented!\n");
    exit(1);
  }
}

// ---- LRU policy ----
// find a lru entry, replace it 
// -- here we only need to mark the flag as the new flag
// if the replaced entry is a dirty entry and we are implementing write back
//    then write it back
// if the replaced entry is not dirty, simply replace it 
void Cache::ReplaceAlgorithm_LRU(uint64_t addr, int &cycle, int read)
{
  #ifdef DEBUG
  printf("cache level %d, LRU replacement\n", level);
  #endif

  uint64_t vpn = (addr << (64-s-b)) >> (64-s);
  uint64_t vpo = (addr << (64-b)) >> (64-b);
  uint64_t flag = addr >> (s+b);
  bool voidblock = false;
  int lru = 0;           
  int temphit;

  // cold start?
  for(int j = 0; j < config_.associativity; ++j)
  {
    if(store[vpn][j].valid == false)
    {
      
      voidblock = true;
      lru = j;
      //printf("Prime: %d\n", lru);
      break;
    }
  }

  #ifdef DEBUG
  if(voidblock) printf("cold start, choose a empty cecheline: set %d, block %d\n", (int)vpn, lru);
  #endif

  // no empty entry, find lru
  // tested
  if(voidblock == false)
  {
    stats_.replace_num++;
    uint64_t minnum = TIMESTAMP_LIMIT;
    for(int j = 0; j < config_.associativity; ++j)
    {
      if(store[vpn][j].recent < minnum)
      {
        minnum = store[vpn][j].recent;
        lru = j;
      }
    }
    #ifdef DEBUG
    printf("evict cecheline: set %d, block %d\n", (int)vpn, lru);
    #endif

    //if write back and the entry is dirty, update the lower cache
    // tested
    if(config_.write_through == WRITEBACK && store[vpn][lru].dirty)
    {
      #ifdef DEBUG
      printf("cache set %d, block %d, dirty write, write to next level of cache\n", (int)vpn, lru);
      #endif
      //write back the sacrified page
      lower_->HandleRequest((store[vpn][lru].flag << (s+b))+(vpn << b), 1 << b, 0, store[vpn][lru].c, temphit, cycle);
    }
  }

  // printf("setting cache line %d valid\n", lru);
  store[vpn][lru].valid = true;
  store[vpn][lru].recent = timestamp;
  #ifdef DEBUG
  printf("cacle level %d, vpn %d, lru %d, increase timestamp %lu\n", level, (int)vpn, lru, timestamp);
  #endif
  store[vpn][lru].flag = flag;
  if(read == READ_) store[vpn][lru].dirty = false;
  else store[vpn][lru].dirty = true;
}


// ---- whether prefetch ----
int Cache::PrefetchDecision() 
{
  return FALSE;
}


// ---- how to prefetch ----
void Cache::PrefetchAlgorithm() 
{
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
  printf("access counter:   %d\n", stats_.access_counter);
  // printf("hit number: the number we get a cache hit\n");
  printf("hit numnber:      %d\n", stats_.access_counter - stats_.miss_num);
  // printf("miss number: the number we get a cache miss\n");
  printf("miss numnber:     %d\n", stats_.miss_num);
  // printf("access cycle: total latency happened in this level of cache\n");
  printf("access cycle:     %d\n", stats_.access_cycle);
  // printf("replace number: number of times that a old line is evicted\n");
  printf("replace number:   %d\n", stats_.replace_num);
  // printf("fetch number: number of times that we fetch from the next level of cache\n");
  printf("fetch number:     %d\n", stats_.fetch_num);
  // printf("fetch number - replace number = cold start number\n");
  // printf("prefetch number: numbers that we perform prefetch\n");
  printf("prefetch number:  %d\n\n", stats_.prefetch_num);
  return;
}