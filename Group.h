#ifndef _UFIX_GROUP_
#define _UFIX_GROUP_

#include "FieldsParser.h"

namespace ufix {
  class Group {

  private:
    int fields_size;
    field_info * fields;
    int num_fields;
    FieldsParser * parser;

  public:
    Group(int size);
    ~Group();
    
    void set_parser(FieldsParser * f_parser);

    void parse(const char * data, int data_len);
    
    int get_num_fields();

    field_info * get_field(int index);

    void get_groups(int index, void * groups);
  };
}
#endif
