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
#include <poll.h>


#include <map>

using namespace std;

#include "common.h"

#define CMD_ERR(code)	ERROR_CODE(0x17, (code))

extern bool g_boIsExit;

#define CMD_OUTTIME			30	//second

enum
{
	_Err_Socket_Full = 1,
	_Err_Socket_Invalid
};



struct StCmdMsg
{
	void *pData;
	uint32_t u32Length;
	int32_t s32Protocol;
	StCmdMsg()
		: pData(NULL)
		, u32Length(0)
		, s32Protocol(_Protocol_Reserved)
	{

	}
	~StCmdMsg()
	{
		if (pData != NULL)
		{
			free(pData);
			pData = NULL;
		}
	}
};


typedef struct StCmdMsg StCmdMsg;

class CSockInfo
{
public:
	static const int32_t m_s_c_s32MSGBufLen = 4 * 1024;
	uint64_t m_u64LastRecvTime;
	StCycleBuf m_stCycleBuf;
	void *m_pMsgBuf;
	bool boIsValid;
	CSockInfo()
		: m_u64LastRecvTime(0)
		, m_pMsgBuf(NULL)
		, boIsValid(false)
	{
		memset(&m_stCycleBuf, 0, sizeof(StCycleBuf));
		m_pMsgBuf = malloc(m_s_c_s32MSGBufLen);
		if (m_pMsgBuf != NULL)
		{
			CycleMsgInit(&m_stCycleBuf, m_pMsgBuf, m_s_c_s32MSGBufLen);
		}
	}
	~CSockInfo()
	{
		if (m_pMsgBuf != NULL)
		{
#if 0
			PRINT("~CSockInfo: %p(%d, %d, %d, %d)\n", m_pMsgBuf,
					m_stCycleBuf.u32Read, m_stCycleBuf.u32Write, m_stCycleBuf.u32Using,
					m_stCycleBuf.u32TotalLength);
#endif
			free(m_pMsgBuf);
		}
	}
};

class CSockCtrl
{
private:
	pthread_mutex_t m_stMutex;
	bool m_boHasInit;


	static const int32_t m_s_c_s32SocketBufCnt = 5;
	int32_t m_s32Socket[m_s_c_s32SocketBufCnt];
	map<int32_t, CSockInfo *> m_csSockMap;
	void Destory(map<int32_t, CSockInfo *>::iterator &iter);
	void Destory();
public:
	int32_t InsertASocket(int32_t s32Socket);
	int32_t DeleteASocket(int32_t s32Socket);
	int32_t UpdateASocket(int32_t s32Socket);
	int32_t FlushSocket(void);
	int32_t *GetSocket(int32_t &s32Count);
	int32_t ValidateTheSocket(int32_t s32Socket);
	bool IsSocketValid(int32_t s32Socket);
	int32_t GetValidMsg(int32_t s32Socket, void *pBuf, uint32_t u32Length, StCmdMsg &stMsg);

	CSockCtrl();
	~CSockCtrl()
	{
		Destory();
	};
};

typedef struct _tagStThreadArg
{
	CSockCtrl *pCtrl;
}StThreadArg;

enum
{
	_Unix_Cmd_Com_Send_Data = _MCS_Cmd_Cmd_Com,
};
#define UNIX_SOCKET_NAME		WORK_DIR "cmd_com_server.socket"


void *ThreadTCPOutCMD(void *pArg);
void *ThreadTCPOutCMDParse(void *pArg);
void *ThreadUnixCmd(void *pArg);


class CMallocAutoRelease
{
public:
	void *m_pData;
	CMallocAutoRelease()
		:m_pData(NULL)
	{};
	CMallocAutoRelease(void *pData)
	{
		m_pData = pData;
	};

	~CMallocAutoRelease()
	{
		if (m_pData != NULL)
		{
			free(m_pData);
		}
	}
};

#endif /* CMD_COMMUNICATION_H_ */
