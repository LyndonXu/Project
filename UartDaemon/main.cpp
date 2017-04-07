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

#define YAN_ANALYSIS_LEN		2048
static uint8_t s_u8BufProtocol[YAN_ANALYSIS_LEN];

typedef struct _tagStThreadArg
{
	int32_t s32FDUart;
	int32_t s32MsgId;
	CEchoCntl *pEchoCntl;
}StThreadArg;

void *ThreadUartRead(void *pArg)
{
	int32_t s32FDUart = ((StThreadArg *)pArg)->s32FDUart;
	int32_t s32MsgId = ((StThreadArg *)pArg)->s32MsgId;
	CEchoCntl *pEchoCntl = ((StThreadArg *)pArg)->pEchoCntl;
	uint8_t u8ReadBuf[64];
	uint64_t u64TimeMsgRcv = TimeGetTime();
	StCycleBuf stAnalysis = {NULL, 0};
	if ((s32FDUart < 0) || (pEchoCntl == NULL))
	{
		return NULL;
	}

	CycleMsgInit(&stAnalysis, s_u8BufProtocol, YAN_ANALYSIS_LEN);

	while (!g_boIsExit)
	{
		int32_t s32ReadCnt;
		s32ReadCnt = read(s32FDUart, u8ReadBuf, 64);
		if (s32ReadCnt <= 0)
		{
			uint64_t u64TimeCur = TimeGetTime();
			//PRINT("current time: %lld, %lld\n", u64TimeCur, u64TimeMsgRcv);
			if (u64TimeCur - u64TimeMsgRcv > 5 * 1000)
			{
				StMsgStruct stMsg = {0};
				stMsg.u32Type = _MSG_UART_Out;
				stMsg.u32LParam = 8;
				stMsg.pMsg = malloc(8);
				if (stMsg.pMsg != NULL)
				{
					uint8_t u8Buf[8] = { 0xAA, 0x00, 0x0C, 0x80, 0x00, 0x00, 0x02, 0x24};
					memcpy(stMsg.pMsg, u8Buf, 8);
					if (msgsnd(s32MsgId, &stMsg, sizeof(StMsgStruct) - offsetof(StMsgStruct, u32WParam), IPC_NOWAIT) < 0)
					{
						free(stMsg.pMsg);
					}
				}
				u64TimeMsgRcv += 1000;
			}
			pEchoCntl->Flush(0, NULL, 0);
			continue;
		}

		PRINT("get some data: %d\n", s32ReadCnt);

		uint8_t *pMsg = NULL;
		do
		{
			int32_t s32Err;
			uint32_t u32GetCmdLen;
			int32_t s32ProtocolType;
			pMsg = (uint8_t *)CycleGetOneMsg(&stAnalysis, (const char *)u8ReadBuf, s32ReadCnt,
				&u32GetCmdLen, &s32ProtocolType, &s32Err);
			s32ReadCnt = 0;	/* flush all the valid messge */
			if (pMsg != NULL)
			{
				u64TimeMsgRcv = TimeGetTime();
#if 0
				uint32_t i;
				uint8_t *pCmd = (uint8_t *)pMsg;
				for (i = 0; i < u32GetCmdLen; i++)
				{
					printf("0x%02x ", pCmd[i]);
				}
				printf("\nget data: %d\n", u32GetCmdLen);
#endif
				PRINT("CycleGetOneMsg get some message: %d, type%d\n", u32GetCmdLen, s32ProtocolType);
				if (s32ProtocolType == _Protocol_YNA)
				{
					if (pMsg[_YNA_Mix] == 0x04 && pMsg[_YNA_Cmd] == 0x00)
					{
						uint32_t u32TotalLength = 0;
						uint32_t u32ReadLength = 0;
						uint8_t *pVariableCmd;

						/* get the total command length */
						LittleAndBigEndianTransfer((char *)(&u32TotalLength), (const char *)pMsg + _YNA_Data2, 2);
						pVariableCmd = pMsg + PROTOCOL_YNA_DECODE_LENGTH;
						u32TotalLength -= 2; /* CRC16 */
						while (u32ReadLength < u32TotalLength)
						{
							uint16_t u16Command = 0, u16Count = 0, u16Length = 0;
							uint16_t u16CmdLen;
							LittleAndBigEndianTransfer((char *)(&u16Command),
								(char *)pVariableCmd, 2);
							LittleAndBigEndianTransfer((char *)(&u16Count),
								(char *)pVariableCmd + 2, 2);
							LittleAndBigEndianTransfer((char *)(&u16Length),
								(char *)pVariableCmd + 4, 2);

							uint8_t *pData = pVariableCmd + 6;
							u16CmdLen = u16Count * u16Length;
							switch (u16Command)
							{
								case 0x8020:
								{
									StYNAAuthForOther *pAuth = (StYNAAuthForOther *)pData;
									if (u16CmdLen != sizeof(StYNAAuthForOther))
									{
										break;
									}
									pEchoCntl->Flush(pAuth->s32Serial, pAuth->u8AuthData, 8);

									break;
								}
								default:
									break;
							}
							u32ReadLength += (6 + (uint32_t)u16CmdLen);
							pVariableCmd = pMsg + PROTOCOL_YNA_DECODE_LENGTH + u32ReadLength;
						}
					}
				}

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
		PRINT("get a msg, %08x-%08x-%p\n", stMsg.u32WParam, stMsg.u32LParam, stMsg.pMsg);
		if ((stMsg.u32LParam != 0) && (stMsg.pMsg != NULL))
		{
			write(s32FDUart, stMsg.pMsg, stMsg.u32LParam);
			tcflush(s32FDUart, TCIOFLUSH);
			free (stMsg.pMsg);
		}
	}

	return NULL;
}


void *ThreadUnixMsg(void *pArg)
{
	int32_t s32MsgId = ((StThreadArg *)pArg)->s32MsgId;
	CEchoCntl *pEchoCntl = ((StThreadArg *)pArg)->pEchoCntl;

	if (s32MsgId < 0 || pEchoCntl == NULL)
	{
		return NULL;
	}
	int32_t s32Server = ServerListen(UNIX_SOCKET_NAME);
	int32_t s32Serial;

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
								PRINT("get some data from unix: %d\n", u32DataLength);
								void *pData = malloc(u32DataLength);
								if (pData != NULL)
								{
									StMsgStruct stMsg;
									memcpy(pData, pMCS, u32DataLength);
									stMsg.pMsg = pData;
									stMsg.u32LParam = u32DataLength;
									stMsg.u32Type = _MSG_UART_Out;
									if (msgsnd(s32MsgId, &stMsg, sizeof(StMsgStruct) -
											offsetof(StMsgStruct, u32WParam), IPC_NOWAIT) < 0)
									{
										free(stMsg.pMsg);
									}
								}
								break;
							}
							case _Unix_Cmd_Uart_Send_Auth:
							{
								PRINT("get authentic from unix: %d\n", u32DataLength);
								if (u32DataLength != 8)
								{
									break;
								}
								StYNAAuthForOther stAuth;
								stAuth.s32Serial = s32Serial++;
								memcpy(stAuth.u8AuthData, pMCS, 8);
								StMsgStruct stMsg = {0};
								stMsg.pMsg = YNAMakeASimpleVarialbleCmd(0x0820, &stAuth,
										sizeof(StYNAAuthForOther), &stMsg.u32LParam);

								if (stMsg.pMsg != NULL)
								{

									CEchoInfo *pInfo = new CEchoInfo;
									if (pInfo != NULL)
									{
										if (pInfo->Init((uint8_t *)stMsg.pMsg, stMsg.u32LParam,
												s32Client, s32Serial) < 0)
										{
											delete pInfo;
										}
										else if (pEchoCntl->InsertAElement(pInfo) < 0)
										{
											delete pInfo;
										}
										else
										{
											boNeedRelease = false;
										}
									}
									free (stMsg.pMsg);
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

#if 1
int main(int argc, const char *argv[])
{
	int32_t s32Err = 0;
	pthread_t s32ThreadUartWrite = -1;
	pthread_t s32ThreadUnixMsg = -1;
	int32_t s32FDUart = open("/dev/ttyUSB0", O_RDWR);
	int32_t s32MsgId = -1;
	StThreadArg stArg;
	CEchoCntl csEchoCntl;
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

	if ((s32Err = UARTInit(s32FDUart, B115200, 0, 8, 1, 0, 2)) < 0)
	{
		PRINT("UARTInit error: 0x%08x\n", s32Err);

		ReleaseAMsgId(s32MsgId);
		goto end;
	}

	if ((s32Err = csEchoCntl.Init(s32MsgId)) < 0)
	{
		PRINT("csEchoCntl init error: 0x%08x\n", s32Err);
		goto end;
	}

#if 1
	{
		StYNAAuthForOther stAuth;
		stAuth.s32Serial = 555;
		uint32_t u32CmdLength = 0;

		uint8_t *pMsg = (uint8_t *)YNAMakeASimpleVarialbleCmd(0x8020, &stAuth,
				sizeof(StYNAAuthForOther), &u32CmdLength);
		CEchoInfo *pInfo = new CEchoInfo;
		if (pInfo != NULL)
		{
			if (pInfo->Init(pMsg, u32CmdLength, -1, 555) == 0)
			{
				csEchoCntl.InsertAElement(pInfo);
			}
			else
			{
				delete pInfo;
			}
		}
		if (pMsg != NULL)
		{
			free(pMsg);
		}
	}
#endif

	stArg.s32FDUart = s32FDUart;
	stArg.s32MsgId = s32MsgId;
	stArg.pEchoCntl = &csEchoCntl;

	s32Err = MakeThread(ThreadUartWrite, &stArg, false, &s32ThreadUartWrite, false);
	if (s32Err < 0)
	{
		goto end;
		return -1;
	}
	s32Err = MakeThread(ThreadUnixMsg, &stArg, false, &s32ThreadUnixMsg, false);
	if (s32Err < 0)
	{
		goto end;
		return -1;
	}


	ThreadUartRead(&stArg);

end:
	g_boIsExit = true;
	ReleaseAMsgId(s32MsgId);

	if (s32ThreadUartWrite >= 0)
	{
		pthread_join(s32ThreadUartWrite, NULL);
	}
	if (s32ThreadUnixMsg >= 0)
	{
		pthread_join(s32ThreadUnixMsg, NULL);
	}

	close(s32FDUart);
	return 0;
}
#endif
