#include "cache.h"
#include "def.h"

// Main access process
// [in]  addr: access address
// [in]  bytes: target number of bytes
// [in]  read: 0|1 for write|read
//             3|4 for write|read in prefetch
// [i|o] content: in|out data
// [out] hit: 0|1 for miss|hit
// [out] cycle: total access cycle
void Cache::HandleRequest(uint64_t addr, int bytes, int read,
                          char *content, int &hit, int &cycle) {

  // Decide on whether a bypass should take place.
  if (!BypassDecision()) {

    // Generate some infomation for the data partition.
    PartitionAlgorithm();

    // Check whether the required data already exist.
    if (ReplaceDecision()) {
      // Hit!
      // Something else should be done here
      // according to your replacement algorithm.
      return;
    }

    // Choose a victim for the current request.
    ReplaceAlgorithm();

    // Decide on whether a prefetch should take place.
    if (PrefetchDecision()) {
      // Fetch the other data via HandleRequest() recursively.
      // To distinguish from the normal requests,
      // the 2|3 is employed for prefetched write|read data
      // while the 0|1 for normal ones.
      PrefetchAlgorithm();
    }
  }
  // Fetch from the lower layer

  // If a victim is selected, replace it.
  // Something else should be done here
  // according to your replacement algorithm.
}

int Cache::BypassDecision() {
  return FALSE;
}

void Cache::PartitionAlgorithm() {
}

int Cache::ReplaceDecision() {
  return TRUE;
  //return FALSE;
}

void Cache::ReplaceAlgorithm(){
}

int Cache::PrefetchDecision() {
  return FALSE;
}

void Cache::PrefetchAlgorithm() {
}

