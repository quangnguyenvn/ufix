/**
   SocketConnector manage the connection (and FIX session) to exchange via tcp socket.
   @author: AnhVu
   @LastUpdateDate: 2019 March 01
 **/

#ifndef _UFIX_SOCKET_CONNECTOR_
#define _UFIX_SOCKET_CONNECTOR_

#include "portable.h"
#include "Session.h"
#include "RMessage.h"
#include "Logger.h"

namespace ufix {
  
 class SocketConnector {
 private:
   int socket_type;
   socket_handle socket_desc;
   char * socket_addr;
   int socket_port;
   sockaddr_in socket_addr_in;
   
   int socket_send_flag;

   thread_handle thread_recv, thread_send, thread_timer;

   Session * session;
   
   char * recv_buf;
   int recv_buf_head, recv_buf_tail, recv_buf_size;

   /**
    * Allocate socket address structure and parse information based on text form address. 
    **/
   void init_socket_addr();
   
   /**                                                                                                                        
    * Make attempts to connect to the exchange.
    * Auto retries until connection successful.
    **/
   void socket_connect();
   
   /**
    * Reset socket. Clear allocate structure.
    * Call socket_create to allocate new structure.
    * Call socket_init to init the socket.
    **/
   void socket_reset();

   /**                                                                                                                             
    * Allocate new structure and create the socket.   
    **/
   void socket_create();
   
   /**                                                                                                                             
    * Init the socket.
    * Set socket to non-blocking mode.
    * Set options TCP_NO_DELAY, SO_LINGER, SO_RCVBUF, SO_SNDBUF
    **/
   void socket_init();
   
   /**
    * process incomming data (streaming bytes)
    * Extract msgs from bytes then process. 
    **/
   int process_new_data();

   /**
    * Process incomming FIX msg 
    * @param msg: FIX msg to be processed
    **/
   void process_msg(RMessage * msg);

   /**                                                                                                                             
    * Process session msg      
    * @msg: FIX msg to be processed
    **/
   void process_session_msg(RMessage * msg);

   /**                                                                                                                             
    * Process heartbeat msg
    * @msg: FIX msg to be processed
    * @note: hearbeat msg is used to determine if the other side is working normally.
    **/
   void process_msg_heartbeat(RMessage * msg);
   
   /**
    * Process test request msg
    * @param msg: FIX msg to be processed                                                                   
    * @note: test request msg is used by other side to force an heartbeat sent out from our side.  
    **/
   void process_msg_test_request(RMessage * msg);

   /**
    * Process resend request msg
    * @param msg: FIX msg to be processed
    * @note: resend request msg is used by other side to recover some lost msgs
    **/  
   void process_msg_resend_request(RMessage * msg);
   
   /**
    * Process reject msg
    * @param msg: FIX msg to be processed
    **/  
   void process_msg_reject(RMessage * msg);

   /**
    * Process sequence reset msg
    * @param msg: FIX msg to be processed
    * @note: sequence reset msg is used to reset session sequences. 
    *        It could cause some pending messages to be lost.
    **/  
   void process_msg_sequence_reset(RMessage * msg);

   /**
    * Process logout msg.
    * @param msg: FIX msg to be processed
    **/
   void process_msg_logout(RMessage * msg);
   
   /**
    * Process logon msg.
    * @param msg: FIX msg to be processed.
    **/
   void process_msg_logon(RMessage * msg);
   
   /**
    * Send msg out via socket
    * @param msg: FIX msg to be sent
    * @param new_msg_flag: indicate msg is new or resend
    * @param data_flag: indicate msg is data or session msg
    **/  
   int send_msg(WMessage * msg, int new_msg_flag, int data_flag);

   /**
    * Send command to other side
    * Command will be converted to FIX msg before sending out
    * @param cmd: command to be sent
    **/  
   int send_cmd(const char * cmd);

   /**
    * Send all pending commands to other side 
    **/  
   void send_cmds();

   /**
    * Send data available in queue to the other side
    * data is a list of FIX msgs
    **/
   int send_data();
   
   /**
    * Send logon msg to other side (initiating session establishement)  
    **/  
   int send_logon();
   
   /**
    * Send heartbeat to other side.
    * Heartbeat msg is used to notify other side that the session is running normally.
    * Heartbeat msg will be sent out periodically in idle time.
    **/
   int send_heartbeat(const char * id);

