/*
 * main.cpp
 *
 *  Created on: 2017年4月3日
 *      Author: lyndon
 */
#include "uart_daemon.h"
bool g_boIsExit = false;


void ProcessStop(int32_t s32Signal)
{
    PRINT("Signal is %d\n", s32Signal);
    g_boIsExit = true;
}
void SignalRegister(void)
{
	signal(SIGINT, ProcessStop);
	signal(SIGTERM, ProcessStop);
}

void *ThreadRead(void *pArg)
{
	int32_t s32MsgId = GetTheMsgId(MSG_KEY_NAME);
	if (s32MsgId < 0)
	{
		return NULL;
	}
	while (!g_boIsExit)
	{
		StMsgStruct stMsg = {0};
		int32_t s32Err = 0;
		s32Err = msgrcv(s32MsgId, &stMsg, sizeof(StMsgStruct), 0, 0);
		if (s32Err < 0)
		{
			if (errno == EIDRM)
			{
				PRINT("msg(%s) is removed\n", MSG_KEY_NAME);
				break;
			}

			continue;
		}

		PRINT("get a msg, %08x-%08x\n", stMsg.u32WParma, stMsg.u32LParma);

	}

	return 0;
}
#define YAN_ANALYSIS_LEN		2048
uint8_t *pBufProtocol[YAN_ANALYSIS_LEN];

typedef struct _tagStThreadArg
{
	int32_t s32FDUart;
	int32_t s32MsgId;
}StThreadArg;

void *ThreadUartRead(void *pArg)
{
	int32_t s32FDUart = ((StThreadArg *)pArg)->s32FDUart;
	int32_t s32MsgId = ((StThreadArg *)pArg)->s32MsgId;
	uint8_t u8ReadBuf[64];
	uint64_t u64TimeMsgRcv = TimeGetTime();
	StCycleBuf stAnalysis = {NULL, 0};
	if (s32FDUart < 0)
	{
		return NULL;
	}

	CycleMsgInit(&stAnalysis, pBufProtocol, YAN_ANALYSIS_LEN);

	while (!g_boIsExit)
	{
		int32_t s32ReadCnt;
		s32ReadCnt = read(s32FDUart, u8ReadBuf, 64);
		if (s32ReadCnt <= 0)
		{
			uint64_t u64TimeCur = TimeGetTime();
			PRINT("current time: %lld, %lld\n", u64TimeCur, u64TimeMsgRcv);
			if (u64TimeCur - u64TimeMsgRcv > 5 * 1000)
			{
				StMsgStruct stMsg = {0};
				stMsg.u32Type = _MSG_UART_Out;
				stMsg.u32LParma = 8;
				stMsg.pMsg = malloc(8);
				if (stMsg.pMsg != NULL)
				{
					uint8_t u8Buf[8] = { 0xAA, 0x00, 0x0C, 0x80, 0x00, 0x00, 0x02, 0x24};
					memcpy(stMsg.pMsg, u8Buf, 8);
					if (msgsnd(s32MsgId, &stMsg, sizeof(StMsgStruct) - offsetof(StMsgStruct, u32WParma), IPC_NOWAIT) < 0)
					{
						free(stMsg.pMsg);
					}
				}
			}
			continue;
		}

		PRINT("get some data: %d\n", s32ReadCnt);

		void *pMsg = NULL;
		do
		{
			int32_t s32Err;
			uint32_t u32GetCmdLen;
			int32_t s32ProtocolType;
			pMsg = CycleGetOneMsg(&stAnalysis, (const char *)u8ReadBuf, s32ReadCnt,
				&u32GetCmdLen, &s32ProtocolType, &s32Err);
			s32ReadCnt = 0;	/* flush all the valid messge */
			if (pMsg != NULL)
			{
				u64TimeMsgRcv = TimeGetTime();
#if 0
				StMsgStruct stMsg = {0};
				stMsg.pMsg = pMsg;
				stMsg.u32WParma = s32ProtocolType;
				stMsg.u32LParma = u32GetCmdLen;
				stMsg.u32Type = _MSG_UART_IN;
#endif
				uint32_t i;
				uint8_t *pCmd = (uint8_t *)pMsg;
				for (i = 0; i < u32GetCmdLen; i++)
				{
					printf("0x%02x ", pCmd[i]);
				}
				printf("\nget data: %d\n", u32GetCmdLen);
				free(pMsg);
			}
			else
			{
				break;
			}
		} while (!g_boIsExit);
	}

	return NULL;

}

