/*
 * sockets.h
 *
 *  Created on: Nov 16, 2024
 *      Author: user
 */

#ifndef SERVER_MY_SOCKETS_H_
#define SERVER_MY_SOCKETS_H_

#define DEFAULT_BUFFER_SIZE			4096
#define DEFAULT_LISTENING_PORT      9000

static inline int my_socket(void) {
	return socket(AF_INET, SOCK_STREAM, 0);
}

static inline int my_bind(int sockfd) {
	struct sockaddr_in addr;
	(void)memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(DEFAULT_LISTENING_PORT);
	return bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
}

static inline int my_setsockopt(int sockfd) {
	int reuse = (int)true;
   return setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void*)&reuse, sizeof(reuse));
}

static inline int my_listen(int sockfd) {
	return listen(sockfd,20);
}

static inline client_context_t *my_accept(int sockfd, const bool daemonized) {
	struct sockaddr_in addr;
   socklen_t len = sizeof(addr);
	(void)memset(&addr, 0, sizeof(addr));
	const int res = accept(sockfd, (struct sockaddr*)&addr, &len);

	if(res > 0) {
		client_context_t *cli = malloc(sizeof(client_context_t));
		if(cli == NULL) return NULL;
		cli->data = malloc(DEFAULT_BUFFER_SIZE);
		if(cli->data == NULL) {
		   const int en = errno;
         syslog(LOG_ERR, "Cannot allocate memory for buffer (errno %d), exiting", en);
         printf("Cannot allocate memory for buffer (errno %d), exiting" EOLN, en);
			close(res);
			return NULL;
		}
		cli->fd = res;
		cli->addr = addr.sin_addr.s_addr;

		sprintf(print_buff, "Accepted connection from %d.%d.%d.%d",
				((cli->addr >> 0 ) & 0xFF),
				((cli->addr >> 8 ) & 0xFF),
				((cli->addr >> 16) & 0xFF),
				((cli->addr >> 24) & 0xFF));
		if(!daemonized) printf("%s" EOLN, print_buff);
		syslog(LOG_INFO, "%s", print_buff);
		return cli;
	}

   const int en = errno;
   // if terminated by signal
   if(en != EINTR) {
      // should never get here, I suppose
      syslog(LOG_ERR, "Cannot accept connection (errno %d), exiting", en);
      printf("Cannot accept connection (errno %d), exiting" EOLN, en);
   }

	return NULL;
}

#endif /* SERVER_MY_SOCKETS_H_ */
