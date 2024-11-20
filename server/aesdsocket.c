#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <pthread.h>
#include <time.h>
#include "my_utils.h"
#include "my_sockets.h"
#include "my_list.h"
#include "my_datafile.h"
#include "my_signals.h"
#include "my_timer.h"

static const char *pMessages[] = { "failed", "interrupted", "terminated", "interrupted and terminated"};

static int my_send(client_context_t *const cli) {
	int sent = 0;
	// we'll start from the beginning of the file
   if(pthread_mutex_lock(&cli->dfctx->lock) != 0) return -1;
   datafile_begin(cli->dfctx);
	do {
		const int szData = read(cli->dfctx->fd, cli->data, DEFAULT_BUFFER_SIZE);
		if(szData < 0) {
			const int en = errno;
			// if terminated by signal
         sent = 0;
			if(en == EINTR) sent = -1;
			break;
		}
		if(szData == 0) {
		   sent = 0;
		   break;
		}
		int cnt = 0;
		while(cnt < szData) {
			const int szSent = send(cli->fd, cli->data + cnt, szData, 0);
			if(szSent < 0) {
				const int en = errno;
				// if terminated by signal
            if((en == EAGAIN) || (errno == EWOULDBLOCK)) continue;
            sent = 0;
				if(en == EINTR) sent = -1;
				break;
			}
			cnt += szSent;
			sent += szSent;
		}
		if(sent < 0) break;
	} while(true);
   datafile_end(cli->dfctx);
   pthread_mutex_unlock(&cli->dfctx->lock);
   return sent;
}

static inline int my_simple_recv(client_context_t *const cli) {
   cli->size = 0;
   do {
      const int szData = recv(cli->fd, cli->data, DEFAULT_BUFFER_SIZE, 0);

      // socket failure
      if(szData == 0) break;

      // success
      if(szData > 0) {
         cli->size = szData;
         break;
      }

      // failure - signal, or socket, or anything else
      const int en = errno;
      // if terminated by signal
      if((en == EAGAIN) || (errno == EWOULDBLOCK)) continue;
      cli->size = 0;
      if(en == EINTR) cli->size = -1;
      break;

   } while(true);

   return cli->size;
}

static int my_recv(client_context_t *const cli) {
	int received = 0;
	do {
		const int szData = my_simple_recv(cli);
		if(szData <= 0) {
		   received = szData;
		   break;
		}

		char *ptr = cli->data;
		received += szData;
		int cnt = szData;
		char *begin = ptr;
		int size = 0;
		while(cnt--) {
			size++;
			if(*ptr++ == '\n') {
				if(datafile_write_safe(cli->dfctx, begin, size) < 0) return -1;
				if(my_send(cli) < 0) return -1;
				begin = ptr;
				size = 0;
			}
		}
		if(datafile_write_safe(cli->dfctx, begin, size) < 0) return -1;
	} while(true);
	return received;
}


static void *thread_network_client(void *arg) {
   list_t *const list = (list_t *)arg;
   my_recv(list->cli);
   // close the client socket
   close(list->cli->fd);
   // free the client memory
   free(list->cli->data);
   free(list->cli);
   // fingers crossed - we'll not get into a troubles with searching over the list,
   // so mark ourselves as destroyed thread
   list->cli = NULL;
   return list;
}

static int my_create_thread(list_t **ppHead, client_context_t *const cli) {
   if(*ppHead == NULL) *ppHead = list_malloc();
   if(*ppHead == NULL) return -1; // not enough memory
   list_t *last = list_findAvailableOrLast(*ppHead);
   if(last->cli != NULL) {
      last->next = list_malloc();
      if(last->next == NULL) return -1; // not enough memory
      last = last->next;
   }

   if(pthread_create(&last->thread, NULL, &thread_network_client, last) != 0) return -1; // cannot create a thread
   last->cli = cli;
   return 0;   // success
}

