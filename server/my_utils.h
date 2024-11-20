/*
 * my_utils.h
 *
 *  Created on: Nov 16, 2024
 *      Author: user
 */

#ifndef SERVER_MY_UTILS_H_
#define SERVER_MY_UTILS_H_

#define EOLN						"\n"

#define dbg_print(...)				printf(__VA_ARGS__)

/***************************************************************************
 *
 * Do not modify the code below!
 *
 * WARNING! This file must be included from one source file only!
 *
 ***************************************************************************/

typedef struct {
   char           *filename;
   int             fd;
   pthread_mutex_t lock;
} datafile_context_t;

typedef struct {
   int                 fd;
   uint32_t            addr;
   char               *data;
   int                 size;
   datafile_context_t *dfctx;
} client_context_t;

static char print_buff[256];

#if !defined(dbg_print)
#define dbg_print(...)
#endif // dbg_print

#endif /* SERVER_MY_UTILS_H_ */
