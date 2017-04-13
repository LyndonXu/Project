/*
 * udp.cpp
 *
 *  Created on: 2017年4月12日
 *      Author: ubuntu
 */

#include "update_daemon.h"

#define UDP_MSG_BUF_LEN	2048
static uint8_t *s_pUDPBuf[UDP_MSG_BUF_LEN];


/* compare cmdline file to get the process's PID, if find it, return the PID(positive number) */
int32_t CompareCmdlineCB(const char *pCurPath, struct dirent *pInfo, void *pContext)
{
	char c8Buf[64];
	int32_t s32Fd;
	int32_t s32Tmp, i;
	const char **p2KeyStr = (const char **)pContext;

	/* not a process directory */
	if ((pInfo->d_name[0] > '9') || (pInfo->d_name[0] < '0'))
	{
		return 0;
	}

	sprintf(c8Buf, "%s%s/cmdline", pCurPath, pInfo->d_name);
	s32Fd = open(c8Buf, O_RDONLY);
	if (s32Fd < 0)
	{
		return 0;
	}

	s32Tmp = read(s32Fd, c8Buf, 62);
	close(s32Fd);
	if (s32Tmp < 0)
	{
		return 0;
	}
	for (i = 0; i < s32Tmp; i++)
	{
		if (c8Buf[i] == 0)
		{
			c8Buf[i] = ' ';
		}
	}
	if (s32Tmp < 20)
	{
		return 0;
	}
	c8Buf[s32Tmp + 1] = 0;
	PRINT("%s(%d)\n", c8Buf, s32Tmp);

	s32Tmp = 0;
	while (p2KeyStr[s32Tmp] != NULL)
	{
		/* the key work is not matching */
		if (strstr(c8Buf, p2KeyStr[s32Tmp]) == NULL)
		{
			return 0;
		}
		s32Tmp++;
	}
	return atoi(pInfo->d_name);
}

/* config the network interface from the configure structure */
void SetNetInterface(const char *pInterface, StNetIfConfigInner *pConfigInner)
{
	char c8Str[128];
	StNetInterfaceConfig *pConfig = &(pConfigInner->stConfig);
	if (pConfig->boIsDHCP)	/* for DHCP */
	{
		int32_t s32Err = 0;
		const char *pKeyStr[3] = {"udhcpc", pInterface, NULL};

		/* first, check whether the process had been run */
		s32Err = TraversalDir("/proc/", false, CompareCmdlineCB, (void *)pKeyStr);
		if (s32Err > 0)		/* we have set up it */
		{
			pConfigInner->s32DHCPPid = s32Err;
			sprintf(c8Str, "kill -SIGUSR1 %d", s32Err);
			PRINT("%s\n", c8Str);
			system(c8Str);	/* force a renew state */
			PRINT("udhcpc for %s had been set up and the PID is: %d\n", pInterface, s32Err);
		}
		else
		{
			/* run it */
			sprintf(c8Str, "udhcpc -b -i %s -s /sbin/udhcpc.sh", pInterface);
			PRINT("%s\n", c8Str);
			system(c8Str);

			/* get the PID */
			s32Err = TraversalDir("/proc/", false, CompareCmdlineCB, (void *)pKeyStr);
			if (s32Err > 0)
			{
				pConfigInner->s32DHCPPid = s32Err;
				PRINT("PID is: %d\n", s32Err);
			}
		}

	}
	else	/* for manual IP */
	{
		if (inet_addr(pConfig->c8IPV4) != INADDR_NONE)
		{
			sprintf(c8Str, "ifconfig %s %s", pInterface, pConfig->c8IPV4);
			PRINT("%s\n", c8Str);
			system(c8Str);
			if (inet_addr(pConfig->c8Mask) != INADDR_NONE)
			{
				sprintf(c8Str, "ifconfig %s netmask %s", pInterface, pConfig->c8Mask);
				PRINT("%s\n", c8Str);
				system(c8Str);
			}
			if (inet_addr(pConfig->c8Gateway) != INADDR_NONE)
			{
				sprintf(c8Str, "route add default gw %s dev %s metric 1", pConfig->c8Gateway, pInterface);
				PRINT("%s\n", c8Str);
				system(c8Str);
			}
			if (inet_addr(pConfig->c8DNS) != INADDR_NONE)
			{
				sprintf(c8Str, "echo nameserver %s > /etc/resolv.conf", pConfig->c8DNS);
				PRINT("%s\n", c8Str);
				system(c8Str);
			}
			if (inet_addr(pConfig->c8ReserveDNS) != INADDR_NONE)
			{
				sprintf(c8Str, "echo nameserver %s >> /etc/resolv.conf", pConfig->c8ReserveDNS);
				PRINT("%s\n", c8Str);
				system(c8Str);
			}
		}
	}
}


typedef struct _tagStMCSCBArg
{
	int32_t s32Socket;
	StNetIfConfigInner *pNetConfigInner;
	struct sockaddr_in *pClinetAddr;
}StMCSCBArg;

