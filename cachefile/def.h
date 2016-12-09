#ifndef CACHE_DEF_H_
#define CACHE_DEF_H_

#define TRUE 1
#define FALSE 0
#define READ_ 1
#define WRITE_ 0
#define READ_PREFETCH 4

#define TIMESTAMP_LIMIT 0xffffffffffff0000
#define SETSIZE_LIMIT 0x200000
#define BLOCKNUM_LIMIT 8
#define LINESIZE_LIMIT 100
#define MEMSIZE 0x10000
#define PARTITIONNUM 1024
#define BYPASS_THRESHOLD 0.5
#define BYPASS_MISS 100

// trigers for the test& debug
#define DEBUG
// #define TEST
#define DEBUG_PREFETCH
#define CACHE

// trigers for select test file
#define TESTX       0
#define GCCX        1
#define SWIMX       2
#define BZIPX       3
#define SIXPACKX    4
#define QUICKSORTX  5
#define TESTFILE    QUICKSORTX

// cache replacement policy
#define LRU             0
#define LFU             1
#define RANDOM          2
#define FIFO            3
#define PDP             4
#define REPLACEPOLICY   LRU
#define PD              16

// prefetch number
#define PREFETCHNUM 1

// prefetch policy
#define NEVER           0
#define ALWAYS          1
#define TAGGED          2
#define LEARNED         3
#define PREFETCHPOLICY  NEVER

// write policy
#define WRITEBACK       0
#define WRITETHROUGH    1
#define WRITEALLOC      1
#define WRITENLLOC      0

// test configuration
#define L1SIZETEST 0X8
#define L1ASSTEST 2
#define L1LINESIZETEST 4
#define L2SIZETEST 0x16
#define L2ASSTEST 2
#define L2LINESIZETEST 4 

// L1 cache
#ifdef TEST
#define L1SIZE L1SIZETEST 
#define L1ASS L1ASSTEST 
#define L1LINESIZE L1LINESIZETEST 
#else
#define L1SIZE 0x8000             // 32K
#define L1ASS 8
#define L1LINESIZE 64
#endif

#define L1SETNUM ((L1SIZE) / (L1LINESIZE * L1ASS)) // should be only one set
#define L1WT WRITEBACK
#define L1WA WRITEALLOC
#define L1BUSCYC 0
#define L1HITCYC 4

// L2 cache
#ifdef TEST
#define L2SIZE L2SIZETEST
#define L2ASS L2ASSTEST 
#define L2LINESIZE L2LINESIZETEST 
#else
#define L2SIZE 0x64000             // 256K
#define L2ASS 8
#define L2LINESIZE 64
#endif

#define L2SETNUM ((L2SIZE) / (L2LINESIZE * L2ASS)) // should be two sets
#define L2WT WRITEBACK
#define L2WA WRITEALLOC
#define L2BUSCYC 6
#define L2HITCYC 5

// L3 cache
#define L3SIZE 0x8000000            // 128M 
#define L3ASS 8
#define L3LINESIZE 64
#define L3SETNUM ((L3SIZE) / (L3LINESIZE * L3ASS))
#define L3WT WRITEBACK
#define L3WA WRITEALLOC
#define L3BUSCYC 6
#define L3HITCYC 41

#endif //CACHE_DEF_H_
