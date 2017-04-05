/*
 * echocntl.cpp
 *
 *  Created on: 2017年4月3日
 *      Author: lyndon
 */

#include "uart_daemon.h"

CEchoInfo::~CEchoInfo()
{
	if (m_pMsg != NULL)
	{
		free (m_pMsg);
	}
	if (m_pEcho != NULL)
	{
		free (m_pEcho);
	}
	if (m_s32Socket >= 0)
	{
		close(m_s32Socket);
	}
};

int32_t CEchoInfo::Init(uint8_t *pMsg, int32_t s32MsgLen, int32_t s32Socket, int32_t s32Serial)
{
	if (pMsg == NULL || s32MsgLen <= 0/* || s32Socket <= 0*/)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	m_pMsg = (uint8_t *)malloc(s32MsgLen);
	if (m_pMsg == NULL)
	{
		return MY_ERR(_Err_Mem);
	}
	memcpy(m_pMsg, pMsg, s32MsgLen);

	m_s32Serial = s32Serial;
	m_u64SendTime = 0;//TimeGetTime();
	m_s32MsgLen = s32MsgLen;
	m_s32Socket = s32Socket;
	return 0;
}


CEchoCntl::CEchoCntl()
	: m_boHasInit(false)
	, m_s32MsgId(-1)
{
	memset(&m_stMutex, 0, sizeof(pthread_mutex_t));
	m_csList.clear();
};

int32_t CEchoCntl::Init(int32_t s32MsgId)
{
	if (m_boHasInit)
	{
		return 0;
	}
	if (pthread_mutex_init(&m_stMutex, NULL) != 0)
	{
		return MY_ERR(_Err_SYS + errno);
	}
	m_s32MsgId = s32MsgId;
	m_boHasInit = true;
	return 0;
}
void CEchoCntl::Destory(void)
{
	if (!m_boHasInit)
	{
		return;
	}

	pthread_mutex_lock(&m_stMutex);

	for (CListEchoInfoIter iter = m_csList.begin(); iter != m_csList.end(); iter++)
	{
		CEchoInfo *pInfo = *(iter);
		if (pInfo != NULL)
		{
			PRINT("delete the info: %d\n",pInfo->m_s32Serial);
			delete pInfo;
		}
	}

	pthread_mutex_unlock(&m_stMutex);

	pthread_mutex_destroy(&m_stMutex);
}

int32_t CEchoCntl::InsertAElement(CEchoInfo *pInfo)
{
	if (pInfo == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}

	pthread_mutex_lock(&m_stMutex);

	PRINT("insert the info: %d\n", pInfo->m_s32Serial);
	m_csList.push_back(pInfo);

	pthread_mutex_unlock(&m_stMutex);
	return 0;
}

int32_t CEchoCntl::Flush(int32_t s32Serial, uint8_t *pMsg, int32_t s32MsgLen)
{
	if (!m_boHasInit)
	{
		return MY_ERR(_Err_Handle);
	}

	uint64_t u64CurTime = TimeGetTime();
	pthread_mutex_lock(&m_stMutex);
	CListEchoInfoIter iter = m_csList.begin();

	while (iter != m_csList.end())
	{
		CListEchoInfoIter iterNext = iter;
		iterNext++;

		CEchoInfo *pInfo = *(iter);
		if (pInfo != NULL)
		{
			if (pInfo->m_s32SendCnt >= 2)
			{
				PRINT("Send count >= 2 delete the Info: %d\n", pInfo->m_s32Serial);
				delete pInfo;
				m_csList.erase(iter);
			}
			else if ((pMsg != NULL) && (s32MsgLen > 0) &&
					(s32Serial == pInfo->m_s32Serial))
			{
				PRINT("get echo transform and delete the Info: %d\n", pInfo->m_s32Serial);
				MCSSyncSendData(pInfo->m_s32Socket, 1000, s32MsgLen, pMsg);
				delete pInfo;
				m_csList.erase(iter);
			}
			else if ((u64CurTime - pInfo->m_u64SendTime) > 500)
			{
				PRINT("retry send(%d) the info: %d\n", pInfo->m_s32SendCnt, pInfo->m_s32Serial);
				StMsgStruct stMsg = {0};
				stMsg.u32Type = _MSG_UART_Out;
				stMsg.u32LParam = pInfo->m_s32MsgLen;
				stMsg.pMsg = malloc(pInfo->m_s32MsgLen);
				if (stMsg.pMsg != NULL)
				{
					memcpy(stMsg.pMsg, pInfo->m_pMsg, pInfo->m_s32MsgLen);
/**/
					if (msgsnd(m_s32MsgId, &stMsg, sizeof(StMsgStruct) -
							offsetof(StMsgStruct, u32WParam), IPC_NOWAIT) < 0)

					{
						free(stMsg.pMsg);
					}
				}
				pInfo->m_u64SendTime = u64CurTime;
				pInfo->m_s32SendCnt++;
			}
		}

		iter = iterNext;
	}
	pthread_mutex_unlock(&m_stMutex);

	return 0;
}




