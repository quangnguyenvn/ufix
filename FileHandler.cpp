#include "utils.h"

#include "FileHandler.h"

namespace ufix {

  FileHandler::FileHandler(const char * name, const char * mode) {
    file = fopen(name, mode);
    abs_path = (char *) mem_alloc("FilePathAllocation", 512*sizeof(char));
#ifdef _WINDOWS_OS_
    if (_fullpath(abs_path, name, 512) == NULL) {
      memcpy(abs_path, name, str_size(name));
      abs_path[str_size(name)] = '\0';
    }
#else
    realpath(name, abs_path);
#endif
  }

  void FileHandler::close() {
    fclose(file);
  }
  
  const char * FileHandler::get_abs_path() {
    return abs_path;
  }

  FILE * FileHandler::get_file() {
    return file;
  }
 
}
