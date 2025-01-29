#ifndef _UFIX_CMDS_
#define _UFIX_CMDS_

namespace ufix {
  class Cmds {
  private:
    pthread_mutex_t lock;
    char * cmds;
    int head;
    int tail;
  public:
    Cmds();
    void add_cmd(const char * cmd);
    const char * get_next_cmd();
  };
}
#endif
