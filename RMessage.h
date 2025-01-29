#ifndef _UFIX_R_MESSAGE_
#define _UFIX_R_MESSAGE_

#include "FieldsParser.h"
#include "RGroups.h"

namespace ufix {
  class RMessage {
  
  private:
    long long timestamp;
    
    const char * data;
    int data_len;
    
    int msg_type_pos;
    ULONG64 seq;
    
    FieldsParser * parser;
  
  public:
    
    RMessage(const char * msg, int msg_len, int msg_type, ULONG64 msg_seq);
    
    void copy(RMessage * msg);
    
    void set_parser(FieldsParser * ps);
    
    void set_gf(ULONG64 seq_no);
    int is_gf();
    int is_session();

    void set_timestamp(ULONG64 ts);
    ULONG64 get_timestamp();

    void reset();

    const char * get_data(int & len);
    
    ULONG64 get_seq();
    
    int is_empty();
    
    const char * get_msg_type();
    
    int get_msg_type_pos();
    
    int parse();
    
    field_info * get_field_ptr(int tag);
    
    const char * get_field(int tag);

    const char * get_field(int tag, int & field_len);
    
    int copy_field(int tag, char * value);  

    void get_groups(int tag, RGroups * group);
  };
}
#endif
