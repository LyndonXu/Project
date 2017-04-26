/*
 * cmd_communication.h
 *
 *  Created on: 2017年4月26日
 *      Author: ubuntu
 */

#ifndef CMD_COMMUNICATION_H_
#define CMD_COMMUNICATION_H_


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

#include "common.h"

#define CMD_ERR(code)	ERROR_CODE(0x17, (code))

extern bool g_boIsExit;



#endif /* CMD_COMMUNICATION_H_ */
