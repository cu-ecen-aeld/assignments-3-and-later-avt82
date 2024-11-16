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

static char print_buff[256];

#include "my_utils.h"
#include "my_sockets.h"
#include "my_signals.h"

#define DEFAULT_BUFFER_SIZE			4096
#define MAX_TEMPBUFF_SIZE			16384
#define TEMPFILE_COPY_SIZE			4096

static char const *const cDataFile = "/var/tmp/aesdsockedata";

typedef struct {
	char *filename;
	int fd;
	off_t pos;
	char *data;
} datafile_context_t;

static void datafile_confirm(datafile_context_t *const ctx) {
	lseek(ctx->fd, 0, SEEK_END);
	ctx->pos = lseek(ctx->fd, 0, SEEK_CUR);
}

static int datafile_init(datafile_context_t *const ctx, char const *const filename) {
	// stores the filename
	ctx->filename = (char*)filename;
	ctx->fd = open(ctx->filename, O_RDWR | O_CREAT | O_APPEND, 0666);
	if(ctx->fd <= 0) return -1;
	datafile_confirm(ctx);
	ctx->data = malloc(DEFAULT_BUFFER_SIZE);
	return (ctx->pos >= 0) && (ctx->data != NULL);
}

//static void datafile_trim(datafile_context_t *const ctx) {
//	ftruncate(ctx->fd, ctx->pos);
//	datafile_confirm(ctx);
//}

static void datafile_close(datafile_context_t *const ctx) {
	if(ctx->fd > 0) close(ctx->fd);
	if(ctx->data) free(ctx->data);
}

static void datafile_kill(datafile_context_t *const ctx) {
	datafile_close(ctx);
	remove(ctx->filename);
}

static int my_recv_write(int fd, char *data, int size) {
	int written = 0;
	while(written < size) {
		int res = write(fd, data + written, size);
		if(res < 0) {
			const int en = errno;
			if(en == EINTR) return -1;
			if((en == EAGAIN) || (errno == EWOULDBLOCK)) continue;
			return 0;
		}
		written += res;
	}
	return written;
}

static int my_send(int clientfd, datafile_context_t *const ctx) {
	int sent = 0;
	// we'll start from the beginning of the file
	lseek(ctx->fd, 0, SEEK_SET);
	do {
		const int szData = read(ctx->fd, ctx->data, DEFAULT_BUFFER_SIZE);
		if(szData < 0) {
			const int en = errno;
			// if terminated by signal
			if(en == EINTR) return -1;
			return 0;
		}
		if(szData == 0) return sent;
		int cnt = 0;
		while(cnt < szData) {
			const int szSent = send(clientfd, ctx->data + cnt, szData, 0);
			if(szSent < 0) {
				const int en = errno;
				// if terminated by signal
				if(en == EINTR) return -1;
				if((en == EAGAIN) || (errno == EWOULDBLOCK)) continue;
				return sent;
			}
			cnt += szSent;
			sent += szSent;
		}
	} while(true);
}

static int my_recv(int clientfd, datafile_context_t *const ctx) {
	int received = 0;
	do {
		const int szData = recv(clientfd, ctx->data, DEFAULT_BUFFER_SIZE, 0);
		if(szData < 0) {
			const int en = errno;
			// if terminated by signal
			if(en == EINTR) return -1;
			if((en == EAGAIN) || (errno == EWOULDBLOCK)) continue;
			return received;
		}
		if(szData == 0) return received;
		char *ptr = ctx->data;
		received += szData;
		int cnt = szData;
		char *begin = ptr;
		int size = 0;
		while(cnt--) {
			size++;
			if(*ptr++ == '\n') {
				if(my_recv_write(ctx->fd, begin, size) < 0) return -1;
				datafile_confirm(ctx);
				if(my_send(clientfd, ctx) < 0) return -1;
				begin = ptr;
				size = 0;
			}
		}
		my_recv_write(ctx->fd, begin, size);
		datafile_confirm(ctx);
	} while(true);
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
    		syslog(LOG_INFO, "Successfully daemonized");
    		printf("Successfully daemonized" EOLN);
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
	if(!datafile_init(&data, cDataFile)) {
		const int en = errno;
	    syslog(LOG_ERR, "Cannot instantiate data file (errno %d), exiting", en);
		printf("Cannot instantiate data file (errno %d), exiting" EOLN, en);
		close(sockfd);
		return 1;
	}

    while(true) {
		const int clientfd = my_accept(sockfd, daemonize);
		if(clientfd < 0) {
			const int en = errno;
			// if terminated by signal
			if(en == EINTR) break;
			// should never get here, I suppose
			syslog(LOG_ERR, "Cannot accept connection (errno %d), exiting", en);
			printf("Cannot accept connection (errno %d), exiting" EOLN, en);
			close(sockfd);
			datafile_close(&data);
			return 1;
		}

		if(my_recv(clientfd, &data) < 0) {
			close(clientfd);
			break;
		}
		// delete all that has no EOLn after
//		datafile_trim(&data);

//		if(my_send(clientfd, &data) < 0) {
//			close(clientfd);
//			break;
//		}

    }
    close(sockfd);
    datafile_kill(&data);

    static const char *pMessages[] = { "WTF?", "interrupted", "terminated", "interrupted and terminated"};

    sprintf(print_buff, "Application execution was %s, exiting",
    		pMessages[(int)bSignalInterrupt + ((int)bSignalTerminate * 2)]);
	syslog(LOG_INFO, "%s", print_buff);
    printf("%s" EOLN, print_buff);

    return 0;
}
