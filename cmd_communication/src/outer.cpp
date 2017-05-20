/*
 * outer.cpp
 *
 *  Created on: 2017年4月26日
 *      Author: lyndon
 */
#include "cmd_communication.h"

void *ThreadTCPOutCMD(void *pArg)
{
	StThreadArg *pThreadArg = (StThreadArg *)pArg;
	int32_t s32Socket = -1;

	s32Socket = socket(AF_INET, SOCK_STREAM, 0);
	if (s32Socket < 0)
	{
		PRINT("socket error: %s\n", strerror(errno));
		return NULL;
	}


	/* bind to address any, and port TCP_SERVER_PORT */
	{
	    struct sockaddr_in stAddr;
	    bzero(&stAddr, sizeof(struct sockaddr_in));
	    stAddr.sin_family = AF_INET;
	    stAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	    stAddr.sin_port = htons(TCP_CMD_SERVER_PORT);

	    if(bind(s32Socket,(struct sockaddr *)&(stAddr), sizeof(struct sockaddr_in)) == -1)
	    {
	        PRINT("bind the socket error: %s\n", strerror(errno));
			close(s32Socket);
			return NULL;
	    }
	}

    if (listen(s32Socket, 1) < 0)
    {
        PRINT("Listen the socket error: %s\n", strerror(errno));
		close(s32Socket);
		return NULL;
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
			continue;
		}
		else if (FD_ISSET(s32Socket, &stSet))
		{
			int32_t s32Client = ServerAccept(s32Socket);
			if (s32Client < 0)
			{
				continue;
			}
			{
				struct timeval stTimeout;
				stTimeout.tv_sec  = 0;
				stTimeout.tv_usec = 10 * 1000;
				if(setsockopt(s32Client, SOL_SOCKET, SO_RCVTIMEO, &stTimeout, sizeof(struct timeval)) < 0)
				{
					close(s32Client);
					continue;
				}
				stTimeout.tv_usec = 50 * 1000;
				if(setsockopt(s32Client, SOL_SOCKET, SO_SNDTIMEO, &stTimeout, sizeof(struct timeval)) < 0)
				{
					close(s32Client);
					continue;
				}

			}
			PRINT("insert a socket into the control block: %d\n", s32Client);
			if (pThreadArg->pCtrl->InsertASocket(s32Client) != 0)
			{
				close(s32Client);
			}
		}
	}
	PRINT("close the server for %s\n", __FUNCTION__);
	close(s32Socket);
	return NULL;
}


void *ThreadTCPOutCMDParse(void *pArg)
{
	StThreadArg *pThreadArg = (StThreadArg *)pArg;
	CSockCtrl *pCtrl = pThreadArg->pCtrl;
	int32_t s32Err = 0;
	while (!g_boIsExit)
	{
		int32_t *pSock;
		int32_t s32Count = 0;
		pSock = pCtrl->GetSocket(s32Count);
		if ((s32Count == 0) || (pSock == NULL))
		{
			PRINT("pCtrl->GetSocket nothing\n");
			usleep(300 * 1000);
		}
		else
		{
			CMallocAutoRelease CAutoSock(pSock);
			struct pollfd *pFDS = (struct pollfd *)calloc(s32Count, sizeof(struct pollfd));
			if (pFDS == NULL)
			{
				continue;
			}
			CMallocAutoRelease CAutoFDS(pFDS);

			PRINT("poll %d socket(first is: %d)\n", s32Count, pSock[0]);
			for (int32_t i = 0; i < s32Count; i++)
			{
				pFDS[i].fd = pSock[i];
				pFDS[i].events = POLLIN | POLLRDHUP;
			}
			s32Err = poll(pFDS, s32Count, 1000);
			PRINT("maybe socket wake up: %d\n", s32Err);
			if (s32Err == 0)	/* timeout */
			{
				continue;
			}
			else if (s32Err < 0)
			{

			}

			for (int32_t i = 0; i < s32Count; i++)
			{
				char c8Buf[1024];
				PRINT("socket %d, revents: %08x\n", pSock[i], pFDS[i].revents);
				if ((pFDS[i].revents & POLLIN) != 0)
				{
					PRINT("begin to get some message\n");
					while (!g_boIsExit)
					{
						int32_t s32RecvLen = recv(pSock[i], c8Buf, 1024, 0);
						PRINT("get message length: %d\n", s32RecvLen);
						if (s32RecvLen == 0)
						{
							break;
						}
						else if (s32RecvLen < 0)
						{
							if (errno == EAGAIN) /* timeout */
							{

							}
							else	/* some error happened on this sock */
							{
								PRINT("Error happened on socket(%d): %s\n", pSock[i], strerror(errno));
								pCtrl->DeleteASocket(pSock[i]);

							}
							break;
						}
						else
						{
							void *pBuf = c8Buf;
							int32_t s32Length = s32RecvLen;
							while (!g_boIsExit)
							{
								StCmdMsg stMsg;
								s32Err = pCtrl->GetValidMsg(pSock[i], pBuf, s32Length, stMsg);
								if ((s32Err != 0) || (stMsg.pData == NULL))
								{
									break;
								}
								PRINT("Get some message: %p, %08x, %d\n\n",
										stMsg.pData, stMsg.s32Protocol, stMsg.u32Length);
								pBuf = NULL;
								if (stMsg.s32Protocol == _Protocol_YNA)
								{
									int32_t s32Sock = ClientConnect(WORK_DIR "uart_daemon_server.socket");
									if (s32Sock >= 0)
									{
										MCSSyncSend(s32Sock, 100, _MCS_Cmd_UartDaemon, stMsg.u32Length, stMsg.pData);
										close(s32Sock);
									}
								}
								s32Length = 0;
							}
						}
					}

				}

				if ((pFDS[i].revents & (POLLRDHUP | POLLERR | POLLHUP)) != 0)
				{
					PRINT("Error happened on socket(%d): %s\n", pSock[i], strerror(errno));
					pCtrl->DeleteASocket(pSock[i]);
					PRINT("pCtrl->DeleteASocket(pSock[i])\n");
				}
			}
		}

	}

	return NULL;
}


