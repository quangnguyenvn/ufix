#include "portable.h"
#include "utils.h"
#include "SimpleLogger.h"
#include "SocketServer.h"
#include "SocketConnector.h" 

namespace ufix {
  
  int is_full_msg(const char * buf, int len) {
    int msg_body_length_pos = seek(buf, len, "9=", 2);
    if (msg_body_length_pos < 0) return 0;
    msg_body_length_pos += 2;

    int pos = msg_body_length_pos;
    while (pos < len) {
      if (buf[++pos] == SOH) break;
    }
    if (pos >= len) return 0;

    int body_length = convert_atoi(buf + msg_body_length_pos);

    int msg_len = pos + 1 + body_length + CHECKSUM_SIZE;
    if (len < msg_len) {
      return 0;
    }
    return msg_len;
  }
  
  int validate_comp_id(const char * msg, int len, int & s_pos, int & t_pos) {
    int sender_comp_id_pos = seek(msg, len, "49=", 3);
    if (sender_comp_id_pos < 0) {
      s_pos = sender_comp_id_pos;
      return 0;
    }
    s_pos = sender_comp_id_pos + 3;
    
    int target_comp_id_pos = seek(msg, len, "56=", 3);

    if (target_comp_id_pos < 0) {
      t_pos = target_comp_id_pos;
      return 0;
    }

    t_pos = target_comp_id_pos + 3;

    return 1;
  }
  
  Session * find_session(const char * sender_comp_id, const char * target_comp_id, SessionPtr * sessions, int num_sessions) {
    for (int i = 0; i < num_sessions; i++) {      
      if (sessions[i]->match(sender_comp_id, target_comp_id)) {
	return sessions[i];
      }
    }
    return NULL;
  }
  
  pthread_main connection_acceptor(void * params) {
    SocketServer * server = (SocketServer *) params;
    server->run();
    return 0;
  }

  pthread_main connection_handler(void * params) {
    char * params_buf = (char *) params;
    socket_handle sock = *((int *) params_buf);
    params_buf +=  sizeof(socket_handle);
    Logger * logger = (Logger *) params_buf;
    params_buf += sizeof(Logger);
    int num_sessions = *((int *) params_buf);
    params_buf += sizeof(int);
    SessionPtr * sessions = (SessionPtr *) params_buf; 
    
    char buf[8192];
    int end = 0;
    int msg_len;

    while(true) {
      int read_size = recv(sock, buf + end, 8192 - end , 0);
      if (read_size <= 0) {
	int err_code = get_errno();
	if (err_code != EAGAIN && err_code != EWOULDBLOCK && err_code != EINTR) {
	  socket_close(sock);
	  mem_free(params);
	  return 0;
	}
	msleep(1);
	continue;
      }
      end += read_size;
      
      msg_len = is_full_msg(buf, end);
      if (msg_len > 0) {
	char log_buf[512];
	sprintf(log_buf, "Recv msg: %.*s", msg_len, buf);
	logger->info(log_buf);
	break;
      }
    }
    int sender_comp_id_pos, target_comp_id_pos;
    int code = validate_comp_id(buf, msg_len, sender_comp_id_pos, target_comp_id_pos);
    if (code == 0) {
      char log_buf[512];
      if (sender_comp_id_pos < 0) {
	sprintf(log_buf, "%s", "Incorrect format msg: Missing SenderCompID");
	logger->error(log_buf);
      } else if (target_comp_id_pos < 0) {
	sprintf(log_buf, "%s", "Incorrect format msg: Missing TargetCompID"); 
	logger->error(log_buf);
      }
      socket_close(sock);
      mem_free(params);
      return 0;
    }
      
    Session * session = find_session(buf + target_comp_id_pos, buf + sender_comp_id_pos, sessions, num_sessions);
    if (session == NULL) {
      logger->warn("Session not found");
      socket_close(sock);
      mem_free(params);
      return 0;
    }
    
    int flag = session->get_status_flag();
    if (flag != STATUS_READY) {
      char log_buf[512];
      sprintf(log_buf, "Unable to start session with status flag: %d", flag);
      logger->warn(log_buf);
      socket_close(sock);
      mem_free(params);
      return 0;
    }

    SocketConnector s_connector(buf, end, sock, session);
    s_connector.start();
    s_connector.keep_update_utc_time();
    s_connector.join();
    
    socket_close(sock);
    mem_free(params);
    return 0;
  } 
  
