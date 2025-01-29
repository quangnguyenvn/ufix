#ifndef _UFIX_LOCKER_
#define _UFIX_LOCKER_

#include "portable.h" 

#define SPIN_TIME 20

namespace ufix {
  class Locker {
  
  private:
    
    pthread_cond_t event;
    pthread_condattr_t attr;
    pthread_mutex_t mutex;
    
    UINT start_timestamp;
    int elapse_time;
    
    int p_wait;
    
    int elapse(UINT timestamp);

  protected:
    virtual int has_event() = 0;

  public:
    Locker();
    
    void signal();
    void wait(int time_ms, int force_wait);
  };
}
#endif
