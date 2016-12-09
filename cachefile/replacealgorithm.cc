#include "cache.h"
#include "def.h"
#include <stdlib.h>
#include <queue>
#include <iostream>
#include <time.h>
using namespace std;

queue <int> cache_queue[SETSIZE_LIMIT];

// ---- LRU policy ----
// find a lru entry, replace it 
// -- here we only need to mark the flag as the new flag
// if the replaced entry is a dirty entry and we are implementing write back
//    then write it back
// if the replaced entry is not dirty, simply replace it 
uint64_t Cache::ReplaceAlgorithm_LRU(uint64_t addr, int &cycle, int read)
{
  #ifdef DEBUG
  printf("cache level %d, LRU replacement\n", level);
  #endif

  uint64_t vpn = (addr << (64-s-b)) >> (64-s);
  uint64_t vpo = (addr << (64-b)) >> (64-b);
  uint64_t flag = addr >> (s+b);
  uint64_t base_addr = ((addr >> b) << b);
  uint64_t evicted_addr;
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
    printf("evict cecheline: set %d, block %d, addr %lx\n", (int)vpn, lru, store[vpn][lru].base_addr);
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
  store[vpn][lru].flag = flag;

  // get the evicted addr
  evicted_addr = store[vpn][lru].base_addr;
  // update new base addr
  store[vpn][lru].base_addr = base_addr;

  if(read == READ_)
  {
    store[vpn][lru].dirty = false;
    store[vpn][lru].prefetch = false; 
  }
  else if(read == WRITE_)
  {
    store[vpn][lru].dirty = true;
    store[vpn][lru].prefetch = false;
  }
  else if(read == READ_PREFETCH)
  {
    store[vpn][lru].prefetch = true;
  }

  return evicted_addr;
}

void Cache::LRU_update(bool ishit, uint64_t addr, int target)
{
  uint64_t vpn = (addr << (64-s-b)) >> (64-s);
  if(ishit) store[vpn][target].recent = timestamp;
  return;
}

// ---- LFU ----
uint64_t Cache::ReplaceAlgorithm_LFU(uint64_t addr, int &cycle, int read)
{
  #ifdef DEBUG
  printf("cache level %d, LFU replacement\n", level);
  #endif

  uint64_t vpn = (addr << (64-s-b)) >> (64-s);
  uint64_t vpo = (addr << (64-b)) >> (64-b);
  uint64_t flag = addr >> (s+b);
  uint64_t base_addr = ((addr >> b) << b);
  uint64_t evicted_addr;
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
  if(stats_.access_counter % 32 == 0)
  {	
  	for(int j = 0; j < config_.associativity; ++j)
  	{
      	store[vpn][j].frequency >>= 1;
  	}
  }
  store[vpn][lfu].frequency = 1;

  // get the evicted addr
  evicted_addr = store[vpn][lfu].base_addr;
  // update new base addr
  store[vpn][lfu].base_addr = base_addr;

  #ifdef DEBUG
  printf("cacle level %d, vpn %d, lfu %d, increase timestamp %lu\n", level, (int)vpn, lfu, timestamp);
  #endif
  store[vpn][lfu].flag = flag;
  if(read == READ_)
  {
    store[vpn][lfu].dirty = false;
    store[vpn][lfu].prefetch = false; 
  }
  else if(read == WRITE_)
  {
    store[vpn][lfu].dirty = true;
    store[vpn][lfu].prefetch = false;
  }
  else if(read == READ_PREFETCH)
  {
    store[vpn][lfu].prefetch = true;
  }

  return evicted_addr;
}

// if needed
void Cache::LFU_update(bool ishit, uint64_t addr, int target)
{
	if(ishit)
	{
		store[(addr << (64-s-b)) >> (64-s)][target].frequency++;
		int vpn = (addr << (64-s-b)) >> (64-s);
		if(stats_.access_counter % 32 == 0)
  		{	
  			for(int j = 0; j < config_.associativity; ++j)
  			{
      			if(j != target)
      				store[vpn][j].frequency >>= 1;
  			}
  		}
	}
}

