#include "constants.h" 
#include "utils.h"
#include "SocketConnector.h"

#define SOCKET_TYPE_ACCEPTOR 1
#define SOCKET_TYPE_INITIATOR 2

#define SOCKET_BLOCK_TIME 1000

#define RR_TIMEOUT 5

namespace ufix {
  
  int is_connected(int fd) {                                          
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    memset(&addr, 0, sizeof(addr));
    return (getpeername(fd, (struct sockaddr *)&addr, &len) != SOCKET_ERROR);
  }
  
  pthread_main socket_connector_recv(void * params) {
    SocketConnector * s_connector = (SocketConnector *) params;
    s_connector->recv_msgs();
    return 0;
  }

  pthread_main socket_connector_send(void * params) {    
    SocketConnector * s_connector = (SocketConnector *) params;
    s_connector->send_msgs();
    return 0;
  }

  pthread_main timer_update(void * params) {
    SocketConnector * s_connector = (SocketConnector *) params;
    s_connector->keep_update_utc_time();
    return 0;
  }


  SocketConnector::SocketConnector(const char * temp_buf, int temp_buf_end, socket_handle socket, Session * s) {
    session = s;
    recv_buf = session->get_recv_buf(recv_buf_head, recv_buf_tail, recv_buf_size);
    memcpy(recv_buf + recv_buf_tail, temp_buf, temp_buf_end); 
    recv_buf_tail += temp_buf_end;

    socket_desc = socket;
    socket_type = SOCKET_TYPE_ACCEPTOR;
    session->add_state(STATE_CONNECTED);
    socket_init();
  }
  
  SocketConnector::SocketConnector(Session * s) {
    session = s;
    recv_buf = session->get_recv_buf(recv_buf_head, recv_buf_tail, recv_buf_size);
    socket_desc = SOCKET_NULL;
    socket_type = SOCKET_TYPE_INITIATOR;
  }
  
  void SocketConnector::log_fatal(const char * msg) {
    Logger * logger = session->get_logger();
    if (logger != NULL) {
      logger->fatal(msg);
    } 
  }

  void SocketConnector::log_error(const char * msg) {
    Logger * logger = session->get_logger();
    if (logger != NULL) {
      logger->error(msg);
    } 
  }

  void SocketConnector::log_warn(const char * msg) {
    Logger * logger = session->get_logger();
    if (logger != NULL) {
      logger->warn(msg);
    } 
  }

  void SocketConnector::log_info(const char * msg) {
    Logger * logger = session->get_logger();
    if (logger != NULL) {
      logger->info(msg);
    }
  }
  
  void SocketConnector::log_data_msg_sent(WMessage * msg) {
    Logger * logger = session->get_logger();
    if (logger != NULL) {
      logger->data_msg_sent(msg);
    }           
  }    

  void SocketConnector::log_data_msg_recv(RMessage * msg) {
    Logger * logger = session->get_logger();
    if (logger != NULL) {
      logger->data_msg_recv(msg);
    }
  }

  void SocketConnector::log_session_msg_sent(WMessage * msg) {
    Logger * logger = session->get_logger();
    if (logger == NULL) return;
    logger->session_msg_sent(msg);
  }

  void SocketConnector::log_session_msg_recv(RMessage * msg) {
    Logger * logger = session->get_logger();
    if (logger != NULL) {
      logger->session_msg_recv(msg);
    } 
  }

  void SocketConnector::log_msg_error(const char * msg, int len, const char * error) {
    Logger * logger = session->get_logger();
    if (logger != NULL) {
      logger->msg_error(msg, len, error);
    } 
  }

  void SocketConnector::init_socket_addr() {
    struct addrinfo * result = (addrinfo *) malloc(sizeof(addrinfo));
    struct addrinfo * ptr = NULL;
    int get_addr_code = getaddrinfo(socket_addr, NULL, NULL, &result);
    
    if (get_addr_code != 0) {
      char buf[256];
      sprintf(buf, "Reading socket address %s error. Code %d\n", socket_addr, get_addr_code);
      log_fatal(buf);
    }
    sockaddr_in * sockaddr_ipv4;
    for (ptr = result; ptr != NULL; ptr=ptr->ai_next) {
      switch (ptr->ai_family) {
      case AF_INET:
	sockaddr_ipv4 = (struct sockaddr_in *) ptr->ai_addr;
	socket_addr_in = *sockaddr_ipv4;
	break;
      default:
	break;
      }

    }
    socket_addr_in.sin_port = htons(socket_port);
  }
  
