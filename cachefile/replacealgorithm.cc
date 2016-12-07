#include "cache.h"
#include "def.h"
#include <stdlib.h>
#include <queue>
#include <iostream>
#include <time.h>
using namespace std;

queue <int> cache_queue[SETSIZE_LIMIT];
// ---- LFU ----
void Cache::ReplaceAlgorithm_LFU(uint64_t addr, int &cycle, int read)
{
  #ifdef DEBUG
  printf("cache level %d, LFU replacement\n", level);
  #endif

  uint64_t vpn = (addr << (64-s-b)) >> (64-s);
  uint64_t vpo = (addr << (64-b)) >> (64-b);
  uint64_t flag = addr >> (s+b);
  bool voidblock = false;
  int lfu = 0;           
  int temphit;

  // cold start
  for(int j = 0; j < config_.associativity; ++j)
  {
    if(store[vpn][j].valid == false)
    {
      
      voidblock = true;
      lfu = j;
      //printf("Prime: %d\n", lru);
      break;
    }
  }

  #ifdef DEBUG
  if(voidblock) printf("cold start, choose a empty cecheline: set %d, block %d\n", (int)vpn, lfu);
  #endif

  // no empty entry, find lfu
  // tested
  if(voidblock == false)
  {
    stats_.replace_num++;
    uint64_t minnum = TIMESTAMP_LIMIT;
    for(int j = 0; j < config_.associativity; ++j)
    {
      if(store[vpn][j].frequency < minnum)
      {
        minnum = store[vpn][j].frequency;
        lfu = j;
      }
    }
    #ifdef DEBUG
    printf("evict cecheline: set %d, block %d\n", (int)vpn, lfu);
    #endif

    //if write back and the entry is dirty, update the lower cache
    // tested
    if(config_.write_through == WRITEBACK && store[vpn][lfu].dirty)
    {
      #ifdef DEBUG
      printf("cache set %d, block %d, dirty write, write to next level of cache\n", (int)vpn, lfu);
      #endif
      //write back the sacrified page
      lower_->HandleRequest((store[vpn][lfu].flag << (s+b))+(vpn << b), 1 << b, 0, store[vpn][lfu].c, temphit, cycle);
    }
  }

  // printf("setting cache line %d valid\n", lru);
  store[vpn][lfu].valid = true;
  store[vpn][lfu].frequency = 1;
  #ifdef DEBUG
  printf("cacle level %d, vpn %d, lfu %d, increase timestamp %lu\n", level, (int)vpn, lfu, timestamp);
  #endif
  store[vpn][lfu].flag = flag;
  if(read == READ_) store[vpn][lfu].dirty = false;
  else store[vpn][lfu].dirty = true;
}

// if needed
void Cache::LFU_update(bool ishit, uint64_t addr, int target)
{
	if(ishit)
		store[(addr << (64-s-b)) >> (64-s)][target].frequency++;
}

// ---- FIFO ----
void Cache::ReplaceAlgorithm_FIFO(uint64_t addr, int &cycle, int read)
{
  #ifdef DEBUG
  printf("cache level %d, FIFO replacement\n", level);
  #endif

  uint64_t vpn = (addr << (64-s-b)) >> (64-s);
  uint64_t vpo = (addr << (64-b)) >> (64-b);
  uint64_t flag = addr >> (s+b);
  bool voidblock = false;
  int lfu = 0;           
  int temphit;

  // cold start
  for(int j = 0; j < config_.associativity; ++j)
  {
    if(store[vpn][j].valid == false)
    {
      
      voidblock = true;
      lfu = j;
      //printf("Prime: %d\n", lru);
      break;
    }
  }

  #ifdef DEBUG
  if(voidblock) printf("cold start, choose a empty cecheline: set %d, block %d\n", (int)vpn, lfu);
  #endif

  // no empty entry, find lfu
  // tested
  if(voidblock == false)
  {
    stats_.replace_num++;
    
    lfu = cache_queue[vpn].front();
    cache_queue[vpn].pop();
    #ifdef DEBUG
    printf("evict cecheline: set %d, block %d\n", (int)vpn, lfu);
    #endif

    //if write back and the entry is dirty, update the lower cache
    // tested
    if(config_.write_through == WRITEBACK && store[vpn][lfu].dirty)
    {
      #ifdef DEBUG
      printf("cache set %d, block %d, dirty write, write to next level of cache\n", (int)vpn, lfu);
      #endif
      //write back the sacrified page
      lower_->HandleRequest((store[vpn][lfu].flag << (s+b))+(vpn << b), 1 << b, 0, store[vpn][lfu].c, temphit, cycle);
    }
  }
  store[vpn][lfu].valid = true;
  cache_queue[vpn].push(lfu);
  #ifdef DEBUG
  printf("cacle level %d, vpn %d, fifo %d, increase timestamp %lu\n", level, (int)vpn, lfu, timestamp);
  #endif
  store[vpn][lfu].flag = flag;
  if(read == READ_) store[vpn][lfu].dirty = false;
  else store[vpn][lfu].dirty = true;
}

// if needed
void Cache::FIFO_update(bool ishit, uint64_t addr, int target)
{
	if(ishit)
		cache_queue[(addr << (64-s-b)) >> (64-s)].push(target);
}

// ---- RANDOM ----
void Cache::ReplaceAlgorithm_RANDOM(uint64_t addr, int &cycle, int read)
{
  #ifdef DEBUG
  printf("cache level %d, RANDOM replacement\n", level);
  #endif

  uint64_t vpn = (addr << (64-s-b)) >> (64-s);
  uint64_t vpo = (addr << (64-b)) >> (64-b);
  uint64_t flag = addr >> (s+b);
  bool voidblock = false;
  int lfu = 0;           
  int temphit;

  // cold start
  for(int j = 0; j < config_.associativity; ++j)
  {
    if(store[vpn][j].valid == false)
    {
      
      voidblock = true;
      lfu = j;
      //printf("Prime: %d\n", lru);
      break;
    }
  }

  #ifdef DEBUG
  if(voidblock) printf("cold start, choose a empty cecheline: set %d, block %d\n", (int)vpn, lfu);
  #endif

  // no empty entry, find lfu
  // tested
  if(voidblock == false)
  {
    stats_.replace_num++;
    
    lfu = rand() % config_.associativity;
    #ifdef DEBUG
    printf("evict cecheline: set %d, block %d\n", (int)vpn, lfu);
    #endif

    //if write back and the entry is dirty, update the lower cache
    // tested
    if(config_.write_through == WRITEBACK && store[vpn][lfu].dirty)
    {
      #ifdef DEBUG
      printf("cache set %d, block %d, dirty write, write to next level of cache\n", (int)vpn, lfu);
      #endif
      //write back the sacrified page
      lower_->HandleRequest((store[vpn][lfu].flag << (s+b))+(vpn << b), 1 << b, 0, store[vpn][lfu].c, temphit, cycle);
    }
  }
  store[vpn][lfu].valid = true;
  #ifdef DEBUG
  printf("cacle level %d, vpn %d, random %d, increase timestamp %lu\n", level, (int)vpn, lfu, timestamp);
  #endif
  store[vpn][lfu].flag = flag;
  if(read == READ_) store[vpn][lfu].dirty = false;
  else store[vpn][lfu].dirty = true;
}

// if needed
void Cache::RANDOM_update(bool ishit, uint64_t addr, int target)
{}