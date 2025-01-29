#include "SimpleLogger.h"

namespace ufix {

  SimpleLogger::SimpleLogger() {
  }
  
  void SimpleLogger::data_msg_sent(WMessage * msg) {
    int len;
    const char * data = msg->get_data(len);
    char time[32];
    format_time(time);
    fprintf(log_file, "%s  DT ->   %.*s\n", time, len, data);
    fflush(log_file);
  }

  void SimpleLogger::data_msg_recv(RMessage * msg) {
    int len;
    const char * data = msg->get_data(len);
    char time[32];
    format_time(time);
    fprintf(log_file, "%s  DT <-   %.*s\n", time, len, data);     
    fflush(log_file);
  }
}

