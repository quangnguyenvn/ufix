#include "portable.h"
#include "utils.h"
#include "SessionOption.h" 

namespace ufix {
  
  SessionOption::SessionOption() {
    session_id = NULL;
    rg_dictionary = NULL;
    max_body_length_size = 4;
    max_msg_seq_size = 10;
    heart_bt_in = 30;
    default_appl_ver_id = -1;

    recv_queue_size = 1 << 20; 
    
    num_send_queues = 1;
    send_queues_size = NULL;
    send_queues_reserve = NULL;
    send_queues_flag = NULL;
    default_appl_ver_id = -1;
    p_dir = NULL;
    
    window_mark_size = 32; 
    fix_msg_avg_length = 256;
    fix_msg_max_length = 1 << 20;

    num_header_options = 0;
    num_logon_options = 0;
  }
  
  void SessionOption::add_repeating_group(int * tags, int num_tags) {
    if (rg_dictionary == NULL) rg_dictionary = new RGroupsDictionary();
    rg_dictionary->add_rg(tags, num_tags);
  }

  void SessionOption::add_header_option(int tag, const char * val) {
    header_options_tag[num_header_options] = tag;
    int val_size = str_size(val);
    header_options_val_size[num_header_options] = val_size;
    header_options_val[num_header_options] = (char *) mem_alloc("Init header option", val_size*sizeof(char));
    memcpy(header_options_val[num_header_options], val, val_size);
    num_header_options++;
  }

  void SessionOption::add_logon_option(int tag, const char * val) {
    logon_options_tag[num_logon_options] = tag;
    int val_size = str_size(val);
    logon_options_val_size[num_logon_options] = val_size;
    logon_options_val[num_logon_options] = (char *) mem_alloc("Init logon option", val_size*sizeof(char));
    memcpy(logon_options_val[num_logon_options], val, val_size);
    num_logon_options++;
  }
}
