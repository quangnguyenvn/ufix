#ifndef _UFIX_W_MESSAGE_
#define _UFIX_W_MESSAGE_

#include "RMessage.h"

namespace ufix {
  class WMessage {
  private:
    
    static const int NEW = 0;
    static const int PRE_FIN = 1;
    
    int q_index;
    int max_body_length_size;
    
    int status;
    int direct_buf_flag;
    
    char * data;
    int data_len;
    
    ULONG64 seq;
    ULONG64 data_seq;
    
    int body_length_pos;
    int body_end_pos;
    
    inline void set_tag(int tag);
    void set_checksum(char * msg, unsigned int cks);

    void finalize_new(int msg_seq_pos, const char * msg_seq, int msg_seq_len,
		      int sending_time_pos, const char * utc_time);
    void finalize_resend(int msg_seq_pos, const char * msg_seq, int msg_seq_len,
                         int sending_time_pos, const char * utc_time);

  public:
    WMessage(int index);
    WMessage(int index, char * message);
    
    int get_q_index();
    void set_q_index(int index);

    void set_max_body_length_size(int size);
    
    const char * get_data();
    const char * get_data(int & len);
    
    void switch_data(char * msg, int len);
    
    void reserve_field(int tag, int len);
    
    void reserve_seq_num(int len);
    void reserve_sending_time();
    
    void set_field(int tag, const char * value, int len);
    
    void set_field(int tag, const char * value);

    void set_field(int tag, int value);

    void set_field(int tag, double value);

    void set_field(int tag, long long value);
    
    void set_field(int tag, ULONG64 value);

    void set_field(int tag, char value);
    
    void set_field(int tag, tm * time);
    
    void copy(int tag, RMessage * rmsg);

    int is_direct_buf();

    ULONG64 get_seq();
    ULONG64 get_data_seq();
    void set_data_seq(ULONG64 seq_num);
    
    int get_msg_seq_pos();
    int get_sending_time_pos();

    const char * get_field(int tag);

    void mark_body_length_pos(int pos);
    void mark_body_end_pos(int pos);
        
    void pre_finalize();
    void finalize(int msg_seq_pos, const char * msg_seq, int msg_seq_len, 
		  int sending_time_pos, const char * utc_time, int resend_flag);
    
    ULONG64 index_seq;
  };
}
#endif
