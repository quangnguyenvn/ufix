#include "utils.h"

#include "Clock.h"

namespace ufix {

  Clock::Clock() {
    current_index = -1;
    for (int i = 0; i < NUM_TIMESTAMP; i++) {
      utc_time[i] = (char *) mem_alloc("Init wall clock", 32*sizeof(char));
    }
  }

  void Clock::update_utc_time() {
    time_t ts;
    time(&ts);
    int new_index = current_index + 1;
    if (new_index == NUM_TIMESTAMP) new_index = 0;
    
    timestamp[new_index] = (ULONG64) ts;
    tm * ptm = gmtime(&ts);
    int sec = (ptm->tm_sec < 60)?ptm->tm_sec:59;

    sprintf(utc_time[new_index], "%04d%02d%02d-%02d:%02d:%02d.%03d",
            ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
            ptm->tm_hour, ptm->tm_min, sec, 0);
    _STORE_MEMORY_BAR_;
    current_index = new_index;
  }

  const char * Clock::get_utc_time() {
    return utc_time[current_index];
  }

  ULONG64 Clock::get_timestamp() {
    return timestamp[current_index];
  }
}
