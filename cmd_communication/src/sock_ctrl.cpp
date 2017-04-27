/*
 * sock_ctrl.cpp
 *
 *  Created on: 2017年4月26日
 *      Author: lyndon
 */

#include "cmd_communication.h"
#include <map>

using namespace std;

typedef struct _tagStSockInfo
{
	uint64_t u64LastRecvTime;
}StSockInfo;

class CSockCtrl
{
private:
	pthread_mutex_t m_stMutex;
	bool m_boHasInit;


	static const int32_t m_s_c_s32SocketBufCnt = 5;
	int32_t m_s32Socket[m_s_c_s32SocketBufCnt];
	map<int32_t, StSockInfo> m_csSockMap;
	void Destory();
public:
	int32_t InsertASocket(int32_t s32Socket);
	int32_t UpdateASocket(int32_t s32Socket, StSockInfo *pInfo);
	int32_t FlushSocket(void);
	int32_t *GetSocket(int32_t &s32Count);

	CSockCtrl();
	~CSockCtrl()
	{
		Destory();
	};
};

CSockCtrl::CSockCtrl()
	: m_boHasInit(false)
{
	memset(&m_stMutex, 0, sizeof(pthread_mutex_t));
	memset(m_s32Socket, -1, sizeof(int32_t) * m_s_c_s32SocketBufCnt);
}

void CSockCtrl::Destory()
{
	if (!m_boHasInit)
	{
		return;
	}

	pthread_mutex_lock(&m_stMutex);

	for (map<int32_t, StSockInfo>::iterator iter = m_csSockMap.begin(); iter != m_csSockMap.end(); iter++)
	{
		int32_t s32Socket = iter->first;
		if (s32Socket >= 0)
		{
			close(s32Socket);
		}
	}

	pthread_mutex_unlock(&m_stMutex);

	pthread_mutex_destroy(&m_stMutex);

}

int32_t CSockCtrl::InsertASocket(int32_t s32Socket)
{
	int32_t s32Err = 0;
	if (s32Socket < 0)
	{
		return COMMON_ERR(_Err_InvalidParam);
	}

	pthread_mutex_lock(&m_stMutex);

	map<int32_t, StSockInfo>::iterator iter = m_csSockMap.find(s32Socket);
	uint64_t u64TimeCur = TimeGetTime();
	if (iter != m_csSockMap.end())
	{
		iter->second.u64LastRecvTime = u64TimeCur;
	}
	else if (m_csSockMap.size() < 5)
	{
		StSockInfo stInfo = {u64TimeCur};
		m_csSockMap.insert(make_pair(s32Socket, stInfo));
	}
	else
	{
		s32Err = CMD_ERR(_Err_Socket_Full);
	}

	pthread_mutex_unlock(&m_stMutex);

	return s32Err;
}


int32_t CSockCtrl::UpdateASocket(int32_t s32Socket, StSockInfo *pInfo)
{
	return InsertASocket(s32Socket);
}



int32_t CSockCtrl::FlushSocket(void)
{
	pthread_mutex_lock(&m_stMutex);

	uint64_t u64TimeCur = TimeGetTime();

	map<int32_t, StSockInfo>::iterator iter = m_csSockMap.begin();
	while (iter != m_csSockMap.end())
	{
		map<int32_t, StSockInfo>::iterator iterNext = iter;
		iterNext++;
		if ((u64TimeCur - iter->second.u64LastRecvTime) > CMD_OUTTIME)
		{
			m_csSockMap.erase(iter);
		}
		iter = iterNext;
	}
	pthread_mutex_unlock(&m_stMutex);

	return m_csSockMap.size();
}

int32_t *CSockCtrl::GetSocket(int32_t &s32Count)
{
	s32Count = 0;
	pthread_mutex_lock(&m_stMutex);

	for (map<int32_t, StSockInfo>::iterator iter = m_csSockMap.begin(); iter != m_csSockMap.end(); iter++)
	{
		m_s32Socket[s32Count++] = iter->first;
	}

	pthread_mutex_unlock(&m_stMutex);
	if (s32Count == 0)
	{
		return NULL;
	}
	return m_s32Socket;
}


