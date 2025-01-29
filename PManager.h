#ifndef _UFIX_P_MANAGER_
#define _UFIX_P_MANAGER_

#include "FileHandler.h"

#include "constants.h"
#include "SentMark.h"

namespace ufix {

typedef FileHandler * FileHandlerPtr;

  class PManager {

  private:
    
    char dir[1024]; 
    int dir_len;
    char q_dir[1024];
    int q_dir_len;

    FileHandler * seqs_processed;
    FileHandler * seqs_sent;
    FileHandlerPtr * data_queue;
    int num_send_queues;
    FileHandler * system_log;

  public:

    PManager(const char * p_dir, const char * s_dir, int num_send_queues);
    
    FILE * get_system_log();

    FILE * init_data_queue(int q_index);
    
    FILE * get_data_recover_file(int q_index);
    
    void close(FILE * file);
    
    void close();
    
    int backup(const char * backup_dir);

    void write_seq_sent(int q_index, ULONG64 seq, ULONG64 data_seq);
    
    void write_seq_processed(ULONG64 seq);
    
    ULONG64 read_last_seq_processed();
    
    ULONG64 read_sent_marks(SentMark * marks, ULONG64 * last_seqs);
  };
}
#endif
