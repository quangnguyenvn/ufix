#include "RGroups.h"
#include "constants.h"

namespace ufix {
  Group::Group(int size) {
    fields_size = size;
    fields = (field_info *) mem_alloc("Init group", fields_size*sizeof(field_info));
    memset(fields, 0, fields_size*sizeof(field_info));
  }
  
  Group::~Group() {
    mem_free(fields);
  }
  
  void Group::set_parser(FieldsParser * f_parser) {
    parser = f_parser;
  }

  void Group::parse(const char * data, int data_len) {
    num_fields = parser->parse_group(data, data_len, fields);
  }
  
  int Group::get_num_fields() {
    return num_fields;
  }

  field_info * Group::get_field(int index) {
    if (index >= num_fields) return NULL;
    return fields + index;
  }

  void Group::get_groups(int index, void * groups_ptr) {
    RGroups * groups =  (RGroups *) groups_ptr;
    groups->set_parser(parser);
    field_info * info = fields + index;
    groups->parse(info->val, info->val_len);
  }
}
