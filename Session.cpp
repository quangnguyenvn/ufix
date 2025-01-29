#include "constants.h"
#include "utils.h"
#include "SimpleLogger.h"
#include "Session.h"

namespace ufix {
  
  inline void format_date_time(char * buf) {
    time_t ts;
    time(&ts);
    
    tm * ptm = gmtime(&ts);
    int sec = (ptm->tm_sec < 60)?ptm->tm_sec:59;

    sprintf(buf, "%04d%02d%02d%02d:%02d:%02d",
            ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
            ptm->tm_hour, ptm->tm_min, sec);
  } 

  pthread_main process_recv_msgs(void * params) {
    Session * session = (Session *) params;
    session->process_recv_queue_msgs();
    return 0;
  }
  
  Session::Session(SessionOption * opt) {
    s_logger = new SimpleLogger();
    init_session(opt);
  }

  Session::Session(SessionOption * opt, Logger * logger) {
    s_logger = logger;
    init_session(opt);
  }

  void Session::init_session(SessionOption * opt) {
    
    s_opt = (SessionOption *) mem_alloc("Init session option", sizeof(SessionOption));
    memcpy(s_opt, opt, sizeof(SessionOption));

    pthread_mutex_init(&state_mutex, NULL);
    
    cmds = new Cmds();
    state = STATE_START;
    
    update_utc_time();
    sender_comp_id_size = str_size(s_opt->sender_comp_id);
    target_comp_id_size = str_size(s_opt->target_comp_id);
    
    calculate_msgs_positions();

    recv_queue = new RecvQueue(s_opt->recv_queue_size, cmds, s_logger);
    
    sent_marks = new SentMark(SENT_MARK_SIZE);
    set_max_msg_seq_size(s_opt->max_msg_seq_size);
    
    ULONG64 last_seqs[512];
    init_p_manager(last_seqs);
        
    set_heart_bt_in(s_opt->heart_bt_in);
    
    session_msgs_parser = new FieldsParser(MAX_FIX_TAG);
    
    init_recv_buf();

    s_logger->set_log_file(p_manager->get_system_log());
    
    init_send_queues();
    char buf[512];
    for (int i = 1; i <= s_opt->num_send_queues; i++) {
      sprintf(buf, "Queue #%d set initially last_seq_sent %lld", i, last_seqs[i]);
      s_logger->warn(buf);
      send_queues[i]->mark_seq_sent(last_seqs[i]);
    }
    num_data_msgs_sent = 0;
    num_data_msgs_recv = 0;

    update_send_event_ts();
    update_recv_event_ts();
  }
  
  Logger * Session::get_logger() {
    return s_logger;
  }
  
  void Session::calculate_msgs_positions() {
    fix_version_len = str_size(s_opt->fix_version);
    body_length_pos = 5 + fix_version_len;
    msg_type_pos =  body_length_pos + s_opt->max_body_length_size + 4;
    msg_seq_pos = msg_type_pos  + 5;
    sending_time_pos = msg_seq_pos + s_opt->max_msg_seq_size + 4 + sender_comp_id_size + 4 + target_comp_id_size + 5 + 4; 
  }

  char * Session::get_recv_buf(int & head, int & tail, int & size) {
    head = recv_buf_head;
    tail = recv_buf_tail;
    size = recv_buf_size;
    return recv_buf;
  }

  void Session::set_recv_buf(int head, int tail) {
    recv_buf_head = head;
    recv_buf_tail = tail;
  }
  
  int Session::match(const char * sender_comp_id, const char * target_comp_id) {
    int sender_size = str_size(sender_comp_id);
    int target_size = str_size(target_comp_id);
    
    int code = str_match(sender_comp_id, s_opt->sender_comp_id) && str_match(target_comp_id, s_opt->target_comp_id);
    return code;
  }
  
  void Session::init_recv_buf() {
    recv_buf_size = s_opt->recv_queue_size*s_opt->fix_msg_avg_length;
    recv_buf = (char *) malloc((recv_buf_size + SOCKET_RECV_WINDOWS)*sizeof(char));
    recv_buf_head = 0;
    recv_buf_tail = 0;
  }

  void Session::process_recv_queue_msgs() {
    ready_for_data();
    FieldsParser * parser = new FieldsParser(MAX_FIX_TAG);
    parser->set_rg_dictionary(s_opt->rg_dictionary);
    while (true) {
      RMessage * msg = recv_queue->get();
      msg->set_parser(parser);
      if (likely(!msg->is_gf())) {
        on_msg(msg);
        p_manager->write_seq_processed(msg->get_seq());
      }
      msg->reset();
    }
  }
  
