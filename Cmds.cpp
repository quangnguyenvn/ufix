#include "utils.h"
#include "Cmds.h"
#include "constants.h"

#define MAX_CMDS 1024
namespace ufix  {
  Cmds::Cmds() {
    pthread_mutex_init(&lock, NULL);
    cmds = (char *) mem_alloc("Init cmds", CMD_BLOCK_SIZE*MAX_CMDS*sizeof(char));
    head = 0;
    tail = 0;
  }

  void Cmds::add_cmd(const char * cmd) {
    pthread_mutex_lock(&lock);
    memcpy(cmds + tail*CMD_BLOCK_SIZE*sizeof(char), cmd, CMD_BLOCK_SIZE*sizeof(char));
    
    int new_tail = tail + 1;
    tail = (new_tail == MAX_CMDS)?0:new_tail;
    pthread_mutex_unlock(&lock);
  }

  const char * Cmds::get_next_cmd() {
    if (head == tail) return NULL;
    const char * cmd = cmds + head*CMD_BLOCK_SIZE*sizeof(char);
    head++;
    if (head == MAX_CMDS) head = 0;
    return cmd;
  }
}
