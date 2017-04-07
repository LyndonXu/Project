/*
 * uart_daemon.h
 *
 *  Created on: 2017年4月5日
 *      Author: ubuntu
 */

#ifndef _UART_DAEMON_H_
#define _UART_DAEMON_H_


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

#include <termios.h>

#include "common.h"

#include <list>

using namespace std;


#define MSG_KEY_NAME			"uart_daemon.msg"
#define UNIX_SOCKET_NAME		WORK_DIR "uart_daemon_server.socket"

enum
{
	_MSG_UART_In = 1,
	_MSG_UART_Out,
};

enum
{
	_Unix_Cmd_Uart_Send_Data = _MCS_Cmd_UartDaemon,
	_Unix_Cmd_Uart_Send_Auth,


};


typedef struct _tagStMsgStruct
{
	uint32_t u32Type;			/* type */
	uint32_t u32WParam;			/* main parameter */
	uint32_t u32LParam;			/* minor parameter */
	void *pMsg;					/* message */
}StMsgStruct;


class CEchoInfo
{
public:
	int32_t m_s32Serial;		/*  */
	uint8_t *m_pMsg;
	int32_t m_s32MsgLen;
	uint8_t *m_pEcho;
	int32_t m_s32EchoLen;

	int32_t m_s32Socket;
	int32_t m_s32SendCnt;
	uint64_t m_u64SendTime;

	CEchoInfo()
		: m_s32Serial(0)
		, m_pMsg(NULL)
		, m_s32MsgLen(0)
		, m_pEcho(NULL)
		, m_s32EchoLen(0)
		, m_s32Socket(-1)
		, m_s32SendCnt(0)
		, m_u64SendTime(0)
	{

	};
	~CEchoInfo();

	int32_t Init(uint8_t *pMsg, int32_t s32MsgLen, int32_t s32Socket, int32_t s32Serial);
};

typedef list<CEchoInfo *> CListEchoInfo;
typedef list<CEchoInfo *>::iterator CListEchoInfoIter;

class CEchoCntl
{
	pthread_mutex_t m_stMutex;
	bool m_boHasInit;
	int32_t m_s32MsgId;
	CListEchoInfo m_csList;

	int32_t ElementMsgSend(CEchoInfo *pInfo);
public:
	CEchoCntl();
	~CEchoCntl()
	{
		Destory();
	};

	int32_t Init(int32_t s32MsgId);
	int32_t InsertAElement(CEchoInfo *pInfo);
	int32_t Flush(int32_t s32Serial, uint8_t *pMsg, int32_t s32MsgLen);
	void Destory(void);
};

#endif /* UARTDAEMON_UART_DAEMON_H_ */
