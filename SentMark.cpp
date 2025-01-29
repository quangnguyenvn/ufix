#include "constants.h"
#include "SentMark.h"

namespace ufix {
  SentMark::SentMark(int size) {
    mark_size = size;
    marks = (seq_sent_info *) mem_alloc("Init sent marks", size*sizeof(seq_sent_info));
    memset(marks, 0, size*sizeof(seq_sent_info));
  }

  seq_sent_info * SentMark::get_mark(ULONG64 seq) {
    int index = seq%mark_size;
    seq_sent_info * info = marks + index;
    if (info->msg_seq == seq) return info;
    return NULL;
  }

  seq_sent_info * SentMark::get_last_mark(ULONG64 seq) {
    while(seq > 0) {
      seq_sent_info * info = get_mark(--seq);
      if (info != NULL) return info;
    }
    return NULL;
  }

  void SentMark::set_mark(int q_index, ULONG64 msg_seq, ULONG64 data_seq) {
    int index = msg_seq%mark_size;
    seq_sent_info * info = marks + index;
    info->q_index = q_index;
    info->msg_seq = msg_seq;
    info->data_seq = data_seq;
  }
}
