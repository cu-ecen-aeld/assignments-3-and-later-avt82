/*
 * sockets.h
 *
 *  Created on: Nov 16, 2024
 *      Author: user
 */

#ifndef SERVER_MY_SOCKETS_H_
#define SERVER_MY_SOCKETS_H_

static inline int my_socket(void) {
	return socket(AF_INET, SOCK_STREAM, 0);
}

static inline int my_bind(int sockfd) {
	struct sockaddr_in addr;
	(void)memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(9000);
	return bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
}

static inline int my_setsockopt(int sockfd) {
	int reuse = (int)true;
    return setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void*)&reuse, sizeof(reuse));
}

static inline int my_listen(int sockfd) {
	return listen(sockfd,20);
}

static inline int my_accept(int sockfd, const bool daemonized) {
	struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
	(void)memset(&addr, 0, sizeof(addr));
	const int res = accept(sockfd, (struct sockaddr*)&addr, &len);
	if(res >= 0) {
		const uint32_t u32addr = addr.sin_addr.s_addr;

		sprintf(print_buff, "Accepted connection from %d.%d.%d.%d",
				((u32addr >> 0 ) & 0xFF),
				((u32addr >> 8 ) & 0xFF),
				((u32addr >> 16) & 0xFF),
				((u32addr >> 24) & 0xFF));
		if(!daemonized) printf("%s" EOLN, print_buff);
		syslog(LOG_INFO, "%s", print_buff);
	}
	return res;
}

#endif /* SERVER_MY_SOCKETS_H_ */
