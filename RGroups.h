#ifndef _UFIX_R_GROUPS_
#define _UFIX_R_GROUPS_

#include "Group.h"
#include "FieldsParser.h"

namespace ufix {
  class RGroups {

  private:
    int max_num_groups;
    field_info * fields;
    int num_groups;
    FieldsParser * parser;
    
  public:
    
    RGroups(int size);
    
    void set_parser(FieldsParser * f_parser);

    void parse(const char * data, int data_len);
    
    int get_num_groups();

    void get_group(int index, Group * group);
  };
}
#endif
