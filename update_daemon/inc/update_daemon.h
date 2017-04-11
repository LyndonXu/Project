/*
 * update_daemon.h
 *
 *  Created on: 2017年4月5日
 *      Author: ubuntu
 */

#ifndef _UPDATE_DAEMON_H_
#define _UPDATE_DAEMON_H_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include <termios.h>

#include "common.h"

#include <list>

using namespace std;


#if HAS_CREOSS
#define INTERFACE_NAME	"eth0"
#else
#define INTERFACE_NAME	"ens33"
#endif

#define UDP_SERVER_PORT	(('Y'<< 8) | 'A')


#endif /* UARTDAEMON_UART_DAEMON_H_ */
