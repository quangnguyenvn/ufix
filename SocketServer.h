#ifndef _UFIX_SOCKET_SERVER_
#define _UFIX_SOCKET_SERVER_

#include "Session.h"
#include "Logger.h"

namespace ufix {

 class SocketServer {
   
 private:
   pthread_t thread_server_addr;

   Logger * s_logger;
   
   SessionPtr * sessions;
   int num_sessions;
   
   char * socket_addr;
   int socket_port;
   
   
 public:
   SocketServer(int max_num_sessions);
   void set_socket_addr(const char * socket_addr);
   void set_socket_port(int socket_port);
   void add_session(Session * sessions);
   
   void start();
   void run();
 };
}
#endif
