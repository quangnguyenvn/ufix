#include "utils.h"

namespace ufix {
  class SentMark {
  
  private:
    int mark_size;
    seq_sent_info * marks;
    
  public:
    
    SentMark(int size);
    void set_mark(int q_index, ULONG64 msg_seq, ULONG64 data_seq);
    seq_sent_info * get_mark(ULONG64 seq);
    seq_sent_info * get_last_mark(ULONG64 seq);
  };
}
