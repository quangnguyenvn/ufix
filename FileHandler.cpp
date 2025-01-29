#include "utils.h"

#include "FileHandler.h"

namespace ufix {

  FileHandler::FileHandler(const char * name, const char * mode) {
    file = fopen(name, mode);
    abs_path = (char *) mem_alloc("FilePathAllocation", 512*sizeof(char));
    realpath(name, abs_path);
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