  void SocketConnector::socket_reset() {
    if (socket_desc != SOCKET_NULL) {
      socket_close(socket_desc);
    }
    socket_create();
    socket_init();
  }
  
  void SocketConnector::socket_create() {
    socket_desc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_desc == SOCKET_INVALID) {
      log_fatal("Socket couldn't be initialized");
    }
  }

  void SocketConnector::socket_init() {
    int tcp_delay = 1;
    int code = setsockopt(socket_desc, IPPROTO_TCP, TCP_NODELAY, (const char *)& tcp_delay, sizeof(tcp_delay));
    if (code == SOCKET_ERROR) {
      log_error("Set TCP_DELAY on socket failed");
    }
    struct linger so_linger;
    so_linger.l_onoff = 1;
    so_linger.l_linger = 0;
    code = setsockopt(socket_desc, SOL_SOCKET, SO_LINGER, (const char *) &so_linger, sizeof(so_linger));
    if (code == SOCKET_ERROR) {
      log_error("Set SO_LINGER on socket failed");
    }

    int size = SOCKET_RECV_BUF_SIZE;
    code = setsockopt(socket_desc, SOL_SOCKET, SO_RCVBUF, (const char *) &size, sizeof(int));
    if (code == SOCKET_ERROR) {
      log_error("Set SO_RECVBUF on socket failed");
    }
    
    size = SOCKET_SEND_BUF_SIZE;
    code = setsockopt(socket_desc, SOL_SOCKET, SO_SNDBUF, (const char *) &size, sizeof(int));
    if (code == SOCKET_ERROR) {
      log_error("Set SO_SNDBUF on socket failed");
    }

    socket_set_nonblocking(socket_desc);
    
    socket_send_flag = 1;
  }
  
  void SocketConnector::socket_connect() {
    char addr[256];
    sprintf(addr, "%s:%d", socket_addr, socket_port);
    char buf[256];
    
    while (true) {
      socket_reset();
      sprintf(buf, "Connection to fix server %s initiating", addr);  
      log_warn(buf);
      int code = connect(socket_desc, (sockaddr *) &socket_addr_in, sizeof(sockaddr_in));
      if (code != SOCKET_ERROR) break;
      code = get_errno();
      if (code == EISCONN) {
        break;
      }
      if (code == EINVAL) {
        log_fatal("Connect return EINVAL");
      }
      
      if (code == EINPROGRESS || code == EWOULDBLOCK) {
	fd_set wfd, efd;
	FD_ZERO(&wfd);
	FD_SET(socket_desc, &wfd);
	FD_ZERO(&efd);
	FD_SET(socket_desc, &efd);
	
	timeval tv;
	tv.tv_sec = 10;
	tv.tv_usec = 0;
	
	int res = select(socket_desc + 1, NULL, &wfd, &efd, &tv);
	if (res == 1 && FD_ISSET(socket_desc, &wfd) && is_connected(socket_desc)) {
	  break;
	}
      }
      msleep(10000);
    }
    sprintf(buf, "Connection to fix server %s established", addr);
    log_info(buf);
  }
  
  void SocketConnector::set_socket_addr(const char * addr) {
    int addr_size = str_size(addr);
    socket_addr = (char *) mem_alloc("Init socket addr", addr_size + 1);
    memcpy(socket_addr, addr, addr_size);
    socket_addr[addr_size] = '\0';
  }

  void SocketConnector::set_socket_port(int port) {
    socket_port = port;
  }
  
  void SocketConnector::join() { 
    pthread_join(thread_send, NULL);
    pthread_join(thread_recv, NULL);
  }

  void SocketConnector::start() {
    
    if (session->get_status_flag() == STATUS_START) return;

    session->set_status_flag(STATUS_START);

    update_utc_time();
    
    pthread_t addr;    
    int thread_creation_code;

    char name[128];
    if (socket_type == SOCKET_TYPE_INITIATOR) {
      init_socket_addr();
      thread_creation_code = thread_create(&addr, &timer_update, this);
      thread_timer = get_thread_handle(addr, thread_creation_code);
      sprintf(name, "TM-%s", session->get_session_id());
      thread_name(thread_timer, name); 
    }
    
    thread_creation_code = thread_create(&addr, &socket_connector_recv, this);
    thread_recv = get_thread_handle(addr, thread_creation_code);
    sprintf(name, "RC-%s", session->get_session_id());
    thread_name(thread_recv, name);

    thread_creation_code = thread_create(&addr, &socket_connector_send, this);
    thread_send = get_thread_handle(addr, thread_creation_code);
    sprintf(name, "SD-%s", session->get_session_id());
    thread_name(thread_send, name);
  }
  
  void SocketConnector::update_utc_time() {
    session->update_utc_time();
  }

  void SocketConnector::keep_update_utc_time() {
    log_warn("Timer thread is running");
    while (session->get_status_flag() == STATUS_START) {
      session->update_utc_time();
      msleep(1000);
    }
    log_warn("Timer thread exit");
  }

  void SocketConnector::recv_msgs() {
    log_warn("Receiver thread is running");
    if (recv_buf_tail > recv_buf_head) {
      process_new_data();
    }

    while (session->get_status_flag() == STATUS_START) {
      if (!session->is_state(STATE_CONNECTED)) {
	msleep(1000);
	continue;
      }
      
      int len;
      
      poll_wait(socket_desc, POLLERR | POLLIN, SOCKET_BLOCK_TIME);
      len = recv(socket_desc, recv_buf + recv_buf_tail, SOCKET_RECV_WINDOWS, 0);
      if (len > 0) {
	recv_buf_tail += len;
	if (process_new_data()) {
	  if (recv_buf_tail > recv_buf_size) {
	    memcpy(recv_buf, recv_buf + recv_buf_head, recv_buf_tail - recv_buf_head);
	    recv_buf_tail -= recv_buf_head;
	    recv_buf_head = 0;
	  }
	  continue;
	} 
      } 
      
      int err_code = get_errno();
      if (err_code == EAGAIN || err_code == EWOULDBLOCK || err_code == EINTR) {
	if (len == SOCKET_ERROR && !session->recv_timeout()) {
	  continue;
	}
      } 
          
      log_warn("Session disconnected");
      session->reset_state();
      recv_buf_tail = recv_buf_head;
            
      if (socket_type == SOCKET_TYPE_ACCEPTOR) {
	session->set_status_flag(STATUS_READY);
	session->set_recv_buf(recv_buf_head, recv_buf_tail);
      } 
    }
    log_warn("Receiver thread exit");
  }
  
  int SocketConnector::process_new_data() {
    while (true) {
      if (recv_buf_tail - recv_buf_head < 12) return 1;
      int pos = recv_buf_head + session->get_fix_version_len() + 2;
      while (true) {
	if (pos > recv_buf_tail) return 1;
	if (recv_buf[pos] == SOH) break;
	pos++;
      }
      pos += 3;
      int msg_len_pos = pos;
      
      while (true) {
	if (pos > recv_buf_tail) return 1;
	if (recv_buf[pos] == SOH) break;
	pos++;
      }
      
      int msg_type_pos = pos + 4;

      int msg_len = pos + 1 - recv_buf_head + convert_atoi(recv_buf + msg_len_pos) + CHECKSUM_SIZE;
      if (recv_buf_tail - recv_buf_head < msg_len) return 1;
      
      int seq_pos = seek(recv_buf + msg_type_pos, msg_len - (msg_type_pos - recv_buf_head), SEQ_PATTERN, SEQ_PATTERN_LENGTH);
      if (seq_pos < 0) {
	log_msg_error(recv_buf + recv_buf_head, msg_len, "Msg recv without tag seq");
	return 0;
      } else {
	ULONG64 seq = convert_atoll(recv_buf + msg_type_pos + seq_pos + SEQ_PATTERN_LENGTH);
	RMessage msg(recv_buf + recv_buf_head, msg_len, msg_type_pos - recv_buf_head, seq); 
	process_msg(&msg);
      }
      recv_buf_head += msg_len;
    }
  }
  
  void SocketConnector::process_msg(RMessage * msg) {
    session->update_recv_event_ts();
        
    int is_session_msg = msg->is_session();
    
    if (unlikely(is_session_msg)) {
      log_session_msg_recv(msg);
      msg->set_parser(session->get_session_msgs_parser());
      process_session_msg(msg);
    } else {
      log_data_msg_recv(msg);
    }
    session->put_msg_to_app_queue(msg, is_session_msg);
  }

  void SocketConnector::process_session_msg(RMessage * msg) {
    msg->parse();
    const char * msg_type = msg->get_msg_type();
    
    if (msg_type[0] == MSG_TYPE_HEARTBEAT[0]) 
      process_msg_heartbeat(msg);
    else if (msg_type[0] == MSG_TYPE_TEST_REQUEST[0]) {
      process_msg_test_request(msg);
    } else if (msg_type[0] == MSG_TYPE_RESEND_REQUEST[0]) {
      process_msg_resend_request(msg);
    } else if (msg_type[0] == MSG_TYPE_REJECT[0]) {
      process_msg_reject(msg);
    } else if (msg_type[0] == MSG_TYPE_SEQUENCE_RESET[0]) {
      process_msg_sequence_reset(msg);
    } else if (msg_type[0] == MSG_TYPE_LOGOUT[0]) {
      process_msg_logout(msg);
    } else if (msg_type[0] == MSG_TYPE_LOGON[0]) {
      process_msg_logon(msg);
    }
  }

  void SocketConnector::process_msg_heartbeat(RMessage * msg) {
  }

  void SocketConnector::process_msg_test_request(RMessage * msg) {
    const char * req_id = msg->get_field(TEST_REQUEST_ID);
    session->add_hb_cmd(req_id);
  }

  void SocketConnector::process_msg_resend_request(RMessage * msg) {
    ULONG64 begin_seq_no = convert_atoll(msg->get_field(BEGIN_SEQ_NO));
    ULONG64 end_seq_no = convert_atoll(msg->get_field(END_SEQ_NO));
    session->add_sequence_reset_cmd(begin_seq_no, end_seq_no);
    session->add_state(STATE_RR_RECV);
  }

  void SocketConnector::process_msg_reject(RMessage * msg) {
  }

  void SocketConnector::process_msg_sequence_reset(RMessage * msg) {
    ULONG64 current_seq = msg->get_seq();
    ULONG64 new_seq_no = convert_atoll(msg->get_field(NEW_SEQ_NO));
    session->update_seq(current_seq, new_seq_no);
  }

  void SocketConnector::process_msg_logout(RMessage * msg) {
  }

  void SocketConnector::process_msg_logon(RMessage * msg) {
    session->add_state(STATE_LOGON_RECV);
    session->signal();
  }

  void SocketConnector::send_msgs() {
    
    log_warn("Sender thread is running");

    int send_status = 1;
    ULONG64 rr_recv_ts = 0;

    while (session->get_status_flag() == STATUS_START) {
      if (send_status == 0) {
	session->wait(SOCKET_BLOCK_TIME, !session->is_logon());
      }
      
      if (socket_type == SOCKET_TYPE_INITIATOR && unlikely(!session->is_state(STATE_CONNECTED))) {
	log_warn("State not connected");
	send_status = 0;
	socket_connect();
	session->add_state(STATE_CONNECTED);
	rr_recv_ts = 0;
	continue;
      }
      
      if (socket_send_flag == 0) {
	send_status = 0;
	continue;
      }
      
      if (unlikely(!session->is_state(STATE_LOGON_SENT))) {
	if (socket_type == SOCKET_TYPE_INITIATOR || session->is_state(STATE_LOGON_RECV)) {
	  if (send_logon()) {
	    session->add_state(STATE_LOGON_SENT);
	  } 
	}
	send_status = 0;
	continue;
      }
      
      if (unlikely(!session->is_state(STATE_LOGON_RECV))) {
	send_status = 0;
	continue;
      }
      
      send_cmds();
      
      if (unlikely(!session->is_logon())) {
	if (session->is_state(STATE_RR_RECV) && !session->is_state(STATE_RR_TIMEOUT)) {
	  if (rr_recv_ts == 0) {
	    rr_recv_ts = get_current_timestamp();
	  } else {
	    if ((int) (get_current_timestamp() - rr_recv_ts) > RR_TIMEOUT*MILLION) {
	      session->add_state(STATE_RR_TIMEOUT);
	    } 
	  }
	}
	send_status = 0;
	continue;
      }
      
      send_status = send_data();
      
      if (send_status == 0) {
	if (session->send_timeout()) {
	  char cmd[8];
	  cmd[1] = 0;
	  send_status = send_heartbeat(cmd);
	}
      } 
      
      if (send_status != 0) {
	session->update_send_event_ts();
      }
    }
    log_warn("Sender thread exit");
    session->set_status_flag(STATUS_READY);
  }
  
  void SocketConnector::send_cmds() {
    while (true) {
      const char * cmd = session->get_next_cmd();
      if (cmd == NULL) break;
      send_cmd(cmd);
    }
  }

  int SocketConnector::send_msg(WMessage * msg, int new_msg_flag, int data_flag) {
    
    int len;
    const char * msg_data = msg->get_data(len);
    int offset = 0;
        
    session->write_msg_sent(msg, new_msg_flag);
    
    int num_retries = 0;
    
    while (true) {
      int nbytes = socket_send(socket_desc, msg_data + offset, len - offset);
      if (nbytes > 0) {
	offset += nbytes;
	if (offset == len) {
	  break;
	}
      } else {
	int err_code = get_errno();
	if (err_code != EAGAIN && err_code != EWOULDBLOCK && err_code != EINTR) {
	  char log_buf[512];
	  sprintf(log_buf, "Send msg seq %lld error. Error code %d", msg->get_seq(), get_errno());
	  log_warn(log_buf);
	  socket_send_flag = 0;
	  if (socket_type == SOCKET_TYPE_ACCEPTOR) {
            session->set_status_flag(STATUS_READY);
	  }
	  return 0;
	} 
      }
      if (num_retries%5000 == 0) { 
	log_warn("Socket send buffer full");
      }
      num_retries++;
      msleep(1);
    }
    
    if (data_flag) {
      log_data_msg_sent(msg);
    } else {
      log_session_msg_sent(msg);
    }
    if (new_msg_flag) {
      session->increment_msg_seq_num();
    }
    return 1;
  }
  
  int SocketConnector::send_data() {
    int num_queues = session->get_num_queues();
    int num_msgs_sent = 0;

    for (int i = 1; i <= num_queues; i++) {
     
      if (!session->is_queue_available(i)) {
	continue;
      }
      
      WMessage msg(i);
      int q_msgs_sent = 0;
      
      while (q_msgs_sent < WINDOW_QUEUE_RR_SIZE) {
	int msg_code = session->get_queued_msg(&msg);
	if (msg_code == 0) break;
	session->finalize_msg(&msg);
	send_msg(&msg, 1, 1);
	session->mark_msg_sent(&msg);
	if (socket_send_flag == 0) break;
	q_msgs_sent++;
      }
      num_msgs_sent += q_msgs_sent;
    }
    return num_msgs_sent;
  }

  int SocketConnector::send_cmd(const char * cmd) {
    char buf[512];
    sprintf(buf, "Sending cmd to other side %c", cmd[0]);
    log_warn(buf);
    char cmd_type = *cmd; 
    if (cmd_type == MSG_TYPE_HEARTBEAT[0]) {
      return send_heartbeat(cmd);
    }
    if (cmd_type == MSG_TYPE_TEST_REQUEST[0]) {
      return send_test_request();
    }
    if (cmd_type == MSG_TYPE_RESEND_REQUEST[0]) {
      return send_resend_request(cmd);
    }
    if (cmd_type == MSG_TYPE_SEQUENCE_RESET[0]) {
      return send_sequence_reset(cmd);
    }
    log_warn("Cmd type not supported");
    return 0;
  }

  int SocketConnector::send_logon() {
    char buf[1024];
    WMessage msg(QUEUE_SESSION, buf);
    session->get_msg(&msg, MSG_TYPE_LOGON);
    session->finalize_msg(&msg);
    return send_msg(&msg, 1, 0);
  }

  int SocketConnector::send_heartbeat(const char * cmd) {
    char buf[1024];
    WMessage msg(QUEUE_SESSION, buf);
    session->get_msg(&msg, MSG_TYPE_HEARTBEAT);
    if (cmd[1] != 0) {
      msg.set_field(TEST_REQUEST_ID, cmd + 1, str_size(cmd + 1));
    }
    session->finalize_msg(&msg);
    return send_msg(&msg, 1, 0);
  }

  int SocketConnector::send_test_request() {
    char buf[1024];
    WMessage msg(QUEUE_SESSION, buf);
    session->get_msg(&msg, MSG_TYPE_TEST_REQUEST);
    session->finalize_msg(&msg);
    return send_msg(&msg, 1, 0);
  }

  int SocketConnector::send_resend_request(const char * cmd) {
    char buf[1024];
    WMessage msg(QUEUE_SESSION, buf);
    session->get_msg(&msg, MSG_TYPE_RESEND_REQUEST);
    msg.set_field(BEGIN_SEQ_NO, *((ULONG64 *) (cmd + 1)));
    msg.set_field(END_SEQ_NO, *((ULONG64 *) (cmd + 1 + sizeof(ULONG64))));
    session->finalize_msg(&msg);
    return send_msg(&msg, 1, 0);
  }

  int SocketConnector::send_sequence_reset(ULONG64 start, ULONG64 end) {
    char buf[1024];
    WMessage msg(QUEUE_SESSION, buf);
    session->get_msg(&msg, MSG_TYPE_SEQUENCE_RESET);
    msg.set_field(GAP_FILL_FLAG, 'Y');
    msg.set_field(NEW_SEQ_NO, end);
    session->finalize_msg(&msg, start, 0);
    return send_msg(&msg, 0, 0);
  }

  int SocketConnector::send_sequence_reset(const char * cmd) {
    ULONG64 * seqs = (ULONG64 *)(cmd + 1);
    ULONG64 begin_seq_no = seqs[0];
    ULONG64 end_seq_no = seqs[1];
    if (end_seq_no == 0) {
      end_seq_no = session->get_msg_seq();
    } else {
      end_seq_no++;
    }
    ULONG64 seq = begin_seq_no;
    int num_msgs_sent = 0;
    
    ULONG64 start_reset_seq = begin_seq_no;
    
    char log_buf[512];
    
    sprintf(log_buf, "Trying to resend msgs seq from %lld to %lld ", begin_seq_no, end_seq_no);
    log_warn(log_buf);
    
    session->lock_all_send_queues();
    log_warn("All send queues locked");
    while (true) {
      WMessage sent_msg(QUEUE_SESSION); 
      
      int code = session->get_sent_msg(&sent_msg, seq);
      int send_code;

      if (code) {
	if (start_reset_seq < seq) {
	  send_code = send_sequence_reset(start_reset_seq, seq);
	  if (send_code == 0) {
	    sprintf(log_buf, "Send msg sequence reset seq %lld error. Error code %d", seq, get_errno());
	    log_warn(log_buf);
	    break;
	  }
	  num_msgs_sent += send_code;
	  start_reset_seq = seq;
	}
	session->finalize_msg(&sent_msg, seq, 1);
	send_code = send_msg(&sent_msg, 0, 0);
	if (send_code == 0) {
	  sprintf(log_buf, "Send data msg seq %lld error. Error code %d", seq, get_errno());
	  log_warn(log_buf);
	  break;
	}
	num_msgs_sent += send_code;
	start_reset_seq++;
      }
      seq++;
      if (seq == end_seq_no) {
	if (start_reset_seq < seq) {
	  num_msgs_sent += send_sequence_reset(start_reset_seq, seq);
	}
	sprintf(log_buf, "%s", "Resend msgs completed sucessfully");
	log_warn(log_buf);
	break;
      }
    }
    session->unlock_all_send_queues();
    log_warn("All send queues unlocked");
    return num_msgs_sent;
  }
}

