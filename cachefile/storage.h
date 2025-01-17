#ifndef CACHE_STORAGE_H_
#define CACHE_STORAGE_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "def.h"

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&); \
  void operator=(const TypeName&)

// Storage access stats
typedef struct StorageStats_ 
{
  int access_counter;   // ++ at hit
  int miss_num;         // ++ at miss
  int access_cycle;     // tatal cycles access at this layer
  int replace_num;      // times of evicting old lines
  int fetch_num;        // times of fetching from lower layer
  int prefetch_num;     // times that we do Prefetch
  int useful_prefetch;  // 
  int harmful_prefetch; //
  int warmup_num;
} StorageStats;

// Storage basic config
typedef struct StorageLatency_ 
{
  int hit_latency; // In cycles
  int bus_latency; // Added to each request
} StorageLatency;

class Storage 
{
 public:
  Storage() 
  {
    memset(&stats_, 0, sizeof(StorageStats));
    memset(&latency_, 0, sizeof(StorageLatency));
  }
  ~Storage() {}

  // Sets & Gets
  void SetStats(StorageStats ss) { stats_ = ss; }
  void GetStats(StorageStats &ss) { ss = stats_; }
  void SetLatency(StorageLatency sl) { latency_ = sl; }
  void GetLatency(StorageLatency &sl) { sl = latency_; }

  // Main access process
  // [in]  addr: access address
  // [in]  bytes: target number of bytes
  // [in]  read: 0|1 for write|read
  //             3|4 for prefetch write|read
  // [i|o] content: in|out data
  // [out] hit: 0|1 for miss|hit
  // [out] cycle: total access cycle
  virtual void HandleRequest(uint64_t addr, int bytes, int read,
                             unsigned char *content, int &hit, int &cycle, int evicted = NOT_EVICTED) = 0;
  virtual void ShowStat() = 0;

 protected:
  StorageStats stats_;
  StorageLatency latency_;
};

#endif //CACHE_STORAGE_H_ 
