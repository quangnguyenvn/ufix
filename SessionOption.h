#ifndef _UFIX_SESSION_OPTION_
#define _UFIX_SESSION_OPTION_

#include "RGroupsDictionary.h"

namespace ufix {

#define MAX_HEADER_OPTIONS 64
  
  class SessionOption {
    
  public:
    
    const char * session_id;

    const char * fix_version;
    
    RGroupsDictionary * rg_dictionary;

    int max_body_length_size;
    int max_msg_seq_size;
    
    const char * sender_comp_id;
    
    const char * target_comp_id;
    
    int heart_bt_in;
    
    int default_appl_ver_id;

    int recv_queue_size;
    
    int num_send_queues;
    int * send_queues_size;
    int * send_queues_reserve;
    char * send_queues_flag; 
    
    const char * p_dir;
    
    int window_mark_size;
    
    int fix_msg_avg_length;
    int fix_msg_max_length;
    
    char * header_options_val[MAX_HEADER_OPTIONS];
    int header_options_val_size[MAX_HEADER_OPTIONS];
    int header_options_tag[MAX_HEADER_OPTIONS];
    int num_header_options;
    
    char * logon_options_val[MAX_HEADER_OPTIONS];
    int logon_options_val_size[MAX_HEADER_OPTIONS];
    int logon_options_tag[MAX_HEADER_OPTIONS];
    int num_logon_options;

    SessionOption();
    
    void add_repeating_group(int * tags, int num_tags);

    void add_header_option(int tag, const char * val);
    void add_logon_option(int tag, const char * val);
  };
}
#endif
