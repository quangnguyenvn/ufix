#ifndef _UFIX_FILE_HANDLER_
#define _UFIX_FILE_HANDLER_

#include <stdio.h>

namespace ufix {
  
  class FileHandler {
  
  private:
    FILE * file;
    char * abs_path;

  public:
    FileHandler(const char * name, const char * mode);
    
    void close();
    const char * get_abs_path();

    FILE * get_file();
  };
  
}

#endif