  void Session::start() {
    thread_create(&thread_addr_recv, &process_recv_msgs, this);
  }
  
  void Session::stop() {
    if (status_flag == STATUS_STOP) return;

    status_flag = STATUS_STOP;
    while (status_flag == STATUS_STOP) {
      msleep(1);
    }
    status_flag = STATUS_STOP;
  }

  int Session::reset() {
    s_logger->warn("Reset operation start");
    s_logger->warn("Close open files");
    p_manager->close();
    s_logger->reset();

    char backup_dir[128];
    format_date_time(backup_dir);
    int code = p_manager->backup(backup_dir);

    if (!code) {
      s_logger->warn("Reset operation failed. Couldn't backup files to history");
      return false;
    }

    init_session(s_opt);
    s_logger->warn("Reset operation completed");
    return true;
  }

  void Session::on_msg(RMessage * msg) {
    if (unlikely(msg->is_session())) {
      on_session_msg(msg);
    } else {
      on_data_msg(msg);
    }
  }

  void Session::set_max_msg_seq_size(int size) {
    msg_seq = (char *) mem_alloc("Init max msg seq size", size*sizeof(char));
    memset(msg_seq, '0', size - 1);
    msg_seq[size - 1] = '1'; 
  }

  void Session::set_heart_bt_in(int hbt) {
    heart_bt_threshold = hbt;
    test_request_threshold = (int) (TR_THRESHOLD*heart_bt_threshold);
    disconnect_threshold = (int) (DISCONNECT_THRESHOLD*heart_bt_threshold);
  }
  
  void Session::init_send_queues() {
    send_queues = (SendQueuePtr *) mem_alloc("Init send queues", (s_opt->num_send_queues + 1)*sizeof(SendQueuePtr));
    for (int i = 1; i <= s_opt->num_send_queues; i++) {
      int q_size, q_reserve;
      if (s_opt->send_queues_size == NULL) {
        q_size = s_opt->recv_queue_size;
      } else {
        q_size = s_opt->send_queues_size[i];
      }
      if (s_opt->send_queues_reserve == NULL) {
	q_reserve = q_size/8;
      } else {
	q_reserve = s_opt->send_queues_reserve[i];
      }
      set_queue(i, q_size, q_reserve);
      int q_flag = ((s_opt->send_queues_flag == NULL) || (s_opt->send_queues_flag[i] == QUEUE_OPEN));
      if (q_flag) {
	set_queue_open(i);
      } else {
        set_queue_close(i);
      }
    }
  }

  void Session::init_p_manager(ULONG64 * last_seqs) {
    char buf[1024];
    sprintf(buf, "%.*s-%.*s", target_comp_id_size, s_opt->target_comp_id, sender_comp_id_size, s_opt->sender_comp_id);
    const char * p_dir = s_opt->p_dir;
    if (p_dir == NULL) p_dir = DEFAULT_P_DIR;
    p_manager = new PManager(p_dir, buf, s_opt->num_send_queues + 1);
    recv_queue->set_head_seq(p_manager->read_last_seq_processed());
    memset(last_seqs, 0, 512*sizeof(ULONG64));
    ULONG64 last_seq = p_manager->read_sent_marks(sent_marks, last_seqs);
    
    seq_sent_info * info = NULL;
    if (last_seq > 0) info = sent_marks->get_mark(last_seq);
    ULONG64 seq;
    if (info == NULL) {
      seq = 1;
      state = STATE_RR_RECV;
    }  else {
      seq = last_seq + s_opt->window_mark_size + 1;
    }
    convert_ltoa(seq, msg_seq, s_opt->max_msg_seq_size);
  }
  
  void Session::update_utc_time() {
    clock.update_utc_time();
  }

  void Session::set_queue(int index, int size, int reserve) {
    char log[512];
    sprintf(log, "Init queue index #%d size %d, reserve %d", index, size, reserve);
    s_logger->warn(log);
    FILE * file = p_manager->init_data_queue(index);
    FILE * recover_file = p_manager->get_data_recover_file(index);
    send_queues[index] = new SendQueue(index, size, reserve, 
				       s_opt->fix_msg_avg_length, s_opt->fix_msg_max_length, file, recover_file);
    p_manager->close(recover_file);
  }
  
  int Session::get_num_queues() {
    return s_opt->num_send_queues;
  }
  
