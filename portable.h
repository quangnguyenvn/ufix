#ifndef _U_FIX_PORTABLE_
#define _U_FIX_PORTABLE_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BILLION 1000000000
#define MILLION 1000000
#define THOUSAND 1000

typedef unsigned long long ULONG64;

typedef unsigned int UINT;

#ifdef _POSIX_
#include <errno.h>
#include <execinfo.h>
#include <pthread.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>

#define unlikely(expr) __builtin_expect(!!(expr), 0)
#define likely(expr) __builtin_expect(!!(expr), 1)

#define thread_handle pthread_t
#define pthread_main void *
#define thread_create(x, y, z) pthread_create(x, NULL, y, z)
#define thread_name(x, y) pthread_setname_np(x, y) 

#define socket_handle int
#define SOCKET_INVALID -1
#define SOCKET_ERROR -1 
#define API_ERROR -1

#define _LOAD_MEMORY_BAR_    
#define _STORE_MEMORY_BAR_   
#define _MEMORY_BAR_ asm volatile("mfence" ::: "memory")

#define get_errno() errno

#define socket_send(x, y, z) send(x, y, z, MSG_NOSIGNAL)

#define msleep(x) usleep(x*THOUSAND)
#define socket_close(x) close(x)

#define DIR_DELIM '/'

#define init_sock_base() 

inline thread_handle get_thread_handle(pthread_t addr, int handle) {
	return addr;
}

inline void pthread_wait(pthread_cond_t * event, pthread_mutex_t * mutex, ULONG64 time_ms, int * status) {   
  if (time_ms == 0) {
    pthread_cond_wait(event, mutex);
  } else {
    timespec ts;    
    clock_gettime(CLOCK_MONOTONIC, &ts);
    
    ULONG64 nsecs = ((ULONG64) ts.tv_sec)*BILLION + ts.tv_nsec + time_ms*MILLION;
    ts.tv_sec =  nsecs/BILLION;    
    ts.tv_nsec = nsecs%BILLION;
    *status = pthread_cond_timedwait(event, mutex, &ts);
  }
}

inline int socket_set_recv_timeout(socket_handle socket, ULONG64 ms) {
  struct timeval tv;
  tv.tv_sec = ms/THOUSAND; 
  tv.tv_usec = 0L;
  return setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));
}

inline int socket_set_send_timeout(socket_handle socket, ULONG64 ms) {
  struct timeval tv;
  tv.tv_sec = ms/THOUSAND;
  tv.tv_usec = 0L;
  return setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(struct timeval));
}

inline void socket_set_nonblocking(socket_handle socket) {
  int fl = fcntl(socket, F_GETFL, 0);
  fcntl(socket, F_SETFL, fl | O_NONBLOCK);
}

inline ULONG64 get_current_timestamp() {
  timespec time;
  clock_gettime(CLOCK_MONOTONIC, &time);
  return time.tv_sec*MILLION + time.tv_nsec/THOUSAND;
}

inline int format_time(char * time_buf) {
  timespec time;
  clock_gettime(CLOCK_REALTIME, &time);
  struct tm * ptm = gmtime(&time.tv_sec);
  return sprintf(time_buf, "[%02d:%02d:%02d.%06d]", ptm->tm_hour, ptm->tm_min, ptm->tm_sec, (int) (time.tv_nsec/THOUSAND));
}

inline void print_stack_trace() {
  void* callstack[128];
  int frames = backtrace(callstack, 128);
  char** strs = backtrace_symbols(callstack, frames);
  for (int i = 0; i < frames; ++i) {
    printf("%s\n", strs[i]);
    free(strs[i]);
  }
}

#elif _WINDOWS_OS_

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>	
#include <process.h>

#define EAGAIN WSAEWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK
#define EEXIST ERROR_ALREADY_EXISTS
#define EINTR WSAEINTR
#define EISCONN WSAEISCONN
#define EINVAL WSAEINVAL
#define EINPROGRESS WSAEINPROGRESS

#define likely(expr) expr
#define unlikely(expr) expr

