#ifndef _UFIX_FIELDS_PARSER_
#define _UFIX_FIELDS_PARSER_

#include "utils.h"
#include "RGroupsDictionary.h"

namespace ufix {
  class FieldsParser {

  private:
    RGroupsDictionary * rg_dictionary;

    int fields_size;
    field_info * fields;

    ULONG64 counter;
    
    int read_tag(const char * data, int & pos, int data_len);
    int read_value(const char * data, int & pos, int data_len);
    int read_rg_value(int rg_tag, const char * data, int & pos, int data_len);

  public:
    FieldsParser(int size);
    
    void set_rg_dictionary(RGroupsDictionary * dictionary);
    int parse(const char * data, int data_len);
    int parse_groups(const char * data, int data_len, field_info * infos);
    int parse_group(const char * data, int data_len, field_info * infos);
    
    field_info * get_field(int tag);
  };
}
#endif