  void Session::mark_msg_sent(WMessage * msg) {
    send_queues[msg->get_q_index()]->mark_msg_sent(msg);
    num_data_msgs_sent++;
  }

  int Session::is_queue_available(int index) {
    if (index > s_opt->num_send_queues || index <= 0) return 0;
    if (send_queues[index]->is_close()) return 0;
    return 1;
  }

  void Session::set_queue_open(int index) {
    if (index > s_opt->num_send_queues || index <= 0) return;
    send_queues[index]->set_open();
    signal();
    char buf[256];
    sprintf(buf, "Queue #%d opened", index);
    s_logger->warn(buf);
  }

  void Session::set_queue_close(int index) {
    if (index > s_opt->num_send_queues || index <= 0) return;
    send_queues[index]->set_close();
    char buf[256];    
    sprintf(buf, "Queue #%d closed", index);
    s_logger->warn(buf);
  }
  
  void Session::set_queue_flag_ref(int index, int * flag) {
    send_queues[index]->set_q_flag_ref(flag);
  }

  int * Session::get_queue_flag_ref(int index) {
    return send_queues[index]->get_q_flag_ref();
  }

  void Session::set_queue_flag_threshold(int index, int threshold) {
    send_queues[index]->set_q_flag_threshold(threshold);
  }

  void Session::update_seq(ULONG64 current_seq_no, ULONG64 new_seq_no) {
    recv_queue->update_seq(current_seq_no, new_seq_no);
  }

  const char * Session::get_next_cmd() {
    return cmds->get_next_cmd();
  }
  
  void Session::update_recv_event_ts() {
    last_recv_utc_ts = clock.get_timestamp();
    tr_sent_flag = 0;
  }

  void Session::update_send_event_ts() {
    last_sent_utc_ts = clock.get_timestamp();
  }

  int Session::recv_timeout() {
    int elapse_time = (int) (clock.get_timestamp() - last_recv_utc_ts);

    if (likely(elapse_time < test_request_threshold)) {
      return 0;
    }
    
    if (elapse_time >= disconnect_threshold) {
      last_recv_utc_ts = clock.get_timestamp();
      return 1;
    } else {
      if (tr_sent_flag == 0) {
	add_tr_cmd();
	tr_sent_flag = 1;
      }
      return 0;
    }
  }

  int Session::send_timeout() {
    int elapse_time = (int) (clock.get_timestamp() - last_sent_utc_ts);
    
    if (elapse_time >= heart_bt_threshold) {
      last_sent_utc_ts = clock.get_timestamp();
      return 1;
    } 
    return 0;
  }

  void Session::add_hb_cmd(const char * id) {
    char cmd[CMD_BLOCK_SIZE];
    cmd[0] = MSG_TYPE_HEARTBEAT[0];
    if (id != NULL) {
      for (int i = 1; i < CMD_BLOCK_SIZE; i++) {
        if (id[i - 1] != SOH) {
          cmd[i] = id[i - 1];
        } else {
          cmd[i] = 0;
          break;
        }
      }
    } else {
      cmd[1] = 0;    
    }
    cmds->add_cmd(cmd);
  }

  void Session::add_tr_cmd() {
    char cmd[CMD_BLOCK_SIZE];
    cmd[0] = MSG_TYPE_TEST_REQUEST[0];
    cmds->add_cmd(cmd);
  }

  void Session::add_sequence_reset_cmd(ULONG64 begin_seq_no, ULONG64 end_seq_no) {
    char cmd[CMD_BLOCK_SIZE];
    cmd[0] = MSG_TYPE_SEQUENCE_RESET[0];
    memcpy(cmd + 1, (char *) &begin_seq_no, sizeof(ULONG64));
    memcpy(cmd + 1 + sizeof(ULONG64), (char *) &end_seq_no, sizeof(ULONG64));
    cmds->add_cmd(cmd);
  }
  
  void Session::lock_all_send_queues() {
    for (int i = 1; i <= get_num_queues(); i++) {
      send_queues[i]->lock();
    }
  }

  void Session::unlock_all_send_queues() {
    for (int i = 1; i <= get_num_queues(); i++) {
      send_queues[i]->unlock();
    }
  }


  int Session::get_sent_msg(WMessage * msg, ULONG64 seq) {
    
    ULONG64 data_seq;
    
    seq_sent_info * info = sent_marks->get_mark(seq);
    if (info == NULL) {
      info = sent_marks->get_last_mark(seq);
      data_seq = info->data_seq + seq - info->msg_seq;
    } else {
      if (info->msg_seq != seq) return 0;
      data_seq = info->data_seq;
    }
    if (info->q_index == QUEUE_SESSION) return 0;
    int code = send_queues[info->q_index]->get_sent_msg(msg, data_seq);
    return code;
  }

