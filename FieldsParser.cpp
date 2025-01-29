#include "FieldsParser.h"
#include "constants.h"

namespace ufix {
  FieldsParser::FieldsParser(int size) {
    fields_size = size;
    counter = 0;
    fields = (field_info *) mem_alloc("Init fields parser", fields_size*sizeof(field_info));
    memset(fields, 0, fields_size*sizeof(field_info));
    rg_dictionary = NULL;
  }

  void FieldsParser::set_rg_dictionary(RGroupsDictionary * dictionary) {
    rg_dictionary = dictionary;
  }
  
  int FieldsParser::read_tag(const char * data, int & pos, int data_len) {
    int tag = 0;
    while (data[pos] != EQUAL) {
      int digit = convert_ctoi(data[pos]);
      if (digit > 9 || digit < 0) {
	return 0;
      }
      tag = tag*10 + digit;

      pos++;
      if (pos >= data_len) return 0;
    }
    return tag;
  }

  int FieldsParser::read_value(const char * data, int & pos, int data_len) {
    while (pos < data_len && data[pos] != SOH) {
      pos++;
    }
    return 1;
  }

  int FieldsParser::read_rg_value(int rg_tag, const char * data, int & pos, int data_len) {
    if (read_value(data, pos, data_len) == 0) return 0;
    
    int last_rg_pos;

    while (true) {
      
      last_rg_pos = pos;
      pos++;

      if (pos >= data_len) return 0;
      int tag = read_tag(data, pos, data_len);
      if (!rg_dictionary->is_tag_in_rg(rg_tag, tag)) {
	break;
      }
      if (read_value(data, pos, data_len) == 0) return 0;
    }

    pos = last_rg_pos;
    return 1;
  }

  int FieldsParser::parse(const char * data, int data_len) {
    counter++;
    
    int pos = 0; 
    int last_index = 0;

    while (true) {                                                                                                                 
      if (pos >= data_len) return 1;                                                                                               
      int tag = read_tag(data, pos, data_len);
      if (tag == 0) return 0;
      pos++;

      int index = tag%fields_size;
      fields[index].counter = counter;
      fields[index].tag = tag;
      fields[index].val = data + pos;                                                                                   
      int start_val_pos = pos;
      
      int value_code;
      if (likely(rg_dictionary == NULL || rg_dictionary->is_normal(tag))) {
	value_code = read_value(data, pos, data_len);
      } else {
	value_code = read_rg_value(tag, data, pos, data_len);
      }
      if (value_code == 0) return 0;
      fields[index].val_len = pos - start_val_pos;
      fields[index].next = NULL;
      fields[last_index].next = fields + index;
      last_index = index;
      pos++;                                                                                                                       
    }
  }
  
  int FieldsParser::parse_groups(const char * data, int data_len, field_info * infos) {
    int pos = 0;
    int num_groups = convert_atoi(data + pos);
    
    if (read_value(data, pos, data_len) == 0) {
      return 0;
    }

    pos++;
    int tag_start = 0;
    int num_groups_read = 0;
        
    int end_val_pos;

    while (true) {
      if (pos >= data_len) {
	if (likely(num_groups_read == num_groups)) {
	  infos[num_groups_read - 1].val_len = end_val_pos - infos[num_groups_read - 1].val_len;
	  return num_groups;
	} else {
	  return 0;
	}
      }
      int tag_pos = pos;
      int tag = read_tag(data, pos, data_len);
            
      if (tag == 0) return 0;
      if (tag_start == 0) tag_start = tag;
      pos++;
      if (tag == tag_start) {
	infos[num_groups_read].val_len = tag_pos;
	infos[num_groups_read].val = data + tag_pos;
	if (num_groups_read > 0) {
	  infos[num_groups_read - 1].val_len = end_val_pos - infos[num_groups_read - 1].val_len;
	}
	num_groups_read++;
      }
      int value_code;

      if (likely(rg_dictionary == NULL || rg_dictionary->is_normal(tag))) {
	value_code = read_value(data, pos, data_len);
      } else {
	value_code = read_rg_value(tag, data, pos, data_len);
      }
      if (value_code == 0) {
	return 0;
      }
      end_val_pos = pos++;
    }
  }

  int FieldsParser::parse_group(const char * data, int data_len, field_info * infos) {
    int index = 0;
    int pos = 0;
        
    while (true) {
      if (pos >= data_len) break;
      
      int tag = read_tag(data, pos, data_len);
      if (tag == 0) return 0;

      infos[index].tag = tag;
      int start_val_pos = ++pos;
      infos[index].val = data + start_val_pos;
      
      int value_code;

      if (likely(rg_dictionary == NULL || rg_dictionary->is_normal(tag))) {
        value_code = read_value(data, pos, data_len);
      } else {
	value_code = read_rg_value(tag, data, pos, data_len);
      }
      if (value_code == 0) return 0;

      infos[index].val_len = pos - start_val_pos;  
      pos++;
      index++;
    }
    return index;
  }


  field_info * FieldsParser::get_field(int tag) {
    int index = tag%fields_size;
    if (fields[index].counter != counter) {
      return NULL;
    }
    if (fields[index].tag != tag) {
      return NULL;
    }
    return fields + index;
  }
}
