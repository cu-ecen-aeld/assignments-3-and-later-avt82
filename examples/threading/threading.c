#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

static inline void msleep(int msec) {
	const long int usec = msec * 1000;
	usleep(usec);
}

void* threadfunc(void* thread_param)
{
	struct thread_data *const pData = (struct thread_data *)thread_param;

	msleep(pData->wait_to_obtain_ms);
	if(pthread_mutex_lock(pData->mutex) == 0) {

		msleep(pData->wait_to_release_ms);
		pthread_mutex_unlock(pData->mutex);
		pData->thread_complete_success = true;
	}

    return (void*)pData;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex, int wait_to_obtain_ms, int wait_to_release_ms)
{
	if((thread == NULL) || (mutex == NULL)) return false;

	struct thread_data *const pData = (struct thread_data *)calloc(sizeof(char), sizeof(struct thread_data));
	pData->mutex              = mutex;
	pData->wait_to_obtain_ms  = wait_to_obtain_ms;
	pData->wait_to_release_ms = wait_to_release_ms;

	return pthread_create(thread, NULL, threadfunc, (void*)pData) == 0;
}

