/*
 * sock_ctrl.cpp
 *
 *  Created on: 2017年4月26日
 *      Author: lyndon
 */

#include "cmd_communication.h"


CSockCtrl::CSockCtrl()
	: m_boHasInit(false)
{
	memset(&m_stMutex, 0, sizeof(pthread_mutex_t));
	memset(m_s32Socket, -1, sizeof(int32_t) * m_s_c_s32SocketBufCnt);
}
void CSockCtrl::Destory(map<int32_t, CSockInfo *>::iterator &iter)
{
	int32_t s32Socket = iter->first;
	if (s32Socket >= 0)
	{
		close(s32Socket);
	}
	if (iter->second != NULL)
	{
		delete iter->second;
	}

}

void CSockCtrl::Destory()
{
	if (!m_boHasInit)
	{
		return;
	}

	pthread_mutex_lock(&m_stMutex);

	for (map<int32_t, CSockInfo *>::iterator iter = m_csSockMap.begin(); iter != m_csSockMap.end(); iter++)
	{
		Destory(iter);
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

	map<int32_t, CSockInfo *>::iterator iter = m_csSockMap.find(s32Socket);
	uint64_t u64TimeCur = TimeGetTime();
	if (iter != m_csSockMap.end())
	{
		iter->second->m_u64LastRecvTime = u64TimeCur;
	}
	else if (m_csSockMap.size() < 5)
	{
		CSockInfo *pInfo = new CSockInfo;
		if (pInfo == NULL)
		{
			return COMMON_ERR(_Err_Mem);
		}
		pInfo->m_u64LastRecvTime = u64TimeCur;
		m_csSockMap.insert(make_pair(s32Socket, pInfo));
	}
	else
	{
		s32Err = CMD_ERR(_Err_Socket_Full);
	}

	pthread_mutex_unlock(&m_stMutex);

	return s32Err;
}

int32_t CSockCtrl::DeleteASocket(int32_t s32Socket)
{
	int32_t s32Err = 0;
	if (s32Socket < 0)
	{
		return COMMON_ERR(_Err_InvalidParam);
	}

	pthread_mutex_lock(&m_stMutex);

	map<int32_t, CSockInfo *>::iterator iter = m_csSockMap.find(s32Socket);
	if (iter != m_csSockMap.end())
	{
		Destory(iter);
		m_csSockMap.erase(iter);
	}
	else
	{
		s32Err = CMD_ERR(_Err_Socket_Invalid);
	}

	pthread_mutex_unlock(&m_stMutex);

	return s32Err;
}


int32_t CSockCtrl::UpdateASocket(int32_t s32Socket)
{
	return InsertASocket(s32Socket);
}



int32_t CSockCtrl::FlushSocket(void)
{
	pthread_mutex_lock(&m_stMutex);

	uint64_t u64TimeCur = TimeGetTime();

	map<int32_t, CSockInfo *>::iterator iter = m_csSockMap.begin();
	while (iter != m_csSockMap.end())
	{
		map<int32_t, CSockInfo*>::iterator iterNext = iter;
		iterNext++;
		if ((u64TimeCur - iter->second->m_u64LastRecvTime) > CMD_OUTTIME)
		{
			Destory(iter);
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
	int32_t *pSock = NULL;
	pthread_mutex_lock(&m_stMutex);

	for (map<int32_t, CSockInfo*>::iterator iter = m_csSockMap.begin(); iter != m_csSockMap.end(); iter++)
	{
		m_s32Socket[s32Count++] = iter->first;
	}
	if (s32Count != 0)
	{
		pSock = (int32_t *)malloc(s32Count * sizeof(int32_t));
		memcpy(pSock, m_s32Socket, s32Count * sizeof(int32_t));
	}

	pthread_mutex_unlock(&m_stMutex);
	return pSock;
}


int32_t CSockCtrl::ValidateTheSocket(int32_t s32Socket)
{
	int32_t s32Err = 0;
	if (s32Socket < 0)
	{
		return COMMON_ERR(_Err_InvalidParam);
	}

	pthread_mutex_lock(&m_stMutex);

	map<int32_t, CSockInfo *>::iterator iter = m_csSockMap.find(s32Socket);
	if (iter != m_csSockMap.end())
	{
		iter->second->boIsValid = true;
	}

	pthread_mutex_unlock(&m_stMutex);

	return s32Err;
}

bool CSockCtrl::IsSocketValid(int32_t s32Socket)
{
	bool boIsValid = false;
	if (s32Socket < 0)
	{
		return boIsValid;
	}

	pthread_mutex_lock(&m_stMutex);

	map<int32_t, CSockInfo *>::iterator iter = m_csSockMap.find(s32Socket);
	if (iter != m_csSockMap.end())
	{
		boIsValid = iter->second->boIsValid;
	}

	pthread_mutex_unlock(&m_stMutex);

	return boIsValid;
}


int32_t CSockCtrl::GetValidMsg(int32_t s32Socket, void *pBuf, uint32_t u32Length, StCmdMsg &stMsg)
{
	int32_t s32Err = 0;
	if (s32Socket < 0)
	{
		return COMMON_ERR(_Err_InvalidParam);
	}

	pthread_mutex_lock(&m_stMutex);

	map<int32_t, CSockInfo *>::iterator iter = m_csSockMap.find(s32Socket);
	if (iter != m_csSockMap.end())
	{
		stMsg.pData = CycleGetOneMsg(&(iter->second->m_stCycleBuf),
				(const char *)pBuf, u32Length,
				&stMsg.u32Length, &stMsg.s32Protocol,
				&s32Err);
	}
	else
	{
		s32Err = CMD_ERR(_Err_Socket_Invalid);
	}

	pthread_mutex_unlock(&m_stMutex);

	return s32Err;
}

