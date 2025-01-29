#ifndef _UFIX_LOGGER_
#define _UFIX_LOGGER_

#include "RMessage.h"
#include "WMessage.h"

namespace ufix {

  class Logger {
    
  protected:
    FILE * log_file;
    
  public:
    Logger();
    void reset(); 
    void set_log_file(FILE * file);
    void fatal(const char * msg);
    
    void error(const char * msg);
    void warn(const char * msg);
    void info(const char * msg);
    void session_msg_sent(WMessage * msg);
    void session_msg_recv(RMessage * msg);
    void msg_error(const char * msg, int len, const char * error);
    
    virtual void data_msg_sent(WMessage * msg) = 0;
    virtual void data_msg_recv(RMessage * msg) = 0;

  };
}

#endif
