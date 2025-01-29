#ifndef _UFIX_R_GROUPS_DICTIONARY_
#define _UFIX_R_GROUPS_DICTIONARY_

#include "constants.h"

namespace ufix {
  class RGroupsDictionary {

  private:
    int * rg_def[MAX_FIX_TAG]; 
    
  public:
    
    RGroupsDictionary();
    
    void add_rg(int * fields, int len);
    int is_normal(int tag);
    int is_tag_in_rg(int rg_tag, int tag);
  };
}
#endif
