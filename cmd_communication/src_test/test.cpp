/*
 * test.cpp
 *
 *  Created on: 2017年4月5日
 *      Author: lyndon
 */
#include "cmd_communication.h"

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

char c8Buf[1 * 1024 * 1024];
char c8Buf2[1 * 1024 * 1024];
int main()
{
	int32_t i;
	int32_t s32Socket = -1;
	int32_t s32Len, s32Err = 0;
	struct sockaddr_in stAddr;
	const char *pAddr = "127.0.0.1";


	if ((s32Socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		return MY_ERR(_Err_SYS + errno);
	}
	memset(&stAddr, 0, sizeof(stAddr));
	stAddr.sin_family = AF_INET;
	stAddr.sin_port = htons(TCP_CMD_SERVER_PORT);
	stAddr.sin_addr.s_addr = inet_addr(pAddr);
	s32Len = sizeof(stAddr);

	if (connect(s32Socket, (struct sockaddr*) (&stAddr), s32Len) < 0)
	{
		PRINT("error: %s\n", strerror(errno));
		s32Err = MY_ERR(_Err_SYS + errno);

		close(s32Socket);
		return s32Err;
	}

    /* 设置套接字选项,接收和发送超时时间 */
	{
		struct timeval stTimeout;
		stTimeout.tv_sec  = 1000;
		stTimeout.tv_usec = 0;
		if(setsockopt(s32Socket, SOL_SOCKET, SO_RCVTIMEO, &stTimeout, sizeof(struct timeval)) < 0)
		{
			close(s32Socket);
			return MY_ERR(_Err_SYS + errno);
		}

		if(setsockopt(s32Socket, SOL_SOCKET, SO_SNDTIMEO, &stTimeout, sizeof(struct timeval)) < 0)
		{
			close(s32Socket);
			return MY_ERR(_Err_SYS + errno);
		}
	}


	for (i = 0; i < 50; i++)
	{
		int32_t j;
		int32_t s32Type = rand() % 2;
		uint32_t u32Length = 0;
		for (j = 0; j < 1 * 1024 * 1024; j++)
		{
			c8Buf2[j] = rand();
		}

		if (s32Type == 0)	/* YNA */
		{
			int32_t s32Tmp = rand();
			if ((s32Tmp & 0x01) == 0)
			{
				c8Buf2[0] = 0xAA;
				YNAGetCheckSum((uint8_t *)c8Buf2);
				u32Length = 8;
			}
			else
			{
				void *pCmd = YNAMakeASimpleVarialbleCmd(11, c8Buf2, rand() % 1024, &u32Length);
				if (pCmd != NULL)
				{
					memcpy(c8Buf2, pCmd, u32Length);
					free(pCmd);
				}
				else
				{
					u32Length = 0;
				}
			}
		}
		else
		{
			/* no wrong message */
			void *pCmd = MCSMakeAnArrayVarialbleCmd(11, c8Buf2, 1, rand() % 1024, &u32Length);
			if (pCmd != NULL)
			{
				memcpy(c8Buf2, pCmd, u32Length);
				free(pCmd);
			}
			else
			{
				u32Length = 0;
			}
		}
		if (u32Length != 0)
		{
			PRINT("send: p--%d, l--%d\n", s32Type, u32Length);
			send(s32Socket, c8Buf2, u32Length, MSG_NOSIGNAL);
		}
	}

	PRINT("press enter key to exit\n");

	getchar();

	close(s32Socket);

	return 0;
}