// ---- FIFO ----
uint64_t Cache::ReplaceAlgorithm_FIFO(uint64_t addr, int &cycle, int read)
{
  #ifdef DEBUG
  printf("cache level %d, FIFO replacement\n", level);
  #endif

  uint64_t vpn = (addr << (64-s-b)) >> (64-s);
  uint64_t vpo = (addr << (64-b)) >> (64-b);
  uint64_t flag = addr >> (s+b);
  uint64_t base_addr = ((addr >> b) << b);
  uint64_t evicted_addr;
  bool voidblock = false;
  int fifo = 0;           
  int temphit;

  // cold start
  for(int j = 0; j < config_.associativity; ++j)
  {
    if(store[vpn][j].valid == false)
    {
      
      voidblock = true;
      fifo = j;
      //printf("Prime: %d\n", fifo);
      break;
    }
  }

  #ifdef DEBUG
  if(voidblock) printf("cold start, choose a empty cecheline: set %d, block %d\n", (int)vpn, fifo);
  #endif

  // no empty entry, find fifo
  // tested
  if(voidblock == false)
  {
    stats_.replace_num++;
    
    fifo = cache_queue[vpn].front();
    cache_queue[vpn].pop();
    #ifdef DEBUG
    printf("evict cecheline: set %d, block %d\n", (int)vpn, fifo);
    #endif

    //if write back and the entry is dirty, update the lower cache
    // tested
    if(config_.write_through == WRITEBACK && store[vpn][fifo].dirty)
    {
      #ifdef DEBUG
      printf("cache set %d, block %d, dirty write, write to next level of cache\n", (int)vpn, fifo);
      #endif
      //write back the sacrified page
      lower_->HandleRequest((store[vpn][fifo].flag << (s+b))+(vpn << b), 1 << b, 0, store[vpn][fifo].c, temphit, cycle);
    }
  }
  store[vpn][fifo].valid = true;
  cache_queue[vpn].push(fifo);
  #ifdef DEBUG
  printf("cacle level %d, vpn %d, fifo %d, increase timestamp %lu\n", level, (int)vpn, fifo, timestamp);
  #endif
  store[vpn][fifo].flag = flag;
  // get the evicted addr
  evicted_addr = store[vpn][fifo].base_addr;
  // update new base addr
  store[vpn][fifo].base_addr = base_addr;

  if(read == READ_)
  {
    store[vpn][fifo].dirty = false;
    store[vpn][fifo].prefetch = false; 
  }
  else if(read == WRITE_)
  {
    store[vpn][fifo].dirty = true;
    store[vpn][fifo].prefetch = false;
  }
  else if(read == READ_PREFETCH)
  {
    store[vpn][fifo].prefetch = true;
  }
  return evicted_addr;
}

// if needed
void Cache::FIFO_update(bool ishit, uint64_t addr, int target)
{
	if(ishit)
		cache_queue[(addr << (64-s-b)) >> (64-s)].push(target);
}

// ---- RANDOM ----
uint64_t Cache::ReplaceAlgorithm_RANDOM(uint64_t addr, int &cycle, int read)
{
  #ifdef DEBUG
  printf("cache level %d, RANDOM replacement\n", level);
  #endif
  uint64_t vpn = (addr << (64-s-b)) >> (64-s);
  uint64_t vpo = (addr << (64-b)) >> (64-b);
  uint64_t flag = addr >> (s+b);
  uint64_t base_addr = ((addr >> b) << b);
  uint64_t evicted_addr;
  bool voidblock = false;
  int random_ = 0;           
  int temphit;

  // cold start
  for(int j = 0; j < config_.associativity; ++j)
  {
    if(store[vpn][j].valid == false)
    {
      
      voidblock = true;
      random_ = j;
      //printf("Prime: %d\n", random_);
      break;
    }
  }

  #ifdef DEBUG
  if(voidblock) printf("cold start, choose a empty cecheline: set %d, block %d\n", (int)vpn, random_);
  #endif

  // no empty entry, find random_
  // tested
  if(voidblock == false)
  {
    stats_.replace_num++;
    
    random_ = rand() % config_.associativity;
    #ifdef DEBUG
    printf("evict cecheline: set %d, block %d\n", (int)vpn, random_);
    #endif

    //if write back and the entry is dirty, update the lower cache
    // tested
    if(config_.write_through == WRITEBACK && store[vpn][random_].dirty)
    {
      #ifdef DEBUG
      printf("cache set %d, block %d, dirty write, write to next level of cache\n", (int)vpn, random_);
      #endif
      //write back the sacrified page
      lower_->HandleRequest((store[vpn][random_].flag << (s+b))+(vpn << b), 1 << b, 0, store[vpn][random_].c, temphit, cycle);
    }
  }
  store[vpn][random_].valid = true;
  #ifdef DEBUG
  printf("cacle level %d, vpn %d, random %d, increase timestamp %lu\n", level, (int)vpn, random_, timestamp);
  #endif
  store[vpn][random_].flag = flag;
  
  // get the evicted addr
  evicted_addr = store[vpn][random_].base_addr;
  // update new base addr
  store[vpn][random_].base_addr = base_addr;
  
  if(read == READ_)
  {
    store[vpn][random_].dirty = false;
    store[vpn][random_].prefetch = false; 
  }
  else if(read == WRITE_)
  {
    store[vpn][random_].dirty = true;
    store[vpn][random_].prefetch = false;
  }
  else if(read == READ_PREFETCH)
  {
    store[vpn][random_].prefetch = true;
  }

  return evicted_addr;
}

