#ifndef CACHE_CACHE_H_
#define CACHE_CACHE_H_

#include <stdint.h>
#include "storage.h"
#include "def.h"

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
  bool dirty;
  bool reused;
  int rd;
} Entry;

class Cache: public Storage 
{
 public:
  Cache() {}
  ~Cache() {}

  void init(int _level, uint64_t _size, int _ass, int _setnum, int _wt, int _wa, int _buscyc, int _hitcyc, Storage *_low);
  // Sets & Gets
  void SetConfig(CacheConfig cc);
  void GetConfig(CacheConfig &cc);
  void SetLower(Storage *ll) { lower_ = ll; }
  // Main access process
  void HandleRequest(uint64_t addr, int bytes, int read,
                     unsigned char *content, int &hit, int &cycle);
  void ShowStat();

 private:
  // Bypassing
  int BypassDecision();
  // Partitioning
  void PartitionAlgorithm();
  // Replacement
  bool ReplaceDecision(uint64_t addr, int &target);
  void ReplaceAlgorithm(uint64_t addr, int &cycle, int read);

  void ReplaceUpdate(bool ishit, uint64_t addr, int target);
  
  void LRU_update(bool ishit, uint64_t addr, int target);
  void LFU_update(bool ishit, uint64_t addr, int target);
  void FIFO_update(bool ishit, uint64_t addr, int target);
  void RANDOM_update(bool ishit, uint64_t addr, int target);
  void PDP_update(bool ishit, uint64_t addr, int target);

  void ReplaceAlgorithm_LRU(uint64_t addr, int &cycle, int read);
  void ReplaceAlgorithm_LFU(uint64_t addr, int &cycle, int read);
  void ReplaceAlgorithm_RANDOM(uint64_t addr, int &cycle, int read);
  void ReplaceAlgorithm_FIFO(uint64_t addr, int &cycle, int read);
  void ReplaceAlgorithm_PDP(uint64_t addr, int &cycle, int read);

  // Prefetching
  int PrefetchDecision();
  void PrefetchAlgorithm();

  CacheConfig config_;
  Storage *lower_;
  DISALLOW_COPY_AND_ASSIGN(Cache);

  Entry store[SETSIZE_LIMIT][BLOCKNUM_LIMIT];
  int s;
  int b;
  uint64_t timestamp;
  int ReplacePolicy;
  int level;
};

#endif //CACHE_CACHE_H_ 
