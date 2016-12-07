// ---- main function for part1 ----

#include "memory.h"
#include "cache.h"
#include "def.h"
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

Memory VMEM;
Cache L1;
Cache L2;
Cache L3;

// ----
int main()
{
  // set up VMEM -- done in the constructor
  // set up L3 cache
  // L3.init(3, L3SIZE, L3ASS, L3SETNUM, L3WT, L3WA, L3BUSCYC, L3HITCYC, &VMEM);
  // set up L2 cache
  L2.init(2, L2SIZE, L2ASS, L2SETNUM, L2WT, L2WA, L2BUSCYC, L2HITCYC, &VMEM /*&L3*/);
  // set up L1 cache
  L1.init(1, L1SIZE, L1ASS, L1SETNUM, L1WT, L1WA, L1BUSCYC, L1HITCYC, &L2);

  int testf = TESTFILE;
  string tracefile;
  
  // read file
  // --  note here we do it streamly
  switch(testf)
  {
  case TEST:
    tracefile = "../trace/test.trace";
    break;
  case GCCX:
    tracefile = "../trace/gccx.trace";
    break;
  case BZIPX:
    tracefile = "../trace/bzipx.trace";
    break;
  case SWIMX:
    tracefile = "../trace/swimx.trace";
    break;
  case SIXPACKX:
    tracefile = "../trace/sixpackx.trace";
    break;
  case QUICKSORTX:
    tracefile = "../trace/1.trace";
    break;
  default:
    tracefile = "../trace/test.trace";
    break;
  }
  
  ifstream fd;
  fd.open(tracefile.c_str());
  
  uint64_t addr;
  char type_;
  char content[100];
  int hit;
  int totalcycle = 0;
  int tmpbyte = 4;
  char tmpstr[100];

  char buf[5000];
  while(fd.getline(buf, 5000))
  {
    sscanf(buf, "%c %lx %s", &type_, &addr, tmpstr);
    // printf("%c 0x%lx\n", type_, addr);
    if(type_ == 'r')
    {
      L1.HandleRequest(addr, tmpbyte, READ_, (unsigned char *)content, hit, totalcycle);
    }
    else
    {
      L1.HandleRequest(addr, tmpbyte, WRITE_, (unsigned char *)content, hit, totalcycle);
    }
  }

  // output latency
  printf("---------------------------------------------------------\n");
  printf("Total access cycle: %d\n\n", totalcycle);
  // output L1 hit time
  L1.ShowStat();
  // output L2 hit time 
  L2.ShowStat();
  // output L3 hit time
  // L3.ShowStat();
  // VM stat
  VMEM.ShowStat();
  return 0;
}