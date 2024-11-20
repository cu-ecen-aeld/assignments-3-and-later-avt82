/*
 * my_timer.h
 *
 *  Created on: Nov 20, 2024
 *      Author: user
 */

#ifndef SERVER_MY_TIMER_H_
#define SERVER_MY_TIMER_H_

static char timebuff[64];

static void timestamp_log(datafile_context_t *const ctx ) {
   time_t t;
   time(&t);
   struct tm *timeinfo = localtime(&t);
   strftime(timebuff, sizeof(timebuff), "timestamp:%Y/%m/%d %H:%M:%S\n", timeinfo);
   datafile_write_safe(ctx, timebuff, strlen(timebuff));
}

static void timestamp_handler(union sigval sv) {
   datafile_context_t *const pCtx = (datafile_context_t *)sv.sival_ptr;
   timestamp_log(pCtx);
}

timer_t my_create_timer(void (*handler) (__sigval_t), datafile_context_t *const arg, unsigned int period)
{
   timer_t timer;

   struct sigevent sev;
   sev.sigev_notify = SIGEV_THREAD;
   sev.sigev_value.sival_ptr = (void*)arg;
   sev.sigev_notify_function = handler;
   sev.sigev_notify_attributes = NULL;

   timer_create(CLOCK_MONOTONIC, &sev, &timer);

   struct itimerspec its;
   its.it_value.tv_sec = period;
   its.it_value.tv_nsec = 0;
   its.it_interval.tv_sec = period;
   its.it_interval.tv_nsec = 0;

   timer_settime(timer, 0, &its, NULL);

   return timer;
}

#endif /* SERVER_MY_TIMER_H_ */