  SocketServer::SocketServer(int max_num_sessions) {
    sessions =(SessionPtr *) mem_alloc("Init sessions ptr", max_num_sessions*sizeof(SessionPtr));
    num_sessions = 0;
    s_logger = new SimpleLogger();
    socket_addr = NULL;
  }

  void SocketServer::set_socket_addr(const char * addr) {
    if (addr != NULL) {
      int addr_size = str_size(addr);
      socket_addr = (char *) mem_alloc("Init addr size", addr_size + 1);
      memcpy(socket_addr, addr, addr_size);
      socket_addr[addr_size] = '\0';
    }
  }

  void SocketServer::set_socket_port(int port) {
    socket_port = port;
  }
  
  void SocketServer::add_session(Session * session) {
    sessions[num_sessions++] = session;
  }

  void SocketServer::start() {
    if(thread_create(&thread_server_addr , connection_acceptor, this) < 0) {
      char buf[512];
      sprintf(buf, "Start thread to handle connection failed. Error code: %d", get_errno());
      s_logger->error(buf);
    }
  }

  void SocketServer::run() {
    char url[256];		
    if (socket_addr != NULL) {
      sprintf(url, "server_log_%s:%d", socket_addr, socket_port);
    } else {
      sprintf(url, "server_log_%s:%d", "0.0.0.0", socket_port);
    }
    s_logger->set_log_file(fopen(url, "ab")); 

    socket_handle socket_server;
    struct sockaddr_in socket_server_addr;
     
    socket_server = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_server == SOCKET_INVALID) {
      char buf[256];
      int code = get_errno();
      sprintf(buf, "Could not create socket server. Error code: %d", code);
      s_logger->fatal(buf);
    }
    
    socket_server_addr.sin_family = AF_INET;
    if (socket_addr == NULL) {
      socket_server_addr.sin_addr.s_addr = INADDR_ANY;
    } else {
      socket_server_addr.sin_addr.s_addr = inet_addr(socket_addr);
    }

    socket_server_addr.sin_port = htons(socket_port);

    if (bind(socket_server,(struct sockaddr *)&socket_server_addr , sizeof(socket_server_addr)) < 0) {
      char buf[256];
      sprintf(buf, "Bind failed. Error code: %d", get_errno()); 
      s_logger->fatal(buf);
    }
    
    if (listen(socket_server, 3) < 0) {
      char buf[256];
      sprintf(buf, "Listen failed. Error code: %d", get_errno());
      s_logger->fatal(buf);
    }
    int sock_addr_int_size = sizeof(struct sockaddr_in);
    
    socket_handle socket_client;
    struct sockaddr_in socket_client_addr;

    while((socket_client = accept(socket_server, (struct sockaddr *) &socket_client_addr, (socklen_t *)&sock_addr_int_size)) ) {
      if (socket_client == SOCKET_INVALID) {
	char buf[256];
	sprintf(buf, "Accept failed. Error code: %d", get_errno());   
	s_logger->error(buf);
	msleep(1000);
	continue;
      }
      
      char buf[512];
      sprintf(buf, "New conection accepted from %s:%d", 
	      inet_ntoa(socket_client_addr.sin_addr), ntohs(socket_client_addr.sin_port));
      s_logger->info(buf);
      
      int params_size = sizeof(socket_client) + sizeof(Logger) + sizeof(int) + num_sessions*sizeof(SessionPtr);
      char * params = (char *) mem_alloc("Init params size of socket client", params_size);
      int pos = 0;
      memcpy(params + pos, (char *) (&socket_client), sizeof(socket_client));
      pos += sizeof(socket_client);
      memcpy(params + pos, (char *) s_logger, sizeof(Logger));
      pos += sizeof(Logger);
      memcpy(params + pos, (char *) (&num_sessions), sizeof(int));
      pos += sizeof(int);
      memcpy(params + pos, (char *) (sessions), num_sessions*sizeof(SessionPtr));
      
      pthread_t thread_id;
      if(thread_create(&thread_id , connection_handler, (void *) params) == API_ERROR) {
	char buf[512];
	sprintf(buf, "Start thread to handle connection failed. Error code: %d", get_errno());
	s_logger->error(buf);
      }
    }
  }
}
