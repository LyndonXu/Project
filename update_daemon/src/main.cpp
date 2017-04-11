/*
 * main.cpp
 *
 *  Created on: 2017年4月11日
 *      Author: ubuntu
 */

#include "update_daemon.h"

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


#define UDP_MSG_BUF_LEN	2048
uint8_t *pUDPBuf[UDP_MSG_BUF_LEN];


int32_t UDPMCSResolveCallBack(uint32_t u32CmdNum, uint32_t u32CmdCnt, uint32_t u32CmdSize,
        const char *pCmdData,
        void *pContext)
{
	PRINT("cmd %d, cnt %d, size %d, %p\n", u32CmdNum, u32CmdCnt, u32CmdSize, pCmdData);

	return 0;
}
void *ThreadUDP(void *pArg)
{
	int32_t s32Socket = -1;
	int32_t s32Err = 0;

	s32Socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (s32Socket < 0)
	{
		PRINT("socket error: %s\n", strerror(errno));
		return NULL;
	}



	{
	    struct sockaddr_in stAddr;
	    bzero(&stAddr, sizeof(struct sockaddr_in));
	    stAddr.sin_family = AF_INET;
	    stAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	    stAddr.sin_port = htons(UDP_SERVER_PORT);

	    if(bind(s32Socket,(struct sockaddr *)&(stAddr), sizeof(struct sockaddr_in)) == -1)
	    {
	        return false;
	    }
	}
//	s32Err = RebindToEth(&s32Socket, SOCK_DGRAM, UDP_SERVER_PORT, INTERFACE_NAME);
//	if (s32Err < 0)
//	{
//		PRINT("RebindToEth: %08x\n", s32Err);
//			close(s32Socket);
//		return NULL;
//	}
	{
		int32_t s32Enable = 1;
		s32Err = setsockopt(s32Socket, SOL_SOCKET, SO_BROADCAST, &s32Enable, sizeof(int32_t));
		if (s32Err < 0)
		{
			PRINT("setsockopt: %08x\n", errno);
			close(s32Socket);
			return NULL;
		}
	}


	while (!g_boIsExit)
	{
		fd_set stSet;
		struct timeval stTimeout;

		stTimeout.tv_sec = 1;
		stTimeout.tv_usec = 0;
		FD_ZERO(&stSet);
		FD_SET(s32Socket, &stSet);

		if (select(s32Socket + 1, &stSet, NULL, NULL, &stTimeout) <= 0)
		{
		}
		else if (FD_ISSET(s32Socket, &stSet))
		{
			/* we need read some info from the server */
			struct sockaddr_in stRecvAddr = {0};
			int32_t s32RecvLen = 0;
			{
				/* I don't think we should check the return value */
				socklen_t s32Len = sizeof(struct sockaddr_in);
				s32RecvLen = recvfrom(s32Socket, pUDPBuf, UDP_MSG_BUF_LEN,
						0, (struct sockaddr *)(&stRecvAddr), &s32Len);
				if (s32RecvLen <= 0 || (s32Len != sizeof (struct sockaddr_in)))
				{
					continue;
				}
			}

			MCSResolve((const char *)pUDPBuf, 0, UDPMCSResolveCallBack, NULL);
			/* I don't think we should check the return value
			s32Err = sendto(s32Socket,pUDPBuf, 0,
					MSG_NOSIGNAL, (struct sockaddr *)(&stRecvAddr), sizeof(struct sockaddr));
			if (s32Err < 0)
			{
				PRINT("UDP send message error: %s\n", strerror(errno));
				continue;
			}*/
		}
	}

	if (s32Socket >= 0)
	{
		close(s32Socket);
	}
	return NULL;
}

int main(int argc, char * const argv[])
{
	int32_t s32Err = 0;
	pthread_t s32ThreadUDP = -1;

	SignalRegister();

	s32Err = MakeThread(ThreadUDP, NULL, false, &s32ThreadUDP, false);
	if (s32Err < 0)
	{
		goto end;
		return -1;
	}

	while (!g_boIsExit)
	{
		sleep(1);
	}

end:
	g_boIsExit = true;

	if (s32ThreadUDP >= 0)
	{
		pthread_join(s32ThreadUDP, NULL);
	}
	return 0;
}

