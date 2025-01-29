#include "utils.h"
#include "PManager.h"

#define SEQS_PROCESSED "seqs_p"
#define SEQS_SENT "seqs_s"
#define SYSTEM_LOG "session.log"

namespace ufix {

  inline int move_file(const char * src, const char * dir) {
    const char * file_name = basename(src);
    char buf[512];
    sprintf(buf, "%s%c%s", dir, DIR_DELIM, file_name);
    
    if (rename(src, buf) == API_ERROR) {
      return 0;
    } 
    return 1;
  }

  inline int move_file(FileHandler * fh, const char * dir) {
    return move_file(fh->get_abs_path(), dir);
  }
  
  PManager::PManager(const char * p_dir, const char * s_dir, int num_s_queues) {
    
    num_send_queues = num_s_queues;
    dir_create(p_dir);
    
    dir_len = sprintf(dir, "%s%c%s", p_dir, DIR_DELIM, s_dir); 
    dir_create(dir);
    
    q_dir_len = sprintf(q_dir, "%s%c%s", dir, DIR_DELIM, "queue");                 
    dir_create(q_dir);

    sprintf(dir + dir_len, "%c", DIR_DELIM);
    dir_len++;

    sprintf(dir + dir_len, "%s", SEQS_PROCESSED);
    seqs_processed = new FileHandler(dir, "ab");
    
    sprintf(dir + dir_len, "%s", SEQS_SENT);
    seqs_sent = new FileHandler(dir, "ab");

    sprintf(dir + dir_len, "%s", SYSTEM_LOG);
    system_log = new FileHandler(dir, "ab");
    
    data_queue = (FileHandlerPtr *) mem_alloc("Init persistent data queue file", num_send_queues*sizeof(FileHandlerPtr));

    for (int i = 0; i < num_send_queues; i++) {
      data_queue[i] = NULL;
    }
  }
  
  FILE * PManager::get_system_log() {
    return system_log->get_file();
  }

  FILE * PManager::init_data_queue(int q_index) {
    sprintf(q_dir + q_dir_len, "%c%d", DIR_DELIM, q_index);
    data_queue[q_index] = new FileHandler(q_dir, "ab");
    return data_queue[q_index]->get_file();
  }
  
  FILE * PManager::get_data_recover_file(int q_index) {
    sprintf(q_dir + q_dir_len, "%c%d", DIR_DELIM, q_index);
    FileHandler fh(q_dir, "r");
    return fh.get_file();
  }
  
  void PManager::close(FILE * file) {
    if (file != NULL) {
      fclose(file);
    }
  }
  
  void PManager::close() {
    seqs_processed->close();
    seqs_sent->close();
    system_log->close();

    for (int i = 0; i < num_send_queues; i++) {
      if (data_queue[i] != NULL) {
	data_queue[i]->close();
      }
    }
  }
  
  int PManager::backup(const char * backup_dir) {
    char b_dir[512];
    int len = sprintf(b_dir, "%s%c%s%c%s", dir, DIR_DELIM, "history", DIR_DELIM, backup_dir);

    dir_create(b_dir);
    
    char b_dir_abs_path[512];    
    realpath(b_dir, b_dir_abs_path);

    int code;
    code = move_file(seqs_processed, b_dir_abs_path);
    if (!code) return 0;

    code = move_file(seqs_sent, b_dir_abs_path);
    if (!code) return 0;

    char q_dir_abs_path[512];
    realpath(q_dir, q_dir_abs_path);
    
    code = move_file(q_dir_abs_path, b_dir_abs_path);
    if (!code) return 0;
    
    code = move_file(system_log, b_dir_abs_path);
    if (!code) return 0;

    return 1;
  }

  void PManager::write_seq_processed(ULONG64 seq) {
    FILE * file = seqs_processed->get_file();
    fwrite((const char *) &seq, sizeof(char), sizeof(ULONG64), file);
    fflush(file);
  }

  void PManager::write_seq_sent(int q_index, ULONG64 seq, ULONG64 data_seq) {
    int buf[1 + 2*sizeof(ULONG64)/sizeof(int)];
    buf[0] = q_index;
    ULONG64 * seqs = (ULONG64 *) (buf + 1);
    seqs[0] = seq;
    seqs[1] = data_seq;
    FILE * file = seqs_sent->get_file();
    fwrite((const char *) buf, sizeof(char), sizeof(int) + 2*sizeof(ULONG64), file);
    fflush(file);
  }

  ULONG64 PManager::read_last_seq_processed() {
    sprintf(dir + dir_len, "%s", SEQS_PROCESSED);
    FILE * file_seqs_processed = fopen(dir, "rb");
    fseek(file_seqs_processed, 0, SEEK_END);
    int size = ftell(file_seqs_processed);
    if (size == 0) return 0;
    fseek(file_seqs_processed, 0 - sizeof(ULONG64), SEEK_CUR);
    char buf[8];
    fread(buf, sizeof(char), sizeof(ULONG64), file_seqs_processed);
    fclose(file_seqs_processed);
    return *((ULONG64 *) buf);
  }

  ULONG64 PManager::read_sent_marks(SentMark * marks, ULONG64 * last_seqs) {
    sprintf(dir + dir_len, "%s", SEQS_SENT);
    FILE * file_seqs_sent = fopen(dir, "rb");
    char buf[1 << 17];
    int max_read_size = (sizeof(int) + 2*sizeof(ULONG64))*(1 << 12);
    
    ULONG64 last_seq = 0;
    int last_q_index = 0;

    while (true) {
      int len = fread(buf, sizeof(char), max_read_size, file_seqs_sent);
      int pos = 0;
      while (true) {
	if (pos >= len) break;
	int * info = (int *) (buf + pos); 
	int q_index = info[0];
	pos += sizeof(int);
	ULONG64 * seqs = (ULONG64 *) (buf + pos);
	marks->set_mark(q_index, seqs[0], seqs[1]);
	if (q_index != last_q_index) {
          last_seqs[last_q_index] += (seqs[0] - 1 - last_seq);
	  last_q_index = q_index;
	}
	last_seqs[q_index] = seqs[1];
	last_seq = seqs[0];
	pos += 2*sizeof(ULONG64);
      }
      if (len != max_read_size) break;
    }
    fclose(file_seqs_sent);
    return last_seq;
  }
}
