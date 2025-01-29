#include "RGroups.h"

namespace ufix {

  RGroups::RGroups(int size) {
    max_num_groups = size;
    fields = (field_info *) mem_alloc("Init routing groups", max_num_groups*sizeof(field_info));
    num_groups = 0;
  }
  
  void RGroups::set_parser(FieldsParser * f_parser) {
    parser = f_parser;
  }

  void RGroups::parse(const char * data, int data_len) {
    num_groups = parser->parse_groups(data, data_len, fields); 
  }
    
  int RGroups::get_num_groups() {
    return num_groups;
  }

  void RGroups::get_group(int index, Group * group) {
    group->set_parser(parser);
    group->parse(fields[index].val, fields[index].val_len);
  }
}
