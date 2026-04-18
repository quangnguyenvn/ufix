#include <cstdio>
#include <iostream>
#include <string>

#include "Session.h"
#include "SessionOption.h"
#include "SimpleLogger.h"
#include "SocketConnector.h"
#include "SocketServer.h"

using namespace ufix;

class DemoSession : public Session {
public:
  DemoSession(SessionOption* opt, Logger* logger) : Session(opt, logger) {}

protected:
  void ready_for_data() override {
    get_logger()->info("DemoSession ready_for_data");
  }

  void pre_process_msg(RMessage* msg) override {
    (void)msg;
  }

  void on_session_msg(RMessage* msg) override {
    int len = 0;
    msg->get_data(len);
    char buf[128];
    std::snprintf(buf, sizeof(buf), "Session message received, len=%d", len);
    get_logger()->info(buf);
  }

  void on_data_msg(RMessage* msg) override {
    int len = 0;
    msg->get_data(len);
    char buf[128];
    std::snprintf(buf, sizeof(buf), "Data message received, len=%d", len);
    get_logger()->info(buf);
  }
};

int main(int argc, char** argv) {
  init_sock_base();

  if (argc < 2) {
    std::cout << "Usage: app <initiator|acceptor>\n";
    return 1;
  }
  std::string mode = argv[1];

  SessionOption opt;
  opt.session_id = "DEMO";
  opt.fix_version = "FIX.4.4";
  if (mode == "initiator") {
    opt.sender_comp_id = "CLIENT1";
    opt.target_comp_id = "SERVER1";
  } else {
    opt.sender_comp_id = "SERVER1";
    opt.target_comp_id = "CLIENT1";
  }
  opt.heart_bt_in = 30;
  opt.p_dir = "data";
  opt.recv_queue_size = 1 << 12;
  opt.num_send_queues = 1;

  SimpleLogger logger;
  DemoSession session(&opt, &logger);
  session.start();

  if (mode == "initiator") {
    SocketConnector connector(&session);
    connector.set_socket_addr("127.0.0.1");
    connector.set_socket_port(5001);
    connector.start();
    connector.join();
    return 0;
  }

  if (mode == "acceptor") {
    SocketServer server(1);
    server.set_socket_addr("0.0.0.0");
    server.set_socket_port(5001);
    server.add_session(&session);
    server.start();
    while (true) {
      msleep(1000);
    }
  }

  std::cout << "Unknown mode: " << mode << "\n";
  return 1;
}