void *ThreadUartWrite(void *pArg)
{
	int32_t s32FDUart = ((StThreadArg *)pArg)->s32FDUart;
	int32_t s32MsgId = ((StThreadArg *)pArg)->s32MsgId;

	if (s32MsgId < 0)
	{
		return NULL;
	}

	if (s32FDUart < 0)
	{
		return NULL;
	}
	while (!g_boIsExit)
	{
		StMsgStruct stMsg = {0};
		int32_t s32Err = 0;
		s32Err = msgrcv(s32MsgId, &stMsg, sizeof(StMsgStruct), _MSG_UART_Out, 0);
		if (s32Err < 0)
		{
			if (errno == EIDRM)
			{
				PRINT("msg(%s) is removed\n", MSG_KEY_NAME);
				break;
			}

			continue;
		}
		PRINT("get a msg, %08x-%08x-%p\n", stMsg.u32WParma, stMsg.u32LParma, stMsg.pMsg);

		if ((stMsg.u32LParma != 0) && (stMsg.pMsg != NULL))
		{
			write(s32FDUart, stMsg.pMsg, stMsg.u32LParma);
			tcflush(s32FDUart, TCIOFLUSH);
			free (stMsg.pMsg);
		}
	}

	return NULL;
}


int32_t Test()
{
	pthread_t u32ThreadRecvId;
	int32_t s32Err = 0;
	int32_t s32MsgId = -1;
	SignalRegister();

	s32MsgId = GetTheMsgId(MSG_KEY_NAME);
	if (s32MsgId < 0)
	{
		return -1;
	}
	s32Err = MakeThread(ThreadRead, NULL, false, &u32ThreadRecvId, false);
	if (s32Err < 0)
	{
		ReleaseAMsgId(s32MsgId);
		return -1;
	}

	uint32_t u32Cnt = 0;
	while (!g_boIsExit)
	{
		StMsgStruct stMsg = {1, u32Cnt++};
		msgsnd(s32MsgId, &stMsg, sizeof(StMsgStruct) - offsetof(StMsgStruct, u32WParma), IPC_NOWAIT);
		sleep(1);
	}

	ReleaseAMsgId(s32MsgId);

	pthread_join(u32ThreadRecvId, NULL);


	return 0;
}

void *ThreadUnixMsg(void *pArg)
{
	int32_t s32MsgId = ((StThreadArg *)pArg)->s32MsgId;

	int32_t s32Server = ServerListen(UNIX_SOCKET_NAME);

	if (s32Server < 0)
	{
		PRINT("ServerListen: %s, error: %08x\n", UNIX_SOCKET_NAME, s32Server);
		return NULL;
	}

	while (!g_boIsExit)
	{
		fd_set stSet;
		struct timeval stTimeout;

		int32_t s32Client = -1;
		bool boNeedRelease = true;
		stTimeout.tv_sec = 2;
		stTimeout.tv_usec = 0;
		FD_ZERO(&stSet);
		FD_SET(s32Server, &stSet);

		if (select(s32Server + 1, &stSet, NULL, NULL, &stTimeout) <= 0)
		{
			continue;
		}
		if (!FD_ISSET(s32Server, &stSet))
		{
			continue;
		}
		s32Client = ServerAccept(s32Server);
		if (s32Client < 0)
		{
			PRINT("ServerAccept error: 0x%08x\n", s32Client);
			break;
		}
		else
		{
			uint32_t u32Size = 0;
			int32_t s32Err = 0;
			void *pMCSStream;
			pMCSStream = MCSSyncReceive(s32Client, false, 1000, &u32Size, &s32Err);
			if (pMCSStream != NULL)
			{

				const char *pMCS = (const char *)pMCSStream;
				const char *pMCSEnd = pMCS + u32Size;
				while (pMCS < pMCSEnd)
				{
					uint32_t u32Cmd, u32Count, u32ElementSize;
					uint32_t u32DataLength;
			        LittleAndBigEndianTransfer((char *)&u32Cmd, pMCS, sizeof(uint32_t));   /* 命令号 */
			        pMCS += sizeof(uint32_t);

			        LittleAndBigEndianTransfer((char *)&u32Count, pMCS, sizeof(uint32_t));   /* 此命令号命令的数量 */
			        pMCS += sizeof(uint32_t);

			        LittleAndBigEndianTransfer((char *)&u32ElementSize, pMCS, sizeof(uint32_t));   /* 每一个命令的大小 */
			        pMCS += sizeof(uint32_t);

			        u32DataLength = u32Count * u32ElementSize;
			        if (u32DataLength != 0)
			        {
						switch (u32Cmd)
						{
							case _Unix_Cmd_Uart_Send_Data:
							{
								void *pData = malloc(u32DataLength);
								if (pData != NULL)
								{
									StMsgStruct stMsg;
									memcpy(pData, pMCS, u32DataLength);
									stMsg.pMsg = pData;
									stMsg.u32LParma = u32DataLength;
									stMsg.u32Type = _MSG_UART_Out;
									if (msgsnd(s32MsgId, &stMsg, sizeof(StMsgStruct) -
											offsetof(StMsgStruct, u32WParma), IPC_NOWAIT) < 0)
									{
										free(stMsg.pMsg);
									}
								}
								break;
							}
							default:
								break;
						}
			        }

			        pMCS += u32DataLength;

				}


				MCSSyncFree(pMCSStream);
			}
			else
			{
				PRINT("MCSSyncReceive error 0x%08x\n", s32Err);
			}
			if (boNeedRelease)
			{
				close(s32Client);
			}
		}
	}

	close(s32Server);

	return NULL;

}


