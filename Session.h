#ifndef _UFIX_SESSION_
#define _UFIX_SESSION_

#include "portable.h"
#include "WMessage.h"
#include "Cmds.h"
#include "FieldsParser.h"
#include "RecvQueue.h"
#include "PManager.h"
#include "SendQueue.h"
#include "SessionOption.h"
#include "Clock.h"
#include "Logger.h"

namespace ufix {

#define STATUS_READY 0
#define STATUS_START 1
#define STATUS_STOP 2

#define STATE_START 0
#define STATE_CONNECTED 1
#define STATE_LOGON_SENT 2
#define STATE_LOGON_RECV 4
#define STATE_RR_RECV 8
#define STATE_RR_TIMEOUT 16
#define STATE_LOGON 31

#define DEFAULT_P_DIR "data";

 class Session : public Locker {

 private:
   
   int status_flag;
   
   SessionOption * s_opt;
   
   pthread_mutex_t state_mutex;
   
   pthread_t thread_addr_recv;
   
   int state;
   
   Logger * s_logger;
   
   int fix_version_len;
   int body_length_pos;
   char * msg_seq;
   
   int msg_type_pos;
   int msg_seq_pos;
   int sending_time_pos; 
   
   int sender_comp_id_size;
   int target_comp_id_size;
   
   int heart_bt_threshold;
   int test_request_threshold;
   int disconnect_threshold;
    
   ULONG64 last_sent_utc_ts;
   ULONG64 last_recv_utc_ts;
   int tr_sent_flag;
   
   Clock clock;
   
   Cmds * cmds;
   
   char * recv_buf;
   int recv_buf_size, recv_buf_head, recv_buf_tail;
   
   RecvQueue * recv_queue;
   
   SendQueuePtr * send_queues;
   int last_q_index_sent;

   PManager * p_manager;
   SentMark * sent_marks;
   
   FieldsParser * session_msgs_parser;
   
   void init_session(SessionOption * opt);
   void calculate_msgs_positions();
   int get_msg_type_size(WMessage * msg);
   void init_p_manager(ULONG64 * last_seqs);
   void set_queue(int index, int size, int reserve);  
   void set_max_msg_seq_size(int size);
   void set_heart_bt_in(int hbt_time);
   void init_send_queues();
   void init_recv_buf();
   void on_msg(RMessage * msg);
   int has_event();
   
   ULONG64 num_data_msgs_sent;
   ULONG64 num_data_msgs_recv;
   
 protected:
   virtual void ready_for_data() = 0;
   virtual void pre_process_msg(RMessage * msg) = 0;
   virtual void on_session_msg(RMessage * msg) = 0;
   virtual void on_data_msg(RMessage * msg) = 0;
   
 public:
   Session(SessionOption * opt);
   Session(SessionOption * opt, Logger * logger);
   
   void start();
   void stop();
   int reset();

   Logger * get_logger();
   
   int match(const char * sender_comp_id, const char * target_comp_id);
   
   char * get_recv_buf(int & head, int & tail, int & size);
   void set_recv_buf(int head, int tail);
   
   void get_msg(WMessage * msg, const char * msg_type);
   int add_msg(WMessage * msg, int num_retries);
   int add_msg(WMessage * msg);
   
   int get_queued_msg(WMessage * msg);
   
   int get_num_queues(); 
   int is_queue_available(int index); 
   void set_queue_open(int index); 
   void set_queue_close(int index); 
   void set_queue_flag_ref(int index, int * flag);
   int * get_queue_flag_ref(int index);
   void set_queue_flag_threshold(int index, int threshold);

   void update_seq(ULONG64 current_seq_no, ULONG64 new_seq_no);
   
   void mark_msg_sent(WMessage * msg);
   void lock_all_send_queues();
   void unlock_all_send_queues();
   
   int get_state();
   int get_status_flag();
   void set_status_flag(int flag);
   void set_status_ready();
   int is_logon();
   int get_state_description(char * buf);
   void add_state(int);
   int is_state(int);
   void reset_state();
   
   int get_num_send_queues();
   ULONG64 get_q_num_pending_msgs(int q_index);
   ULONG64 get_num_data_msgs_sent();
   ULONG64 get_num_data_msgs_recv();
   const char * get_session_id();
   
   void update_utc_time();
   
   const char * get_next_cmd();
   
   int recv_timeout();
   int send_timeout();
   void update_recv_event_ts();
   void update_send_event_ts();
   void add_hb_cmd(const char * id);
   void add_tr_cmd();
   void add_sequence_reset_cmd(ULONG64 begin_seq_no, ULONG64 end_seq_no);
   
   void put_msg_to_app_queue(RMessage * msg, int is_session_msg);
    
   FieldsParser * get_session_msgs_parser();
   
   ULONG64 get_msg_seq();
   
   int get_sent_msg(WMessage * msg, ULONG64 seq);
   
   int get_fix_version_len();
   
   void increment_msg_seq_num();
   
   void finalize_msg(WMessage * msg);
   void finalize_msg(WMessage * msg, ULONG64 seq, int resend_flag);
   
   void write_msg_sent(WMessage * msg, int flag);
   int get_last_q_index_sent();
   
   void process_recv_queue_msgs();
 };
 typedef Session * SessionPtr;
}

#endif
