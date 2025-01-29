#ifndef _UFIX_SIMPLE_LOGGER_
#define _UFIX_SIMPLE_LOGGER_

#include "Logger.h"

namespace ufix {

  class SimpleLogger : public Logger {
    
  public:
    SimpleLogger();
    
    void data_msg_sent(WMessage * msg);
    void data_msg_recv(RMessage * msg);
  };
}

#endif