// if needed
void Cache::RANDOM_update(bool ishit, uint64_t addr, int target)
{}

uint64_t Cache::ReplaceAlgorithm_PDP(uint64_t addr, int &cycle, int read)
{
	#ifdef DEBUG
  printf("cache level %d, PDP replacement\n", level);
  #endif

  uint64_t vpn = (addr << (64-s-b)) >> (64-s);
  uint64_t vpo = (addr << (64-b)) >> (64-b);
  uint64_t flag = addr >> (s+b);
  uint64_t base_addr = ((addr >> b) << b);
  uint64_t evicted_addr;
  bool voidblock = false;
  int pdp = 0;           
  int temphit;

  // cold start
  for(int j = 0; j < config_.associativity; ++j)
  {
    if(store[vpn][j].valid == false)
    {
      
      voidblock = true;
      pdp = j;
      //printf("Prime: %d\n", pdp);
      break;
    }
  }
  #ifdef DEBUG
  if(voidblock) printf("cold start, choose a empty cecheline: set %d, block %d\n", (int)vpn, pdp);
  #endif
  if(voidblock == false)
  {
  	stats_.replace_num++;
  	for(int j = 0; j < config_.associativity; ++j)
  	{
  		if(store[vpn][j].rd == 0)
  		{
  			pdp = j;
  			voidblock = true;
  			break;
  		}
  	}
  }
  // no empty entry, find pdp
  // tested
  if(voidblock == false)
  {
    stats_.replace_num++;
    
    bool allReused = true;
    bool firstInserted = true;
    int maxRD = 0;
    for(int j = 0; j < config_.associativity; ++j)
    {
    	if(store[vpn][j].reused == false)
    	{
    		if(firstInserted)
    		{
    			firstInserted = false;
    			pdp = j;
    		}
    		allReused = false;
    		if(store[vpn][j].rd > maxRD)
    		{
    			maxRD = store[vpn][j].rd;
    			pdp = j;
    		}
    	}
    }
    maxRD = 0;
    if(allReused)
    {
    	for(int j = 0; j < config_.associativity; ++j)
    	{
    		if(store[vpn][j].rd > maxRD)
    		{
    			maxRD = store[vpn][j].rd;
    			pdp = j;	
    		}	
    	}	
    }
    #ifdef DEBUG
    printf("evict cecheline: set %d, block %d\n", (int)vpn, pdp);
    #endif

    //if write back and the entry is dirty, update the lower cache
    // tested
    if(config_.write_through == WRITEBACK && store[vpn][pdp].dirty)
    {
      #ifdef DEBUG
      printf("cache set %d, block %d, dirty write, write to next level of cache\n", (int)vpn, pdp);
      #endif
      //write back the sacrified page
      lower_->HandleRequest((store[vpn][pdp].flag << (s+b))+(vpn << b), 1 << b, 0, store[vpn][pdp].c, temphit, cycle);
    }
  }
  store[vpn][pdp].valid = true;
  store[vpn][pdp].rd = PD;
  store[vpn][pdp].reused = false;
  for(int j = 0; j < config_.associativity; ++j)
  {
  	if(store[vpn][j].rd > 0)
  		store[vpn][j].rd--;
  }
  #ifdef DEBUG
  printf("cacle level %d, vpn %d, random %d, increase timestamp %lu\n", level, (int)vpn, pdp, timestamp);
  #endif
  store[vpn][pdp].flag = flag;
  // get the evicted addr
  evicted_addr = store[vpn][pdp].base_addr;
  // update new base addr
  store[vpn][pdp].base_addr = base_addr;
  if(read == READ_)
  {
    store[vpn][pdp].dirty = false;
    store[vpn][pdp].prefetch = false; 
  }
  else if(read == WRITE_)
  {
    store[vpn][pdp].dirty = true;
    store[vpn][pdp].prefetch = false;
  }
  else if(read == READ_PREFETCH)
  {
    store[vpn][pdp].prefetch = true;
  }

  return evicted_addr;
}

void Cache::PDP_update(bool ishit, uint64_t addr, int target)
{
	int vpn = (addr << (64-s-b)) >> (64-s);
	if(ishit)
	{
		if(store[vpn][target].reused == false)
			store[vpn][target].reused == true;
		store[vpn][target].rd = PD;
		for(int j = 0; j < config_.associativity; ++j)
		{
			if(store[vpn][j].rd > 0)
				store[vpn][j].rd--;
		}
	}
}