  void Session::get_msg(WMessage * msg, const char * msg_type) {
    
    if (msg->get_data() == NULL) {
      msg->switch_data(send_queues[msg->get_q_index()]->get_new_msg_buf(), 0);
    }
    msg->set_max_body_length_size(s_opt->max_body_length_size);
    int msg_type_len = str_size(msg_type);
        
    msg->mark_body_length_pos(body_length_pos);
    msg->mark_body_end_pos(body_length_pos + s_opt->max_body_length_size);
    msg->set_field(BEGIN_STRING, s_opt->fix_version, fix_version_len);
    msg->reserve_field(BODY_LENGTH, s_opt->max_body_length_size);
    msg->set_field(MSG_TYPE, msg_type, msg_type_len);
    msg->reserve_field(MSG_SEQ_NUM, s_opt->max_msg_seq_size);
    msg->set_field(SENDER_COMP_ID, s_opt->sender_comp_id, sender_comp_id_size);
    msg->set_field(TARGET_COMP_ID, s_opt->target_comp_id, target_comp_id_size);
    msg->set_field(POS_DUP_FLAG, 'N');
    msg->reserve_field(SENDING_TIME, SENDING_TIME_SIZE);  
    msg->reserve_field(ORIG_SENDING_TIME, SENDING_TIME_SIZE);
        
    for (int i = 0; i < s_opt->num_header_options; i++) {
      msg->set_field(s_opt->header_options_tag[i], s_opt->header_options_val[i], s_opt->header_options_val_size[i]);
    }
    
    if (msg_type_len == 1) {
      if (msg_type[0] == MSG_TYPE_LOGON[0]) {
	msg->set_field(ENCRYPT_METHOD, ENCRYPT_METHOD_NONE);
	msg->set_field(HEART_BT_IN, s_opt->heart_bt_in);
	if (s_opt->default_appl_ver_id >= 0) {
          msg->set_field(DEFAULT_APPL_VER_ID, s_opt->default_appl_ver_id);
        }

	for (int i = 0; i < s_opt->num_logon_options; i++) {
	  msg->set_field(s_opt->logon_options_tag[i], s_opt->logon_options_val[i], s_opt->logon_options_val_size[i]);
	}

      } else if (msg_type[0] == MSG_TYPE_TEST_REQUEST[0]) {
	msg->set_field(TEST_REQUEST_ID, msg_seq, s_opt->max_msg_seq_size);
      }
    }
  }
  
  int Session::add_msg(WMessage * msg, int num_retries) {
    int code = send_queues[msg->get_q_index()]->add_msg(msg, num_retries);
    if (code) signal();
    return code;
  }

  int Session::add_msg(WMessage * msg) {
    return add_msg(msg, 0);
  }
  
  int Session::get_queued_msg(WMessage * msg) {
    return send_queues[msg->get_q_index()]->get_msg(msg);
  }
  
  int Session::has_event() {
    for (int i = 1; i <= s_opt->num_send_queues; i++) {
      if (send_queues[i]->is_close()) continue;
      if (send_queues[i]->has_msg()) return 1;
    }
    return 0;
  }

  int Session::get_state() {
    return state;
  }

  int Session::get_status_flag() {
    return status_flag;
  }
  
  void Session::set_status_flag(int flag) {
    status_flag = flag;
  }

  void Session::set_status_ready() {
    set_status_flag(STATUS_READY);
  }

  int Session::is_logon() {
    return state == STATE_LOGON;
  }

  int Session::get_state_description(char * buf) {
    if (state == STATE_LOGON) {
      return sprintf(buf, "%s", "LOGON");
    }
    if (!is_state(STATE_CONNECTED)) {
      return sprintf(buf, "%s", "DISCONNECTED");
    }

    if (!is_state(STATE_LOGON_SENT) && !is_state(STATE_LOGON_RECV)) {
      return sprintf(buf, "%s", "CONNECTED");
    }
    
    if (!is_state(STATE_RR_RECV) || !is_state(STATE_RR_TIMEOUT)) {
      return sprintf(buf, "%s", "SESSION INIT-TRY LOGON");
    }
    
    return sprintf(buf, "%s", "SESSION INIT-RECOVERING");
  }