int32_t UDPMCSResolveCallBack(uint32_t u32CmdNum, uint32_t u32CmdCnt, uint32_t u32CmdSize,
        const char *pCmdData,
        void *pContext)
{
	StMCSCBArg *pArg = (StMCSCBArg *)pContext;
	int32_t s32Err = 0;
	uint32_t u32TotalLength = u32CmdCnt * u32CmdSize;
	uint32_t u32MCDLen = 0;
	void *pMCS = NULL;
	bool boNeedReboot = false;
	PRINT("cmd %d, cnt %d, size %d, %p\n", u32CmdNum, u32CmdCnt, u32CmdSize, pCmdData);

	switch (u32CmdNum)
	{
		case _UDP_Cmd_GetEthInfo:
		{
			pMCS = MCSMakeAnArrayVarialbleCmd(_MCS_Cmd_Echo | _UDP_Cmd_GetEthInfo,
					&(pArg->pNetConfigInner->stConfig), 1, sizeof(StNetInterfaceConfig), &u32MCDLen);

			break;
		}
		case _UDP_Cmd_SetEthInfo:
		{
			if (u32TotalLength != sizeof(StNetInterfaceConfig))
			{
				pMCS = MCSMakeAnArrayVarialbleCmd(MY_ERR(_Err_InvalidParam), NULL, 0, 0, &u32MCDLen);
			}
			else
			{
#if HAS_CROSS
				SetNetInterface(INTERFACE_NAME, &stNetIfConfig);
#endif
				/* get current information */
				{
					StIPV4Addr stAddr;
					GetInterfaceIPV4Addr(INTERFACE_NAME, &stAddr);
					memcpy(pArg->pNetConfigInner->stConfig.c8IPV4, stAddr.c8IPAddr, IPV4_ADDR_LENGTH * 3);
				}
				pMCS = MCSMakeAnArrayVarialbleCmd(_MCS_Cmd_Echo | _UDP_Cmd_GetEthInfo,
						&(pArg->pNetConfigInner->stConfig), 1, sizeof(StNetInterfaceConfig), &u32MCDLen);

			}

			break;
		}
		case _UDP_Cmd_SetMAC:
		{
			if (u32TotalLength != sizeof(StHardwareAddr))
			{
				pMCS = MCSMakeAnArrayVarialbleCmd(MY_ERR(_Err_InvalidParam), NULL, 0, 0, &u32MCDLen);
			}
			else
			{
				StHardwareAddr *pAddr = (StHardwareAddr *)pCmdData;
				if(strcmp(pAddr->c8OldMACAddr, pArg->pNetConfigInner->stConfig.c8MACAddr) != 0)
				{
					//pMCS = MCSMakeAnArrayVarialbleCmd(MY_ERR(_Err_InvalidParam), NULL, 0, 0, &u32MCDLen);
				}
				else
				{
					char c8Buf[256];
					sprintf(c8Buf, "mkdir -p %s", PROGRAM_DIR);
					PRINT("%s", c8Buf);
					system(c8Buf);
					sprintf(c8Buf, "echo HWAddr=%s > %s", pAddr->c8NewMACAddr, HW_ARRD_CONFIG);
					PRINT("%s", c8Buf);
					system(c8Buf);
					pMCS = MCSMakeAnArrayVarialbleCmd(_MCS_Cmd_Echo | _UDP_Cmd_SetMAC,
							NULL, 0, 0, &u32MCDLen);
#if HAS_CROSS
					boNeedReboot = true;
#endif
				}
			}
			break;
		}

		default:
			break;
	}
	if (pMCS != NULL)
	{
		s32Err = sendto(pArg->s32Socket, pMCS, u32MCDLen,
				MSG_NOSIGNAL, (struct sockaddr *)(pArg->pClinetAddr), sizeof(struct sockaddr));
		if (s32Err < 0)
		{
			PRINT("UDP send message error: %s\n", strerror(errno));
		}

		MCSFree(pMCS);
	}

	if (boNeedReboot)
	{
		system("reboot");
	}
	return 0;
}
void *ThreadUDP(void *pArg)
{
	int32_t s32Socket = -1;
	int32_t s32Err = 0;

	StMCSCBArg stMCSCBArg;

	StNetIfConfigInner stNetIfConfig;

	/*
	 * load the configure
	 * */

#if HAS_CROSS
	SetNetInterface(INTERFACE_NAME, &stNetIfConfig);
#endif
	/* get current information */
	{
		StIPV4Addr stAddr;
		GetInterfaceIPV4Addr(INTERFACE_NAME, &stAddr);
		memcpy(stNetIfConfig.stConfig.c8IPV4, stAddr.c8IPAddr, IPV4_ADDR_LENGTH * 5 + MAC_ADDR_LENGTH);
	}


	s32Socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (s32Socket < 0)
	{
		PRINT("socket error: %s\n", strerror(errno));
		return NULL;
	}


	/* bind to address any, and port UDP_SERVER_PORT */
	{
	    struct sockaddr_in stAddr;
	    bzero(&stAddr, sizeof(struct sockaddr_in));
	    stAddr.sin_family = AF_INET;
	    stAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	    stAddr.sin_port = htons(UDP_SERVER_PORT);

	    if(bind(s32Socket,(struct sockaddr *)&(stAddr), sizeof(struct sockaddr_in)) == -1)
	    {
			close(s32Socket);
			return NULL;
	    }
	}

	/* enable broadcast */
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

	stMCSCBArg.s32Socket = s32Socket;
	stMCSCBArg.pNetConfigInner = &stNetIfConfig;

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
				s32RecvLen = recvfrom(s32Socket, s_pUDPBuf, UDP_MSG_BUF_LEN,
						0, (struct sockaddr *)(&stRecvAddr), &s32Len);
				if (s32RecvLen <= 0 || (s32Len != sizeof (struct sockaddr_in)))
				{
					continue;
				}
			}
			stMCSCBArg.pClinetAddr = &stRecvAddr;

			MCSResolve((const char *)s_pUDPBuf, 0, UDPMCSResolveCallBack, &stMCSCBArg);
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


