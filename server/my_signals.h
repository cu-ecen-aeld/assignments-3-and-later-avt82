/*
 * my_signals.h
 *
 *  Created on: Nov 16, 2024
 *      Author: user
 */

#ifndef SERVER_MY_SIGNALS_H_
#define SERVER_MY_SIGNALS_H_

/***************************************************************************
 *
 * This header MUST be included from ONLY-ONE-EVER source file
 *
 ***************************************************************************/

static volatile bool bSignalTerminate = false;
static volatile bool bSignalInterrupt = false;

static void signal_handler(int sig) {
    if (sig == SIGINT)  bSignalInterrupt = true;
    if (sig == SIGTERM) bSignalTerminate = true;
}

static void init_signals(void) {
	struct sigaction signals;
	bzero(&signals, sizeof(signals));
	signals.sa_handler = &signal_handler;
	sigaction(SIGTERM, &signals, NULL);
	sigaction(SIGINT,  &signals, NULL);
}

#endif /* SERVER_MY_SIGNALS_H_ */