int main(int argc, char *argv[])
{
	bool daemonize = false;

	for(int i = 1; i < argc; ++i) {
		if(strcmp(argv[i], "-d") == 0) daemonize = true;
	}

   openlog(NULL, LOG_PERROR, LOG_USER);

   const int sockfd = my_socket();
   if(sockfd < 0) {
      const int en = errno;
      syslog(LOG_ERR, "Cannot create socket (errno %d), exiting", en);
      printf("Cannot create socket (errno %d), exiting" EOLN, en);
      return 1;
   }

   if(my_setsockopt(sockfd) != 0) {
      const int en = errno;
      syslog(LOG_ERR, "Cannot set socket options (errno %d), exiting", en);
      printf("Cannot set socket options (errno %d), exiting" EOLN, en);
      close(sockfd);
      return 1;
   }

   if(my_bind(sockfd) != 0) {
      const int en = errno;
      syslog(LOG_ERR, "Cannot bind socket (errno %d), exiting", en);
      printf("Cannot bind socket (errno %d), exiting" EOLN, en);
      close(sockfd);
      return 1;
   }

	init_signals();

   if(daemonize) {
      const int res = fork();
      if(res < 0) {
         const int en = errno;
         syslog(LOG_ERR, "Cannot fork (errno %d), exiting", en);
         printf("Cannot fork (errno %d), exiting" EOLN, en);
         close(sockfd);
         return 1;
   	}
      // parent should exit now
      if(res > 0) {
         syslog(LOG_INFO, "Successfully daemonized, pid=%d", res);
         printf("Successfully daemonized, pid=%d" EOLN, res);
         close(sockfd);
         return 0;
      }
   }

	if(my_listen(sockfd) != 0) {
		const int en = errno;
	   syslog(LOG_ERR, "Cannot listen socket (errno %d), exiting", en);
		printf("Cannot listen socket (errno %d), exiting" EOLN, en);
		close(sockfd);
		return 1;
	}

	datafile_context_t data;
	if(datafile_init(&data, cDataFile) < 0) {
		const int en = errno;
	   syslog(LOG_ERR, "Cannot instantiate data file (errno %d), exiting", en);
		printf("Cannot instantiate data file (errno %d), exiting" EOLN, en);
		close(sockfd);
		return 1;
	}

	// create a head of our list
	list_t *head = NULL;

	timer_t timer = my_create_timer(timestamp_handler, &data, 10);

   while(true) {
      client_context_t *const cli = my_accept(sockfd, daemonize);
      if(cli == NULL) break;

      cli->dfctx = &data;
      if(my_create_thread(&head, cli) != 0) {
         const int en = errno;
         syslog(LOG_ERR, "Cannot instantiate connection thread (errno %d), exiting", en);
         if(!daemonize) printf("Cannot instantiate connection thread (errno %d), exiting" EOLN, en);
         close(cli->fd);
         break;
      }
   }
   timer_delete(timer);

   sprintf(print_buff, "Application execution was %s, cleaning up.",
      pMessages[(int)bSignalInterrupt + ((int)bSignalTerminate * 2)]);
   syslog(LOG_INFO, "%s", print_buff);
   if(!daemonize) printf("%s" EOLN, print_buff);
   fflush(stdout);

   close(sockfd);
   list_t *list = head;
   int idx = 0;
   while(list != NULL) {
      if(list->cli != NULL) {
         // the client thread should know we're done here
         shutdown(list->cli->fd, SHUT_RDWR);
         void *ret = NULL;
         const int res = pthread_join(list->thread, &ret);
         sprintf(print_buff, "Joined thread #%d exits(%d) with useless arg=0x%08lx.", ++idx, res, (long int)ret);
         syslog(LOG_DEBUG, "%s", print_buff);
         if(!daemonize) printf("%s" EOLN, print_buff);
      }
      list_t *prev = list;
      list = list->next;
      free(prev);
   }
   datafile_kill(&data);

   syslog(LOG_INFO, "stopped.");
   if(!daemonize) printf("stopped.\n" EOLN);

   return 0;
}
