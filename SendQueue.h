#ifndef _UFIX_SEND_QUEUE_
#define _UFIX_SEND_QUEUE_

#include "portable.h"

#include "WMessage.h"

namespace ufix {

#define QUEUE_CLOSE 0
#define QUEUE_OPEN 1

  class SendQueue {
  
  private:
    int q_index;
    
    pthread_mutex_t copy_lock;
    
    int * q_flag;
    int q_flag_threshold;

    FILE * data_file;

    char * q_buf;
    int q_buf_pos;
    int q_buf_size;
    
    send_queue_info * msgs_queue;
    int q_size;
    int q_max_msgs;

    ULONG64 data_seq;
    ULONG64 recover_data_seq;

    ULONG64 head_data_seq;

    void write_data(const char * data, int len); 
    void recover_data(FILE * file);
    int recover_msgs(char * buf, int buf_len);
    void recover_msg(const char * msg, int msg_len);
    int is_queue_full();
    int is_queue_empty();
    
  public:
    SendQueue(int index, int size, int reserve, int avg_msg_size, int max_msg_size, FILE * file, FILE * recover_file);
    
    void set_q_flag_ref(int * flag);
    int * get_q_flag_ref();
    void set_q_flag_threshold(int threshold); 

    int is_close();
    void set_open();
    void set_close();

    int get_num_pending_msgs();
    int add_msg(WMessage * msg, int num_retries);
    int get_msg(WMessage * msg);
    int has_msg();
    void mark_msg_sent(WMessage * msg);
    void mark_seq_sent(ULONG64 seq);
    
    char * get_new_msg_buf();
    
    void lock();
    void unlock();

    int get_sent_msg(WMessage * msg, ULONG64 seq);
    ULONG64 get_head_data_seq();
  };
  typedef SendQueue * SendQueuePtr;
}
#endif
