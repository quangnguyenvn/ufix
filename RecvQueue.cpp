#include "constants.h"
#include "RecvQueue.h"

namespace ufix {
  
  RecvQueue::RecvQueue(int size, Cmds * reqs, Logger * logger) {
    q_size = size;
    cmds = reqs;
    q_logger = logger;
    msgs_recv = (RMessage *) mem_alloc("Init msgs_recv queue", q_size*sizeof(RMessage));
    for (int i = 0; i < q_size;  i++) {
      msgs_recv[i].reset();
    }
    head = 1;
    head_seq = 0;
    tail_seq = 0;
  }
  
  void RecvQueue::set_head_seq(ULONG64 seq) {
    head = (seq + 1)%q_size;
    head_seq = seq;
    tail_seq = seq;
  }
  
  int RecvQueue::has_event() {
    return !is_empty(head);
  }

  void RecvQueue::put(RMessage * msg) {
    ULONG64 seq = msg->get_seq();
    int index = seq%q_size;
    
    ULONG64 slot_seq = msgs_recv[index].get_seq();
    
    if (slot_seq >= seq) {
      char buf[512];
      sprintf(buf, "Recv seq %lld while already recv seq %lld. Discard msg", seq, slot_seq);
      q_logger->warn(buf);
      return;
    }

    ULONG64 start_ts = 0;

    while (!msgs_recv[index].is_empty()) {
      if (start_ts == 0) start_ts = get_current_timestamp();
      msleep(1);
      ULONG64 current_ts = get_current_timestamp();
      if (current_ts > start_ts + 5*MILLION) {
	start_ts = current_ts;
	char buf[512];
	sprintf(buf, "Recv queue add msg to index %d failed. Queue full", index);
	q_logger->warn(buf);
      }  
    }

    msgs_recv[index].copy(msg);
    if (tail_seq < seq) {
      _STORE_MEMORY_BAR_;
      tail_seq = seq;
    }
    signal();
  }
  
  int RecvQueue::is_empty(int index) {
    return (msgs_recv[index].is_empty() || (head_seq >= msgs_recv[index].get_seq()));
  }

  int RecvQueue::add_resend_request() {
    ULONG64 begin_seq_no = head_seq + 1;
    int index = begin_seq_no%q_size;
    if (msgs_recv[index].get_timestamp() > 0) return 0;
    
    ULONG64 timestamp = get_current_timestamp();
    msgs_recv[index].set_timestamp(timestamp);
    
    ULONG64 end_seq_no = begin_seq_no + 1;
    
    while (true) {
      index = end_seq_no%q_size;
      if (is_empty(index)) {
	msgs_recv[index].set_timestamp(timestamp);
	end_seq_no++;
      } else {
	break;
      }
    }
    end_seq_no--;
    
    char cmd[CMD_BLOCK_SIZE];
    cmd[0] = MSG_TYPE_RESEND_REQUEST[0];
    memcpy(cmd + 1, (char *) &begin_seq_no, sizeof(ULONG64));
    memcpy(cmd + 1 + sizeof(ULONG64), (char *) &end_seq_no, sizeof(ULONG64));
    cmds->add_cmd(cmd);
    return 1;
  }
  
  void RecvQueue::update_seq(ULONG64 current_seq_no, ULONG64 new_seq_no) {
    for(ULONG64 i = current_seq_no + 1; i < new_seq_no; i++) {
      msgs_recv[i%q_size].set_gf(i);
    }
  }
  
  RMessage * RecvQueue::get() {
    ULONG64 start_ts = 0;
    
    while (is_empty(head)) {
      wait(1000, 0);
      if (!is_empty(head)) {
	break;
      }
      if (tail_seq > head_seq) {
	if (!add_resend_request()) {
	  ULONG64 current_ts = get_current_timestamp();
	  if (start_ts == 0) start_ts = current_ts;
	  if (current_ts > start_ts + 5*MILLION) {
	    start_ts = current_ts;
	    char buf[512];
	    sprintf(buf, "Waiting for seq %lld while current tail seq is %lld", head_seq, tail_seq);
	    q_logger->warn(buf);
	  }
	}
      }
    }
    
    RMessage * msg = msgs_recv + head;
    head++;
    if (head == q_size) head = 0;
    head_seq = msg->get_seq();
    
    return msg;
  }
}

 
