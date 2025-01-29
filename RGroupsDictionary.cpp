#include "RGroupsDictionary.h"

#include "utils.h"

namespace ufix {
  
  RGroupsDictionary::RGroupsDictionary() {
    memset(rg_def, 0, MAX_FIX_TAG*sizeof(int *));
  }
  
  void RGroupsDictionary::add_rg(int * fields, int len) {
    int tag = fields[0];
    rg_def[tag] = (int *) mem_alloc("Init rg def", MAX_FIX_TAG*sizeof(int));
    memset(rg_def[tag], 0, MAX_FIX_TAG*sizeof(int));
    for (int i = 1; i < len; i++) {
      rg_def[tag][fields[i]] = 1;
    }
  }

  int RGroupsDictionary::is_normal(int tag) {
    return (rg_def[tag] == NULL);
  }

  int RGroupsDictionary::is_tag_in_rg(int rg_tag, int tag) {
    return (rg_def[rg_tag][tag] == 1);
  }
}
