#ifndef CACHE_CACHE_H_
#define CACHE_CACHE_H_

#include <stdint.h>
#include <vector>
#include "storage.h"
#include "def.h"

using namespace std;

typedef struct CacheConfig_ 
{
  uint64_t size;
  int associativity;
  int set_num; // Number of cache sets
  int line_size;
  int write_through; // 0|1 for back|through
  int write_allocate; // 0|1 for no-alc|alc
} CacheConfig;

typedef struct Entry_ 
{
  bool valid;
  uint64_t flag;
  unsigned char c[LINESIZE_LIMIT];
  uint64_t recent;
  uint64_t frequency;
  uint64_t base_addr;
  bool dirty;
  bool reused;
  bool prefetch;
  int rd;
} Entry;

typedef struct Partition_ 
{
  int access_counter;
  int miss_num;
} Partition;

class EvictQueue
{
 private:
  vector <uint64_t> equeue;
  int limit;
  int cnt;

 public:
  EvictQueue()
  {
    cnt = 0;
    limit = PREFETCHNUM;
  }

  void push(uint64_t addr)
  {
    if(cnt < limit)
    {
      equeue.push_back(addr);
      cnt++;
    }
    else
    {
      equeue.push_back(addr);
      equeue.erase(equeue.begin());
    }
  }

  bool IsMember(uint64_t addr)
  {
    int i;
    for(i = 0; i < cnt; i++)
    {
      if(equeue[i] == addr) return true;
    }
    return false;
  }
};

class Cache: public Storage 
{
 public:
  Cache() {}
  ~Cache() {}

  void init(int _level, uint64_t _size, int _ass, int _setnum, int _wt, 
            int _wa, int _buscyc, int _hitcyc, Storage *_low, int _PrefetchPolicy);
  // Sets & Gets
  void SetConfig(CacheConfig cc);
  void GetConfig(CacheConfig &cc);
  void SetLower(Storage *ll) { lower_ = ll; }
  // Main access process
  void HandleRequest(uint64_t addr, int bytes, int read,
                     unsigned char *content, int &hit, int &cycle, int evicted = NOT_EVICTED);
  void ShowStat();
  void ShowConfig();

 private:
  // Bypassing
  int BypassDecision(uint64_t addr, int read);
  // Partitioning
  void PartitionAlgorithm();

  // Replacement
  bool ReplaceDecision(uint64_t addr, int &target, int read);
  void ReplaceAlgorithm(uint64_t addr, int &cycle, int read);

  void ReplaceUpdate(bool ishit, uint64_t addr, int target);
  
  void LRU_update(bool ishit, uint64_t addr, int target);
  void LFU_update(bool ishit, uint64_t addr, int target);
  void FIFO_update(bool ishit, uint64_t addr, int target);
  void RANDOM_update(bool ishit, uint64_t addr, int target);
  void PDP_update(bool ishit, uint64_t addr, int target);

  uint64_t ReplaceAlgorithm_LRU(uint64_t addr, int &cycle, int read);
  uint64_t ReplaceAlgorithm_LFU(uint64_t addr, int &cycle, int read);
  uint64_t ReplaceAlgorithm_RANDOM(uint64_t addr, int &cycle, int read);
  uint64_t ReplaceAlgorithm_FIFO(uint64_t addr, int &cycle, int read);
  uint64_t ReplaceAlgorithm_PDP(uint64_t addr, int &cycle, int read);

  // Prefetching
  int PrefetchDecision(int read, int evicted);
  bool PrefetchDecision_TAGGED();
  bool PrefetchDecision_LEARNED();

  void PrefetchAlgorithm();
  void AlwaysPrefetch(int prefetch_num);
  void TaggedPrefetch(int prefetch_num);
  void LearnedPrefetch(int prefetch_num);

  // auxiliary
  uint64_t ROUND_TO_BASE(uint64_t addr){ return ((addr >> b) << b);}
  uint64_t GetPrefetchAddr(uint64_t current_addr, int line_size, int next_line);

  CacheConfig config_;
  Storage *lower_;
  DISALLOW_COPY_AND_ASSIGN(Cache);

  Entry store[SETSIZE_LIMIT][BLOCKNUM_LIMIT];
  Partition partCtrl[PARTITIONNUM];
  int s;
  int b;
  uint64_t timestamp;
  uint64_t current_addr;
  int ReplacePolicy;
  int level;
  int PrefetchPolicy;
  EvictQueue evict_queue;
};

#endif //CACHE_CACHE_H_ 