  void Session::add_state(int add_state) {
    
    pthread_mutex_lock(&state_mutex);
    if (is_state(add_state)) {
      pthread_mutex_unlock(&state_mutex); 
      return;
    }
    
    if (add_state == STATE_CONNECTED || add_state == STATE_RR_RECV || is_state(STATE_CONNECTED)) {
      state += add_state;
    } 
    char buf[512];
    sprintf(buf, "Current session state is %d", state);  
    s_logger->warn(buf);
    pthread_mutex_unlock(&state_mutex);
  }
  
  int Session::is_state(int state_to_check) {
    return state%(2*state_to_check) >= state_to_check;  
  }

  void Session::reset_state() {
    pthread_mutex_lock(&state_mutex);
    state = STATE_RR_RECV;
    pthread_mutex_unlock(&state_mutex);
  }
  
  ULONG64 Session::get_q_num_pending_msgs(int q_index) {
    return send_queues[q_index]->get_num_pending_msgs();
  }
  
  int Session::get_num_send_queues() {
    return s_opt->num_send_queues;
  }
  
  ULONG64 Session::get_num_data_msgs_sent() {
    return num_data_msgs_sent;
  }

  ULONG64 Session::get_num_data_msgs_recv() {
    return num_data_msgs_recv;
  }

  const char * Session::get_session_id() {
    return s_opt->session_id;
  }

  int Session::get_fix_version_len() {
    return fix_version_len;
  }
  
  ULONG64 Session::get_msg_seq() {
    return convert_atoll(msg_seq, s_opt->max_msg_seq_size);
  }

  void Session::put_msg_to_app_queue(RMessage * msg, int is_session_msg) {
    pre_process_msg(msg);
    recv_queue->put(msg);
    if (!is_session_msg) {
      num_data_msgs_recv++;
    }
  }
  
  FieldsParser * Session::get_session_msgs_parser() {
    return session_msgs_parser;
  }

  void Session::increment_msg_seq_num() {
    int index = s_opt->max_msg_seq_size - 1;
    while (true) {
      if (msg_seq[index] == '9') {
	msg_seq[index] = '0';
	index--;
      } else {
	msg_seq[index]++;
	break;
      }
    }
  }
  
  int Session::get_msg_type_size(WMessage * msg) {
    int len;
    const char * data = msg->get_data(len);
    if (len > 0) {
      return str_size(data + msg_type_pos);
    } else {
      return -1;
    }
  }

  void Session::finalize_msg(WMessage * msg) {
    int add = get_msg_type_size(msg) - 1;
    if (add >= 0) {
      msg->finalize(msg_seq_pos + add, msg_seq, s_opt->max_msg_seq_size, 
		    sending_time_pos + add, clock.get_utc_time(), 0);
    } else {
      char buf[512];
      sprintf(buf, "Get msg size 0 at seq %lld, data_seq %lld, head_data_seq %lld", 
	      convert_atoll(msg_seq, s_opt->max_msg_seq_size), 
	      msg->get_data_seq(), msg->index_seq);
      s_logger->fatal(buf);
    }
  }

  void Session::finalize_msg(WMessage * msg, ULONG64 seq, int resend_flag) {
    char buf[32];
    int num_digits = convert_ltoa(seq, buf);
    char seq_buf[32];
    memset(seq_buf, '0', s_opt->max_msg_seq_size - num_digits);
    memcpy(seq_buf + s_opt->max_msg_seq_size - num_digits, buf, num_digits); 
    int add = get_msg_type_size(msg) - 1;
    msg->finalize(msg_seq_pos + add, seq_buf, s_opt->max_msg_seq_size, 
		  sending_time_pos + add, clock.get_utc_time(), resend_flag);
  }

  void Session::write_msg_sent(WMessage * msg, int flag) {
    if (flag == 0) return;
    
    int w_flag = 0;
    
    int q_index = msg->get_q_index();
        
    ULONG64 msg_seq = msg->get_seq();
    
    ULONG64 data_seq = msg->get_data_seq();
    
    if (unlikely(q_index != last_q_index_sent)) {
      w_flag = 1;
      last_q_index_sent = q_index;
    } else if (unlikely(q_index == 0 || msg_seq%s_opt->window_mark_size == 0)) {
      w_flag = 1;
    }
    
    if (unlikely(w_flag)) {
      p_manager->write_seq_sent(q_index, msg_seq, data_seq);
      sent_marks->set_mark(q_index, msg_seq, data_seq);
    }
  }
  
}