   /**
    * Send test request to other side to force a heartbeat back from them.
    * TestRequest msg is used to determine if the other side is dead.
    * If no response from other side following a test request in a timely manner, the session will be reset.
    **/  
   int send_test_request();

   /**
    * Send resend request to other side to recover some lost msgs.
    * @param cmd: command to be sent 
    **/  
   int send_resend_request(const char * cmd);
   
   /**
    * Send sequence reset to other side to ask them to reset sequence.
    * SequenceReset msg is used when some messages are lost but there is no need to retransmit them (like heartbeats)
    * @param cmd: command to be sent  
    **/  
   int send_sequence_reset(const char * cmd);
   
   /**
    * Send sequence reset to other side to ask them to reset sequence.
    * SequenceReset msg is used when some messages are lost but there is no need to retransmit them (like heartbeats)  
    * @param start_seq: seq before resetting
    * @param end_seq: seq after resetting
    **/  
   int send_sequence_reset(ULONG64 start_seq, ULONG64 end_seq);
   
   /**
    * Maintain a wall clock accross all threads   
    **/  
   void update_utc_time();
   
   /**
    * Log message to file then terminate the process.
    * Used when encounter some serious error that couldn't be handled.
    * @param msg: message to be log out.
    **/
   void log_fatal(const char * msg);
   
   /**
    * Log message to file with ERROR tag.
    * Used when encounter some serious error.
    * @param msg: message to be logged out.
    **/
   void log_error(const char * msg);

   /**
    * Log message to file with WARN tag.
    * Used for some errors or important information need specific attention.
    * @param msg: message to be logged out.
    **/  
   void log_warn(const char * msg);

   /**
    * Log message to file with INFO tag
    * Used to output standard information 
    * @param msg: message to be logged out.
    **/
   void log_info(const char * msg);
   
   /**
    * Log data msg sent out to file 
    * This method is abstract at session layer.
    * The actual implementation will be on application layer. For example, not log any data messages for performance reason.
    * @param msg: message to be logged out.
    **/
   void log_data_msg_sent(WMessage * msg);

   /**
    * Log data msg recv in to file
    * This method is abstract at session layer.
    * The actual implementation will be on application layer. For example, not log any data messages for performance reason.
    * @param msg: message to be logged out. 
    **/   
   void log_data_msg_recv(RMessage * msg);

   /**
    * Log session msg sent out to file
    * Session msgs will always be logged out to let operator know about session status.
    * @param msg: message to be logged out.
    **/
   void log_session_msg_sent(WMessage * msg);

   /**
    * Log session msg recv in to file
    * Session msgs will always be logged out to let operator know about session status.   
    * @param msg: message to be logged out.                                  
    **/
   void log_session_msg_recv(RMessage * msg); 

   /**
    * Log msg error recv. It could be a message of wrong format or wrong business tag.
    * It should never happened if both side act rightfully.
    * @param msg: message to be logged out.
    * @param len: len of the message
    * @param error: error detected in text
    **/
   void log_msg_error(const char * msg, int len, const char * error);
   
 public:

   /**
    * Constructor (acceptor side)
    * Create a socket connector and assign it to a resumed session
    * @param temp_buf: temporary buffer used when resuming opoeration is ongoing. 
    * @param temp_buf_end: end of the buffer position
    * @param socket: socket to handle the connection
    * @param s: session to be resumed
    **/
   SocketConnector(const char * temp_buf, int temp_buf_end, socket_handle socket, Session * s);
   
   /**
    * Constructor (initiator side)
    * Create a socket connector based on a session
    * @param s: session to be established
    **/
   SocketConnector(Session * s);
   
   /**
    * Wait for all the other threads to terminate   
    **/
   void join();
   
   /**
    * Set address of the socket
    * @param socket_addr: address in text format (ip or domain name)
    **/
   void set_socket_addr(const char * socket_addr);
   
   /**
    * Set port of the socket
    * @param: port number of the socket. Example 8080
    **/
   void set_socket_port(int socket_port);
   
   /**
    * Start all threads of this SocketConnector
    **/
   void start();
   
   /**
    * Maintain a wall clock accross all threads
    **/
   void keep_update_utc_time();

   /**
    * Infinite loop to recv incomming data and process msgs  
    **/
   void recv_msgs();

   /**
    * Infinite loop to send out msgs avalable in queue
    **/
   void send_msgs();
 };
}

#endif