#define thread_handle HANDLE
#define pthread_t unsigned int
#define pthread_main unsigned int _stdcall
#define thread_create(x, y, z) _beginthreadex(NULL, 0, y, z, 0, x)
#define pthread_mutex_t CRITICAL_SECTION
#define pthread_mutex_lock(x) EnterCriticalSection(x)
#define pthread_mutex_trylock(x) TryEnterCriticalSection(x)
#define pthread_mutex_unlock(x) LeaveCriticalSection(x)
#define pthread_join(x, y) WaitForSingleObject(x, INFINITE)
#define pthread_cond_t HANDLE
#define pthread_condattr_t int
#define pthread_condattr_init(x) 1
#define pthread_condattr_setclock(x, y) 1
#define pthread_mutex_init(x, y) InitializeCriticalSection(x)
#define pthread_cond_signal(x) SetEvent(x)
#define thread_name(x, y) 
#define pthread_yield() Sleep(0)

#define _LOAD_MEMORY_BAR_   
#define _STORE_MEMORY_BAR_
#define _MEMORY_BAR_ MemoryBarrier()

#define backtrace(x,y) 0
#define backtrace_symbols(x, y) NULL

#define socket_handle SOCKET
#define SOCKET_INVALID INVALID_SOCKET
#define API_ERROR 0
#define get_errno() GetLastError()
#define msleep(x) Sleep(x)
#define socket_send(x, y, z) send(x, y, z, 0)
#define socket_close(x) closesocket(x)

#define mkdir(x, y) CreateDirectory(x, NULL)
#define DIR_DELIM '\\'
#define poll WSAPoll

inline void init_sock_base() {
	WSADATA wsaData;
	DWORD wVersionRequested = MAKEWORD(2, 2);
	int err = WSAStartup((WORD) wVersionRequested, &wsaData);
	if (err != 0) { 
		printf("ERROR: No usable winsock dll\n");
		msleep(5000);
		exit(-1);
	}

	if ( LOBYTE( wsaData.wVersion ) != 2 ||
        HIBYTE( wsaData.wVersion ) != 2 ) {                             
		WSACleanup();
		printf("ERROR: no winsock2 available\n");
		msleep(5000);
		exit(-1); 
	}
}

inline thread_handle get_thread_handle(pthread_t addr, int handle) {
	return (thread_handle) handle;
}

inline void pthread_wait(pthread_cond_t event, pthread_mutex_t * mutex, ULONG64 time_ms, int * status) {
  *status = WaitForSingleObject(event, time_ms);
}

inline int socket_set_recv_timeout(socket_handle socket, ULONG64 s) {  
  int timeout = s;
  return setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*) &timeout, sizeof(int));        
}

inline int socket_set_send_timeout(socket_handle socket, ULONG64 s) {
  int timeout = s;
  return setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (const char*) &timeout, sizeof(int));   
}  

inline void socket_set_nonblocking(socket_handle socket) {
	u_long mode = 1;
	ioctlsocket(socket, FIONBIO, &mode);
}

inline ULONG64 get_current_timestamp() {
 LARGE_INTEGER nTime;
 LARGE_INTEGER nFrequency;

 ::QueryPerformanceFrequency(&nFrequency); 
 ::QueryPerformanceCounter(&nTime);

 return (nTime.QuadPart*1000000)/nFrequency.QuadPart;
}

inline int format_time(char * time_buf) {
  SYSTEMTIME st;
  GetLocalTime(&st);
  return sprintf(time_buf, "[%02d:%02d:%02d.%06d]", st.wHour, st.wMinute, st.wSecond, (int) (st.wMilliseconds*THOUSAND));
}

inline int pthread_cond_init(pthread_cond_t * data_event, pthread_condattr_t * atrr) {
	HANDLE handle = CreateEvent(NULL, false, false, NULL);
	if (handle == NULL) {
		return API_ERROR;
	} else {
		*data_event = handle;
		return 1;
	}
}

inline void print_stack_trace() {
}

#endif

#endif
