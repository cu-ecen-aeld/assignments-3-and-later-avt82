/*
 * my_datafile.h
 *
 *  Created on: Nov 18, 2024
 *      Author: user
 */

#ifndef SERVER_MY_DATAFILE_H_
#define SERVER_MY_DATAFILE_H_

static char const *const cDataFile = "/var/tmp/aesdsockedata";

static inline void datafile_begin(datafile_context_t *const ctx) {
   lseek(ctx->fd, 0, SEEK_SET);
}

static inline void datafile_end(datafile_context_t *const ctx) {
   lseek(ctx->fd, 0, SEEK_END);
}

static inline int datafile_init(datafile_context_t *const ctx, char const *const filename) {
   // first the file
   ctx->filename = (char*)filename;
   ctx->fd = open(ctx->filename, O_RDWR | O_CREAT | O_APPEND, 0666);
   if (ctx->fd <= 0) return -1;
   datafile_end(ctx);
   // It is wrong, but I assume it would be always initialized w/o error
   pthread_mutex_init(&ctx->lock, NULL);
   return 0;
}

static inline void datafile_close(datafile_context_t *const ctx) {
   if (ctx->fd > 0) close(ctx->fd);
}

static inline void datafile_kill(datafile_context_t *const ctx) {
   datafile_close(ctx);
   remove(ctx->filename);
}

static int datafile_write_safe(datafile_context_t *const ctx, char const *data, int size) {
   if(size == 0) return 0;
   int written = 0;
   if(pthread_mutex_lock(&ctx->lock) != 0) return -1;
   datafile_end(ctx);
   while(size > 0) {
      int res = write(ctx->fd, data, size);
      const int en = errno;
      if(res < 0) {
         if((en == EAGAIN) || (errno == EWOULDBLOCK)) continue;
         written = 0;
         if(en == EINTR) written = -1;
         break;
      }
      written += res;
      data += res;
      size -= res;   // assuming res is never bigger than size
   }
   pthread_mutex_unlock(&ctx->lock);
   return written;
}

#endif /* SERVER_MY_DATAFILE_H_ */
