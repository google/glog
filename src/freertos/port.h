
#ifndef CTEMPLATE_FREERTOS_PORT_H_
#define CTEMPLATE_FREERTOS_PORT_H_

#ifdef FREERTOS

#include <stdint.h>

// ----------------------------------- THREADS
typedef uint32_t pthread_t;
//typedef DWORD pthread_key_t;
//typedef LONG pthread_once_t;
//enum { PTHREAD_ONCE_INIT = 0 };   // important that this be 0! for SpinLock
//#define pthread_self  GetCurrentThreadId
#define pthread_self getThreadId
#define pthread_equal(pthread_t_1, pthread_t_2)  ((pthread_t_1)==(pthread_t_2))

int getThreadId() { return 0; }

#endif  /* FREERTOS */

#endif  /* CTEMPLATE_FREERTOS_PORT_H_ */
