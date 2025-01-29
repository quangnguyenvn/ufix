
#include "portable.h"
#include "constants.h"
#include "utils.h"

#include "WMessage.h"

namespace ufix {
  
  WMessage::WMessage(int index) {
    status = PRE_FIN;
    q_index = index;
  }

  WMessage::WMessage(int index, char * message) {
    status = NEW;
    q_index = index;
    data = message;
    data_len = 0;
    if (message == NULL) {
      direct_buf_flag = 1;
    } else {
      direct_buf_flag = 0;
    }
  }
  
  int WMessage::get_q_index() {
    return q_index;
  }
  
  void WMessage::set_q_index(int index) {
    if (q_index < 0) q_index = index;
  }

  void WMessage::set_max_body_length_size(int size) {
    max_body_length_size = size;
  }

  const char * WMessage::get_data() {
    return data;
  }

  const char * WMessage::get_data(int & len) {
    len = data_len;
    return data;
  }
  
  void WMessage::switch_data(char * msg, int len) {
    data = msg;
    data_len = len;
  }
  
  int WMessage::is_direct_buf() {
    return (direct_buf_flag == 1);
  }
  
  void WMessage::set_tag(int tag) {
    int nbytes = convert_itoa(tag, data + data_len);
    data_len += nbytes;
    data[data_len++] = EQUAL;
  }

  void WMessage::reserve_field(int tag, int len) {
    set_tag(tag);
    memset(data + data_len, '0', len);
    data_len += len;
    data[data_len++] = SOH;
  }

  ULONG64 WMessage::get_seq() {
    return seq;
  }

  ULONG64 WMessage::get_data_seq() {
    return data_seq;
  }

  void WMessage::set_data_seq(ULONG64 seq_num) {
    data_seq = seq_num;
  }

  void WMessage::set_field(int tag, const char * value, int len) {
    set_tag(tag);
    memcpy(data + data_len, value, len);
    data_len += len;
    data[data_len++] = SOH;
  }
  
  void WMessage::set_checksum(char * msg, unsigned int cks) {
    cks %= 256;
    unsigned int digit = cks/100; 
    msg[0] = convert_itoc(digit);
    cks -= digit*100;
    digit = cks/10;
    msg[1] = convert_itoc(digit);
    cks -= digit*10;
    msg[2] = convert_itoc(cks); 
  }

  void WMessage::set_field(int tag, const char * value) {
    set_tag(tag);
    
    int pos = 0;
    while (value[pos] != '\0' && value[pos] != SOH) {
      data[data_len++] = value[pos++];
    }
    data[data_len++] = SOH;
  }

  void WMessage::set_field(int tag, int value) {
    set_tag(tag);

    int nbytes = convert_itoa(value, data + data_len);
    data_len += nbytes;
    data[data_len++] = SOH;
  }

  void WMessage::set_field(int tag, double value) {
    set_tag(tag);

    int nbytes = sprintf(data + data_len, "%f", value);
    data_len += nbytes;
    data[data_len++] = SOH;
  }

  void WMessage::set_field(int tag, long long value) {
    set_tag(tag);

    int nbytes = convert_ltoa(value, data + data_len);
    data_len += nbytes;
    data[data_len++] = SOH;
  }
  
  void WMessage::set_field(int tag, ULONG64 value) {
    set_tag(tag);

    int nbytes = convert_ltoa(value, data + data_len);
    data_len += nbytes;
    data[data_len++] = SOH;
  }

  void WMessage::set_field(int tag, char value) {
    set_tag(tag);

    data[data_len++] = value;
    data[data_len++] = SOH;
  }

