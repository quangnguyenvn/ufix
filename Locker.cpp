#include <assert.h>
#include <limits.h>

#include "utils.h"
#include "Locker.h"

namespace ufix {
  
  Locker::Locker() {
    assert(pthread_condattr_init(&attr) != API_ERROR);
    assert(pthread_condattr_setclock(&attr, CLOCK_MONOTONIC) != API_ERROR);
    assert(pthread_cond_init(&event, &attr) != API_ERROR);

    pthread_mutex_init(&mutex, NULL);
    start_timestamp = (UINT) get_current_timestamp();
    elapse_time = 0;
  }
  
  int Locker::elapse(UINT timestamp) {
    int time = timestamp - start_timestamp;
    if (time < 0) time += UINT_MAX;
    return time;
  }

  void Locker::wait(int time_ms, int force_wait) {
    if (unlikely(force_wait)) {
      int status;
      pthread_mutex_lock(&mutex);
      pthread_wait(&event, &mutex, time_ms, &status);
      pthread_mutex_unlock(&mutex);
      return;
    }

    if (has_event()) return;
    start_timestamp = (UINT) get_current_timestamp();
    p_wait = 1;
    pthread_mutex_lock(&mutex);
    while (!has_event()) {
      UINT timestamp = (UINT) get_current_timestamp();
      elapse_time = elapse(timestamp);
      if (elapse_time > SPIN_TIME) break;
      pthread_yield();
    }
    
    if (!has_event()) {
      int status;
      pthread_wait(&event, &mutex, time_ms, &status);
    }
    pthread_mutex_unlock(&mutex);
    elapse_time = 0;
    p_wait = 0;
  }

  void Locker::signal() {
    if (p_wait == 1) {
      
      _MEMORY_BAR_;
      
      if (elapse_time < SPIN_TIME) {
	UINT timestamp = (UINT) get_current_timestamp();
	if (elapse(timestamp) < SPIN_TIME) return;
      }
      int code = API_ERROR;
      while (true) {
	if (p_wait == 0) break;
	code = pthread_mutex_trylock(&mutex);
	if (code != API_ERROR) {
	  pthread_cond_signal(&event);
	  pthread_mutex_unlock(&mutex);
	  break;
	}
	pthread_yield();
      }
    } 
  }
}
