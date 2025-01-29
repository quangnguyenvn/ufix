#include <assert.h>

#include "constants.h"
#include "utils.h"

#include "SendQueue.h"

namespace ufix {
  
  SendQueue::SendQueue(int index, int size, int reserve, int avg_msg_size, int max_msg_size, FILE * file, FILE * recover_file) {
    q_index = index;
    q_size = size;
    q_max_msgs = size - reserve;

    data_file = file;

    pthread_mutex_init(&copy_lock, NULL);
    
    q_flag = (int *) mem_alloc("Init q_flag", 1*sizeof(int));
    assert(q_flag != NULL);
    *q_flag = QUEUE_CLOSE;
    q_flag_threshold = QUEUE_OPEN;

    q_buf_size = q_size*avg_msg_size;
    q_buf = (char *) mem_alloc("Init q_buf", (q_buf_size + max_msg_size)*sizeof(char));
    q_buf_pos = 0;
    
    msgs_queue = (send_queue_info *) mem_alloc("Init msgs_queue send", q_size*sizeof(send_queue_info));
    for (int i = 0; i < q_size; i++) {
      msgs_queue[i].data_seq = 0;
    }
    data_seq = 1;
    recover_data_seq = 1;
    head_data_seq = 1;
    
    recover_data(recover_file);
  }
  
  void SendQueue::recover_data(FILE * file) {
    if (file == NULL) return;
    int buf_size = (1 << 24);
    char * buf = (char *) mem_alloc("Init buffer to recover data", buf_size);

    int buf_start = 0;

    while (true) {
      int nbytes = fread(buf + buf_start, sizeof(char), buf_size - buf_start, file);
      if (nbytes <= 0) break;  
      buf_start = recover_msgs(buf, buf_start + nbytes);
    }
    mem_free(buf);
    data_seq = recover_data_seq;
    printf("Recover data_seq %lld\n", recover_data_seq);
  }

  int SendQueue::recover_msgs(char * buf, int buf_len) {
    int msg_start = 0;
    while (true) {
      if (msg_start >= buf_len) {
	return 0;
      }
      while (true) {
	if (msg_start + sizeof(int) > buf_len) {
	  memcpy(buf, buf + msg_start, buf_len - msg_start);
	  return buf_len - msg_start;
	}
	if (convert_ctoi(buf[msg_start + sizeof(int)]) == BEGIN_STRING && 
	    buf[msg_start + sizeof(int) + 1] == EQUAL &&
	    buf[msg_start + sizeof(int) + 2] == 'F' &&
	    buf[msg_start + sizeof(int) + 3] == 'I') {
	  break;
	} else {
	  msg_start += 4;
	  continue;
	}
      }
      
      int msg_len = *((int *) (buf + msg_start));
      
      if (msg_start + sizeof(int) + msg_len > buf_len) {
	memcpy(buf, buf + msg_start, buf_len - msg_start); 
        return buf_len - msg_start;
      }
      recover_msg(buf + msg_start + sizeof(int), msg_len);
      msg_start += (sizeof(int) + msg_len);
    }
  }
  
  void SendQueue::recover_msg(const char * msg, int msg_len) {
    memcpy(q_buf + q_buf_pos, msg, msg_len);
    int index = recover_data_seq%q_size;

    send_queue_info * info = msgs_queue + index;
    info->data_seq = recover_data_seq;
    info->data = q_buf + q_buf_pos;
    info->len = msg_len;
    q_buf_pos += msg_len;
    if (q_buf_pos > q_buf_size) {
      q_buf_pos = 0;
    }
    recover_data_seq++;
  }

  void SendQueue::write_data(const char * data, int len) {
    fwrite((const char *)&len, sizeof(char), sizeof(int), data_file);
    fwrite(data, sizeof(char), len, data_file);
    fflush(data_file);
  }

  void SendQueue::set_q_flag_ref(int * flag) {
    q_flag = flag;
  }
    
  void SendQueue::set_q_flag_threshold(int threshold) {
    q_flag_threshold = threshold;
  }

  int * SendQueue::get_q_flag_ref() {
    return q_flag;
  }

  int SendQueue::is_close() {
    int is_close = (*q_flag < q_flag_threshold);
    if (is_close && *q_flag > QUEUE_CLOSE) {
      *q_flag = *q_flag + 1;
    }
    return is_close; 
  }

  void SendQueue::set_open() {
    *q_flag = QUEUE_OPEN;
  }
  
  void SendQueue::set_close() {
    *q_flag = QUEUE_CLOSE;
  }

  void SendQueue::mark_msg_sent(WMessage * msg) {
    head_data_seq++;
  }

  void SendQueue::mark_seq_sent(ULONG64 seq) {
    if (seq < head_data_seq) return;
    if (seq >= recover_data_seq) return;
    head_data_seq = seq + 1;
  }

  int SendQueue::is_queue_full() {
    return ((int) (data_seq - head_data_seq) >= q_max_msgs);
  }

  int SendQueue::is_queue_empty() {
    return (data_seq == head_data_seq);
  }
  
  int SendQueue::get_num_pending_msgs() {
    return (data_seq - head_data_seq);
  }

  void SendQueue::lock() {
    pthread_mutex_lock(&copy_lock);
  }

  void SendQueue::unlock() {
    pthread_mutex_unlock(&copy_lock);
  }

  int SendQueue::add_msg(WMessage * msg, int num_retries) {
    msg->pre_finalize();
    int len;
    const char * data = msg->get_data(len);
    
    lock();
    while (is_queue_full()) {
      unlock();
      num_retries--;
      if (num_retries == 0) return 0;
      msleep(1000);
      lock();
    }

    write_data(data, len);
    if (!msg->is_direct_buf()) {
      memcpy(q_buf + q_buf_pos, data, len);
    }
    int index = data_seq%q_size;
    
    send_queue_info * info = msgs_queue + index;
    
    info->data = q_buf + q_buf_pos;
    info->len = len;
    info->data_seq = data_seq;
    q_buf_pos += len;
    if (q_buf_pos > q_buf_size) {
      q_buf_pos = 0;
    }
    _STORE_MEMORY_BAR_; 
    data_seq++;
    unlock();
    return 1;
  }
  
  int SendQueue::get_msg(WMessage * msg) {
    if (is_queue_empty()) return 0;
    
    int head = (int) (head_data_seq%q_size);
    send_queue_info * info = msgs_queue + head;
    msg->switch_data(info->data, info->len);
    msg->set_data_seq(info->data_seq);
    msg->index_seq = head_data_seq;
    return 1;
  }

  int SendQueue::has_msg() {
    return !(is_queue_empty());
  }

  int SendQueue::get_sent_msg(WMessage * msg, ULONG64 seq) {
    int index = seq%q_size;
    send_queue_info * info = msgs_queue + index;
    if (info->data_seq != seq) {
      if (info->data_seq < seq) {
	int head_index = head_data_seq%q_size;
	if (msgs_queue[head_index].data_seq != 0 && msgs_queue[head_index].data_seq + q_size >= seq) {
	  mark_seq_sent(seq - 1);
	}
      } else {
	mark_seq_sent(seq);
      }
      return 0;
    } else {
      msg->switch_data(info->data, info->len);
      msg->set_data_seq(info->data_seq);
      mark_seq_sent(seq);
      return 1;
    }
  }

  char * SendQueue::get_new_msg_buf() {
    return q_buf + q_buf_pos;
  }

  ULONG64 SendQueue::get_head_data_seq() {
    return head_data_seq;
  }
}

 
