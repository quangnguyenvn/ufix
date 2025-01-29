#include "portable.h"
#include "constants.h"

#include "RMessage.h"

namespace ufix {
  RMessage::RMessage(const char * msg, int msg_len, int msg_type, ULONG64 msg_seq) {    
    data = msg;
    data_len = msg_len;
    msg_type_pos = msg_type;
    timestamp = 0;
    seq = msg_seq;
  }
  
  void RMessage::copy(RMessage * msg) {
    data = msg->get_data(data_len);
    msg_type_pos = msg->get_msg_type_pos();
    _STORE_MEMORY_BAR_;
    seq = msg->get_seq();
  }
  
  void RMessage::set_gf(ULONG64 seq_no) {
    timestamp = -1;
    _STORE_MEMORY_BAR_;
    seq = seq_no;
  }

  int RMessage::is_gf() {
    return (timestamp == -1);
  }
  
  int RMessage::is_session() {
    return is_session_msg(data + msg_type_pos);
  }

  void RMessage::set_parser(FieldsParser * ps) {
    parser = ps;
  }
  
  void RMessage::set_timestamp(ULONG64 ts) {
    timestamp = ts;
  }

  ULONG64 RMessage::get_timestamp() {
    return timestamp;
  }

  void RMessage::reset() {
    timestamp = 0;
    seq = 0;
  }

  const char * RMessage::get_data(int & len) {
    len = data_len;
    return data;
  }
  
  ULONG64 RMessage::get_seq() {
    return seq;
  }
  
  int RMessage::is_empty() {
    return (seq == 0);
  }
  
  const char * RMessage::get_msg_type() {
    return data + msg_type_pos;
  }
  
  int RMessage::get_msg_type_pos() {
    return msg_type_pos;
  }

  int RMessage::parse() {
    return parser->parse(data, data_len);
  }

  field_info * RMessage::get_field_ptr(int tag) {
    return parser->get_field(tag);
  }
  
  const char * RMessage::get_field(int tag) {
    field_info * info = parser->get_field(tag);
    if (info == NULL) return NULL;
    return info->val;
  }

  const char * RMessage::get_field(int tag, int & field_len) {
    field_info * info = parser->get_field(tag);
    if (info == NULL) return NULL;
    field_len = info->val_len;
    return info->val;
  }

  int RMessage::copy_field(int tag, char * value) {
    field_info * info = parser->get_field(tag);
    if (info == NULL) return 0;
    memcpy(value, info->val, info->val_len);
    return info->val_len;
  }

  void RMessage::get_groups(int tag, RGroups * group) {
    group->set_parser(parser);
    field_info * info = parser->get_field(tag);
    group->parse(info->val, info->val_len);
  }
}