  void WMessage::set_field(int tag, tm * ptm) {
    set_tag(tag);

    int sec = (ptm->tm_sec < 60)?ptm->tm_sec:59;
    
    int nbytes = convert_itoa(ptm->tm_year + 1900, data + data_len, 4);
    memset(data + data_len, '0', 4 - nbytes); 
    data_len += 4;
    nbytes = convert_itoa(ptm->tm_mon + 1, data + data_len, 2);
    if (nbytes < 2) {
      data[data_len] = '0';
    }
    data_len += 2;
    nbytes = convert_itoa(ptm->tm_mday, data + data_len, 2);
    if (nbytes < 2) {
      data[data_len] = '0';
    }
    data_len += 2;
    data[data_len++] = '-';
    nbytes = convert_itoa(ptm->tm_hour, data + data_len, 2); 
    if (nbytes < 2) {
      data[data_len] = '0';
    }
    data_len += 2;
    data[data_len++] = ':';
    nbytes = convert_itoa(ptm->tm_min, data + data_len, 2);          
    if (nbytes < 2) {
      data[data_len] = '0';
    }
    data_len += 2;
    data[data_len++] = ':';
    nbytes = convert_itoa(sec, data + data_len, 2);    
    if (nbytes < 2) {
      data[data_len] = '0';
    }
    data_len += 2;
    memcpy(data + data_len, ".000", 4);
    data_len += 4;
    
    data[data_len++] = SOH;
  }
  
  void WMessage::copy(int tag, RMessage * rmsg) {
    field_info * info = rmsg->get_field_ptr(tag);
    if (info != NULL) {
      set_field(tag, info->val, info->val_len);
    }
  }

  void WMessage::mark_body_length_pos(int pos) {
    body_length_pos = pos;
  }
  
  void WMessage::mark_body_end_pos(int pos) {
    body_end_pos = pos;
  }
  
  void WMessage::pre_finalize() {
    convert_itoa(data_len - body_length_pos - max_body_length_size - 1, data + body_length_pos, body_end_pos - body_length_pos);
    
    set_tag(CHECKSUM);
    unsigned int cks = 0;
    for(int i = 0; i < data_len - CHECKSUM_TAG_PART_SIZE; i++) {
      cks += (unsigned int) data[i];
    }

    set_checksum(data + data_len, cks);
    data_len += CHECKSUM_TAG_PART_SIZE;
    data[data_len++] = SOH;
    status = PRE_FIN;
  }
  
  void WMessage::finalize(int msg_seq_pos, const char * msg_seq, int msg_seq_len,
			  int sending_time_pos, const char * utc_time, int resend_flag) {
    if (unlikely(resend_flag)) {
      finalize_resend(msg_seq_pos, msg_seq, msg_seq_len, sending_time_pos, utc_time);
    } else {
      finalize_new(msg_seq_pos, msg_seq, msg_seq_len, sending_time_pos, utc_time);
    }
  }
  
  void WMessage::finalize_new(int msg_seq_pos, const char * msg_seq, int msg_seq_len, 
			      int sending_time_pos, const char * utc_time) {
    if (status == NEW) {
      pre_finalize();
    }
    
    unsigned int cks = convert_atoi(data + data_len - CHECKSUM_VALUE_PART_SIZE);
    memcpy(data + msg_seq_pos, msg_seq, msg_seq_len);
    seq = convert_atoll(msg_seq, msg_seq_len);
    
    for (int i = 0; i < msg_seq_len; i++) {
      cks += ((unsigned int) msg_seq[i] - '0');
    }
    
    memcpy(data + sending_time_pos, utc_time, SENDING_TIME_SIZE);
    memcpy(data + sending_time_pos + DISTANCE_SENDING_ORIG_SENDING, utc_time, SENDING_TIME_SIZE);

    for (int i = 0; i < SENDING_TIME_SIZE; i++) {
      cks += (((unsigned int) utc_time[i] - '0') << 1);
    }
    
    set_checksum(data + data_len - CHECKSUM_VALUE_PART_SIZE, cks);
  }
  
  void WMessage::finalize_resend(int msg_seq_pos, const char * msg_seq, int msg_seq_len, 
				 int sending_time_pos, const char * utc_time) {
    memcpy(data + msg_seq_pos, msg_seq, msg_seq_len);
    seq = convert_atoll(msg_seq, msg_seq_len);
    
    data[sending_time_pos - 5] = 'Y';
    memcpy(data + sending_time_pos, utc_time, SENDING_TIME_SIZE);
    if (data[sending_time_pos + DISTANCE_SENDING_ORIG_SENDING] == '0') {
      memcpy(data + sending_time_pos + DISTANCE_SENDING_ORIG_SENDING, utc_time, SENDING_TIME_SIZE);
    }
    
    unsigned int cks = 0;
    for(int i = 0; i < data_len - CHECKSUM_SIZE; i++) {
      cks += (unsigned int) data[i];
    }

    set_checksum(data + data_len - CHECKSUM_VALUE_PART_SIZE, cks);
  }
}
