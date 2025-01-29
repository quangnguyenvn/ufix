#include "Logger.h"
#include "utils.h"
#include "constants.h"

namespace ufix {
  Logger::Logger() {    
    reset();
  }
  
  void Logger::reset() {
    log_file = NULL;
  }

  void Logger::set_log_file(FILE * file) {
    if (log_file == NULL) {
      log_file = file;
    }
  }
  
  void Logger::fatal(const char * msg) {
    char time[32];
    format_time(time);
    fprintf(log_file, "%s   FATAL:  %s\n", time, msg);
    
    void* callstack[128];
    int i, frames = backtrace(callstack, 128);
    char** strs = backtrace_symbols(callstack, frames);
    for (i = 0; i < frames; ++i) {
      fprintf(log_file, "%s\n", strs[i]);
    }
    mem_free(strs);
    fflush(log_file);
    exit(-1);
  }

  void Logger::error(const char * msg) {
    char time[32];
    format_time(time);
    fprintf(log_file, "%s   ERROR:  %s\n", time, msg);
    fflush(log_file);
  }

  void Logger::warn(const char * msg) {
    char time[32];
    format_time(time);
    fprintf(log_file, "%s   WARN:  %s\n", time, msg);
    fflush(log_file);
  }

  void Logger::info(const char * msg) {
    char time[32];
    format_time(time);
    fprintf(log_file, "%s   INFO:  %s\n", time, msg);
    fflush(log_file);
  }

  void Logger::session_msg_sent(WMessage * msg) {
    int len;
    const char * data = msg->get_data(len);
    char time[32];
    format_time(time);
    fprintf(log_file, "%s  SS ->   %.*s\n", time, len, data);
    fflush(log_file);
  }

  void Logger::session_msg_recv(RMessage * msg) {
    int len;
    const char * data = msg->get_data(len);
    char time[32];
    format_time(time);
    fprintf(log_file, "%s  SS <-   %.*s\n", time, len, data);
    fflush(log_file);
  }

  void Logger::msg_error(const char * msg, int len, const char * error) {
    char time[32];
    format_time(time);
    fprintf(log_file, "%s   ERR   %.*s. ERROR msg: %s\n", time, len, msg, error);
    fflush(log_file);
  }
}