void *ThreadEchoCntlFlush(void *pArg)
{
	CEchoCntl *pCntl = (CEchoCntl *)pArg;
	uint8_t u8Msg[8];
	int32_t i = 0;
	while (!g_boIsExit)
	{
		pCntl->Flush(((i++) * 2), u8Msg, 8);
		usleep(10 * 1000);
	}
	return NULL;
}

void EchoCntlTest(void)
{
	pthread_t s32TreadTest = -1;
	uint8_t u8Msg[8];
	CEchoCntl csCntl;
	csCntl.Init(-1);

	SignalRegister();

	MakeThread(ThreadEchoCntlFlush, &csCntl, false, &s32TreadTest, false);

	int32_t i = 0;
	for (;i < 10; i++)
	{
		CEchoInfo *pInfo = new CEchoInfo;
		if (pInfo != NULL)
		{
			pInfo->Init(u8Msg, 8, -1, i);
			csCntl.InsertAElement(pInfo);
		}
	}

	while (!g_boIsExit)
	{
		CEchoInfo *pInfo = new CEchoInfo;
		if (pInfo != NULL)
		{
			pInfo->Init(u8Msg, 8, -1, i++);
			csCntl.InsertAElement(pInfo);
		}
		usleep(1000 * 1000);
	}


	pthread_join(s32TreadTest, NULL);

	return;
}

#if 1
int main(int argc, const char *argv[])
{
	EchoCntlTest();
	return 0;
}
#else
int main(int argc, const char *argv[])
{
	int32_t s32Err = 0;
	pthread_t s32TreadUartWrite = -1;
	int32_t s32FDUart = open("/dev/ttyUSB0", O_RDWR);
	int32_t s32MsgId = -1;
	StThreadArg stArg;
	if (s32FDUart < 0)
	{
		PRINT("error is: %s\n", strerror(errno));
		return 0;
	}
	s32MsgId = GetTheMsgId(MSG_KEY_NAME);
	if (s32MsgId < 0)
	{
		close(s32FDUart);
		return -1;
	}

	SignalRegister();

	if ((s32Err = UARTInit(s32FDUart, B115200, 0, 8, 1, 0, 10)) < 0)
	{
		PRINT("UARTInit error: 0x%08x\n", s32Err);

		ReleaseAMsgId(s32MsgId);
		close(s32FDUart);
		return -1;
	}

	stArg.s32FDUart = s32FDUart;
	stArg.s32MsgId = s32MsgId;

	s32Err = MakeThread(ThreadUartWrite, &stArg, false, &s32TreadUartWrite, false);
	if (s32Err < 0)
	{
		ReleaseAMsgId(s32MsgId);
		close(s32FDUart);
		return -1;
	}


	ThreadUartRead(&stArg);

	g_boIsExit = true;
	ReleaseAMsgId(s32MsgId);

	pthread_join(s32TreadUartWrite, NULL);

	close(s32FDUart);
	return 0;
}
#endif
