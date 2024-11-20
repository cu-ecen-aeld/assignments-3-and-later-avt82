/*
 * my_list.h
 *
 *  Created on: Nov 20, 2024
 *      Author: user
 */

#ifndef SERVER_MY_LIST_H_
#define SERVER_MY_LIST_H_

typedef struct tag_list {
//   struct tag_list  *prev;
   struct tag_list  *next;
   client_context_t *cli;
   pthread_t         thread;
} list_t;

static inline list_t *list_malloc(void) {
   list_t *list = (list_t*)malloc(sizeof(list_t));
   if(list != NULL) bzero(list, sizeof(list_t));
   return list;
}

static inline list_t *list_findAvailableOrLast(list_t *head) {
   // if there's already a thread that has terminated, we will re-use it
   while((head->cli != NULL) && (head->next != NULL)) head = head->next;
   return head;
}

#endif /* SERVER_MY_LIST_H_ */
