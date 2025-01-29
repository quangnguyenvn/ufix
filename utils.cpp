#include <assert.h>

#include "constants.h"
#include "utils.h"

namespace ufix {
  
  void * mem_alloc(const char * keyword, int size) {
    void * ptr = malloc(size);
    if (ptr == NULL) {
      printf("Request key_word %s %d bytes of memory failed. Errno %d\n", keyword, size, get_errno());
      
      print_stack_trace();
            
      char * debug = NULL;
      debug[0] = '1';
      exit(0);
    }
   
    return ptr;
  }
  
  void mem_free(void * val) {
    if (val != NULL) {
      free(val);
    }
  }

  void dir_create(const char * name) {
    int status = mkdir(name, 0744);
    if (status == 0) {
      return;
    }
    if (get_errno() == EEXIST) {
      return;
    }
    assert(status == 0);
  }

  int convert_itoa(int value, char * buf) {
    int pow = 10;
    int digits = 1;
    
    while (pow <= value) {
      pow *= 10;
      digits++;
    }
    
    for (int i = digits - 1; i >= 0; i--) {
      int new_value = value/10;
      buf[i] = convert_itoc(value - new_value*10);
      value = new_value;
    }
    return digits;
  }
  
  int convert_itoa(int value, char * buf, int buf_end) {
    int pos = buf_end - 1;
    while (true) {
      buf[pos] = convert_itoc(value % 10);
      value /= 10;
      if (value == 0) break;
      pos--;
    }
    return buf_end - pos;
  }

  int convert_ltoa(long long value, char * buf) {
    long long pow = 10;
    int digits = 1;

    while (pow <= value) {
      pow *= 10;
      digits++;
    }
    
    for (int i = digits - 1; i >= 0; i--) {
      long long new_value = value/10;
      buf[i] = convert_itoc(value - new_value*10);
      value = new_value;
    }
    return digits;
  }
  
  int convert_ltoa(long long value, char * buf, int buf_end) {
    int pos = buf_end - 1;
    while (true) {
      buf[pos] = convert_itoc((int) (value % 10));
      value /= 10;
      if (value == 0) break;
      pos--;
    }
    return buf_end - pos;
  }

  ULONG64 convert_atoll(const char * buf, int len) {
    ULONG64 val = 0;
    for (int i = 0; i < len; i++) {
      val = val*10 + convert_ctoi(buf[i]);
    }
    return val;
  }
  
  ULONG64 convert_atoll(const char * buf) {
    ULONG64 val = 0;
    int pos = 0;
    while (true) {
      if(buf[pos] < '0' || buf[pos] > '9') break;
      val = val*10 + convert_ctoi(buf[pos++]);
    }
    return val;
  }
  
  int convert_atoi(const char * buf, int len) {
    int val = 0;
    for (int i = 0; i < len; i++) {
      val = val*10 + convert_ctoi(buf[i]);
    }
    return val;
  }

  int convert_atoi(const char * buf) {
    int val = 0;
    int pos = 0;
    while (true) {
      if(buf[pos] < '0' || buf[pos] > '9') break;
      val = val*10 + convert_ctoi(buf[pos++]);
    }
    return val;
  }

  int is_char_end(char val) {
    return (val == '\0' || val == SOH);
  }
  
  int str_size(const char * str) {
    int size = 0;
    while (!is_char_end(str[size])) {
      size++;
    }
    return size;
  }

  int str_match(const char * src_val, const char * dest_val) {
    int pos = 0;
    while (true) {
      if (src_val[pos] != dest_val[pos]) {
	if (src_val[pos] == 0 && dest_val[pos] == SOH) {
	  return 1;
	} 
	if (src_val[pos] == SOH && dest_val[pos] == 0) {
	  return 1;
	}
	return 0;
      }
      if (src_val[pos] == 0 || src_val[pos] == SOH) {
	break;
      }
      pos++;
    }
    return 1;
  }
  
  int is_session_msg(const char * msg_type) {
    if (msg_type[1] != SOH) return 0;
    return ((msg_type[0] >= MSG_TYPE_HEARTBEAT[0] && msg_type[0] <= MSG_TYPE_LOGOUT[0]) ||
            msg_type[0] == MSG_TYPE_LOGON[0] ||
            msg_type[0] == MSG_TYPE_XML_NON_FIX[0]);
  }

  int seek(const char * data, int data_len, const char * pattern, int pattern_len) {
    int pos = 0;
    int prev_pos = 0;
    while (true) {
      for (int i = 0; i < pattern_len; i++) {
	if (pos == data_len) return -1;
	if (data[pos] == pattern[i]) {
	  pos++;
	} else {
	  pos++;
	  prev_pos = pos;
	  break;
	}
      }
      if (prev_pos != pos) {
	return prev_pos;
      }
    }
  }

  void poll_wait(int fd, int events, int timeout) {
    pollfd pollfds[1];
    memset((char *) &pollfds, 0, sizeof(pollfds));

    pollfds[0].fd = fd;
    pollfds[0].events = events;
    
    poll(pollfds, 1, timeout);
  }
}
