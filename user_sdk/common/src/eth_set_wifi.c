/****************************************************************************
 * Copyright(c), 2001-2060, ******************************* 版权所有
 ****************************************************************************
 * 文件名称             : eth_set_wifi.c
 * 版本                 : 0.0.1
 * 作者                 : 许龙杰
 * 创建日期             : 2014年12月12日
 * 描述                 : 
 ****************************************************************************/
#include "common_define.h"
#include <linux/if.h>
#include <arpa/inet.h>
#include "../inc/common.h"

bool BCPCheckSum(StBaseCommProtocol *pCmd)
{

	if (pCmd == NULL)
	{
		return false;
	}
	if (pCmd->u8DataLength < 4)
	{
		return false;
	}
	do
	{
		uint8_t u8CheckSum = 0;
		uint32_t i;
		uint32_t u32Length = pCmd->u8DataLength;
		uint8_t *pData = &(pCmd->u8StatID);
		u32Length &= 0xFF;
		for (i = 0; i < u32Length; i++)
		{
			u8CheckSum += pData[i];
		}
		if (u8CheckSum == pData[i])
		{
			return true;
		}
		else
		{
			pData[i] = u8CheckSum;
			return false;
		}
	}while(0);
	return false;
}

static void UDPSetWifiBuildIDPS(StBaseCommProtocol *pCmd, StPtlIDPS *pIDPS, StIPV4Addr *pLocalAddr)
{
	char *pTmp = (char *)((uint32_t)pCmd + sizeof(StBaseCommProtocol));
	pCmd->u8CommandHead = 0xA0;
	pCmd->u8DataLength = 4 + sizeof(StPtlIDPS) + IPV4_HEX_ADDR_LENGTH + MAC_HEX_ADDR_LENGTH; /* */
	pCmd->u8StatID = 0xA0;
	pCmd->u8SequenceID++;
	pCmd->u8ProductType = 0xA0;
	pCmd->u8CommandID = 0xF0;
	memcpy(pTmp, pIDPS, sizeof(StPtlIDPS));
	pTmp += sizeof(StPtlIDPS);
	inet_pton(AF_INET, pLocalAddr->c8IPAddr, pTmp);
	pTmp += IPV4_HEX_ADDR_LENGTH;

	memcpy(pTmp, pLocalAddr->c8MacAddr, MAC_HEX_ADDR_LENGTH);
	BCPCheckSum(pCmd);
}

static void UDPSetWifiResolveFAAndBuildFB(StBaseCommProtocol *pCmd,
		char c8Rand[RAND_NUM_CNT],
		const char c8ID[PRODUCT_ID_CNT],
		const char c8Key[XXTEA_KEY_CNT_CHAR],
		StIPV4Addr *pLocalAddr)
{
	char *pTmp = (char *)((uint32_t)pCmd + sizeof(StBaseCommProtocol));
	char *pID = pTmp;
	char *pRand1 = pTmp + PRODUCT_ID_CNT;
	char *pRand2 = pRand1 + RAND_NUM_CNT;
	uint32_t i;

	btea((int32_t *)pTmp, -(RAND_NUM_CNT / sizeof(int32_t)), (int32_t *)c8Key);
	for (i = 0; i < RAND_NUM_CNT; i++)
	{
		pRand1[RAND_NUM_CNT -1 - i] = pID[RAND_NUM_CNT -1 - i];
		c8Rand[i] = rand();
	}
	memcpy(pID, c8ID, PRODUCT_ID_CNT);
	memcpy(pRand2, c8Rand, RAND_NUM_CNT);
	btea((int32_t *)pRand2, -(RAND_NUM_CNT / sizeof(int32_t)), (int32_t *)c8Key);

	memcpy(pRand2 + RAND_NUM_CNT, pLocalAddr->c8MacAddr, MAC_HEX_ADDR_LENGTH);

	pCmd->u8CommandHead = 0xA0;
	pCmd->u8DataLength = 4 + PRODUCT_ID_CNT + RAND_NUM_CNT + RAND_NUM_CNT + MAC_HEX_ADDR_LENGTH; /* */
	pCmd->u8StatID = 0x00;
	pCmd->u8SequenceID++;
	pCmd->u8ProductType = 0xA0;
	pCmd->u8CommandID = 0xFB;
	BCPCheckSum(pCmd);
}


static uint8_t UDPSetWifiResolveFCAndBuildFD_E(StBaseCommProtocol *pCmd,
		char c8Rand[RAND_NUM_CNT], StIPV4Addr *pLocalAddr)
{
	char *pTmp = (char *)((uint32_t)pCmd + sizeof(StBaseCommProtocol));
	char *pRand2 = pTmp;

	if (memcmp(pRand2, c8Rand, RAND_NUM_CNT) == 0)
	{
		pCmd->u8CommandID = 0xFD;
	}
	else
	{
		pCmd->u8CommandID = 0xFE;
	}

	memcpy(pTmp, pLocalAddr->c8MacAddr, MAC_HEX_ADDR_LENGTH);

	pCmd->u8CommandHead = 0xA0;
	pCmd->u8DataLength = 4 + MAC_HEX_ADDR_LENGTH; /* */
	pCmd->u8StatID = 0x00;
	pCmd->u8SequenceID++;
	pCmd->u8ProductType = 0xA0;
	BCPCheckSum(pCmd);
	return pCmd->u8CommandID;
}

#define UDP_SET_WIFI_SSID_LENGTH		32
#define UDP_SET_WIFI_PWD_LENGTH			32
typedef struct _tagStUDPSetStWifiInfo
{
	char c8Reserved;
	char c8SecurityType;
	char c8SSID[UDP_SET_WIFI_SSID_LENGTH];
	char c8Password[UDP_SET_WIFI_PWD_LENGTH];
}StUDPSetStWifiInfo;

static void UDPSetWifiResolveE8AndBuildE5(StBaseCommProtocol *pCmd,
		const char c8Key[XXTEA_KEY_CNT_CHAR], StIPV4Addr *pLocalAddr)
{
	StUDPSetStWifiInfo *pInfo = (StUDPSetStWifiInfo *)((uint32_t)pCmd + sizeof(StBaseCommProtocol));
	char c8SSID[UDP_SET_WIFI_SSID_LENGTH + 1];
	char *pTmp = (char *)((uint32_t)pCmd + sizeof(StBaseCommProtocol));
	char c8Password[UDP_SET_WIFI_PWD_LENGTH + 1];

	memcpy(c8SSID, pInfo->c8SSID, UDP_SET_WIFI_SSID_LENGTH);
	memcpy(c8Password, pInfo->c8Password, UDP_SET_WIFI_PWD_LENGTH);

	btea((int32_t *)(c8SSID), -(UDP_SET_WIFI_SSID_LENGTH / sizeof(int32_t)), (int32_t *)c8Key);
	btea((int32_t *)(c8Password), -(UDP_SET_WIFI_PWD_LENGTH / sizeof(int32_t)), (int32_t *)c8Key);

	c8SSID[UDP_SET_WIFI_SSID_LENGTH] = 0;
	c8Password[UDP_SET_WIFI_PWD_LENGTH] = 0;
	PRINT("SSID: %s\n", c8SSID);
	PRINT("PSK: %s\n", c8Password);

	/* <TODO> send to sundries thread and get echo */
	do
	{
		StMCIWIFISSIDAndPSK stSSIDAndPSK = {c8SSID, c8Password};
		int32_t s32Return = 0;
		int32_t s32Err = 0;
		char *pEnsureFlag = pTmp;
		s32Err = MCSSendToServerAndGetEcho(SUNDRIES_SOCKET, _MCS_Cmd_Inner_SetWifiSSIDAndPSK,
				&stSSIDAndPSK, sizeof(StMCIWIFISSIDAndPSK), &s32Return, sizeof(int32_t));
		if ((s32Err != 0) || (s32Return != 0))
		{
			pEnsureFlag[0] = 2;
		}
		else
		{
			pEnsureFlag[0] = 1;
		}

	}while(0);

	memcpy(pTmp + 1, pLocalAddr->c8MacAddr, MAC_HEX_ADDR_LENGTH);

	pCmd->u8CommandHead = 0xA0;
	pCmd->u8DataLength = 5 + MAC_HEX_ADDR_LENGTH; /* */
	pCmd->u8StatID = 0x00;
	pCmd->u8SequenceID++;
	pCmd->u8ProductType = 0xA0;
	pCmd->u8CommandID = 0xE5;

	BCPCheckSum(pCmd);
}
static void UDPSetWifiResolveE9AndBuildE0(StBaseCommProtocol *pCmd, StIPV4Addr *pLocalAddr)
{
	char *pTmp = (char *)((uint32_t)pCmd + sizeof(StBaseCommProtocol));
	/* <TODO> check net state */
	do
	{
		int32_t s32Return = 0;
		int32_t s32Err = 0;
		char *pEnsureFlag = pTmp;
		s32Err = MCSSendToServerAndGetEcho(SUNDRIES_SOCKET, _MCS_Cmd_Inner_GetWifiConnectStatus,
				NULL, 0, &s32Return, sizeof(int32_t));
		if ((s32Err != 0) || (s32Return != 0))
		{
			pEnsureFlag[0] = 2;
		}
		else
		{
			pEnsureFlag[0] = 1;
		}

	}while(0);

	memcpy(pTmp + 1, pLocalAddr->c8MacAddr, MAC_HEX_ADDR_LENGTH);

	pCmd->u8CommandHead = 0xA0;
	pCmd->u8DataLength = 5 + MAC_HEX_ADDR_LENGTH; /* */
	pCmd->u8StatID = 0x00;
	pCmd->u8SequenceID++;
	pCmd->u8ProductType = 0xA0;
	pCmd->u8CommandID = 0xE0;

	BCPCheckSum(pCmd);
}

void *ThreadEthSetWifi(void *pArg)
{
	StPtlIDPS stIDPS;
	StIPV4Addr stAddr[8];
	uint32_t u32Cnt = 8;
	uint32_t i, u32AddrIndex;
	int32_t s32SockFd = -1;
	struct sockaddr_in stLocalAddrr;
	struct sockaddr stQAAddr;
	char c8Buf[512];
	char c8Rand[RAND_NUM_CNT];
	StBaseCommProtocol *pCMDApp = (StBaseCommProtocol *)c8Buf;

	EmUDPSetWIFIStat emStateMachine = _UDP_SET_WIFI_Scan;

	memset(&stIDPS, 0, sizeof(StPtlIDPS));
	GetIDPSStruct(&stIDPS);

	GetIPV4Addr(stAddr, &u32Cnt);
	u32AddrIndex = u32Cnt;
	for (i = 0; i < u32Cnt; i++)
	{
#if defined _DEBUG
		uint64_t *pAddr = (uint64_t *) stAddr[i].c8MacAddr;
#endif
		if (strcmp(stAddr[i].c8Name, ETH_LAN_NAME) == 0)
		{
			PRINT("name: %s, IP: %s, MAC: %012llX\n", stAddr[i].c8Name, stAddr[i].c8IPAddr, *pAddr);
			u32AddrIndex = i;
			break;
		}
	}

	s32SockFd = socket(AF_INET, SOCK_DGRAM, 0);
	if (s32SockFd < 0)
	{
		PRINT("create socket error: %s\n", strerror(errno));
		goto end;
	}

	bzero(&stLocalAddrr, sizeof(stLocalAddrr));
	stLocalAddrr.sin_family = AF_INET;
	stLocalAddrr.sin_addr.s_addr = htonl(INADDR_ANY);
	stLocalAddrr.sin_port = htons(UDP_SETWIFI_PORT);
	PRINT("PORT: %d\n", UDP_SETWIFI_PORT);
	do
	{
		struct ifreq interface;
		socklen_t s32Length = IFNAMSIZ;
		strncpy(interface.ifr_ifrn.ifrn_name, ETH_LAN_NAME, IFNAMSIZ);
		if(setsockopt(s32SockFd, SOL_SOCKET, SO_BINDTODEVICE, interface.ifr_ifrn.ifrn_name, s32Length) < 0)
		{
			PRINT("setsockopt SO_BINDTODEVICE err: %s!\n", strerror(errno));
		}
		else
		{
			PRINT("setsockopt sockfd bind to %s\n", interface.ifr_ifrn.ifrn_name);

		}
	}while(0);

	if (bind(s32SockFd, (struct sockaddr *) &stLocalAddrr, sizeof(stLocalAddrr)) == -1)
	{
		PRINT("bind err: %s\n", strerror(errno));
		goto end;
	}
	do
	{
		struct timeval stTimeout;
		stTimeout.tv_sec  = 2;
		stTimeout.tv_usec = 0;
		if(setsockopt(s32SockFd, SOL_SOCKET, SO_RCVTIMEO, &stTimeout, sizeof(struct timeval)) < 0)
		{
			close(s32SockFd);
			return NULL;
		}

		if(setsockopt(s32SockFd, SOL_SOCKET, SO_SNDTIMEO, &stTimeout, sizeof(struct timeval)) < 0)
		{
			close(s32SockFd);
			return NULL;
		}
	}while(0);

	while (!g_boIsExit)
	{
		struct sockaddr stClientAddr;
		socklen_t s32LenInOut = sizeof(stClientAddr);
		int32_t s32Recv = recvfrom(s32SockFd, c8Buf, 512, MSG_NOSIGNAL, &stClientAddr, &s32LenInOut);
		if (s32Recv <= 0)
		{
			PRINT("cannot get a message\n");
			continue;
		}
		PRINT("get a message\n");
#if 0
		if ((s32Recv < 6) || (s32Recv > 70))
		{
			PRINT("unknown message\n");
			continue;
		}
#endif
		if (pCMDApp->u8CommandHead != 0xB0)
		{
			PRINT("unknown message\n");
			continue;
		}
		if (!BCPCheckSum(pCMDApp))
		{
			PRINT("check sum error\n");
			continue;
		}

		switch (pCMDApp->u8CommandID)
		{
		case 0xD0:
		{
			if (pCMDApp->u8ProductType != 0xFF)
			{
				PRINT("error product type\n");
				break;
			}
			/*
			 *  why bigger than FA? if I have send answer the command D0, then, my state will be changed to FA,
			 *  but the client maybe has not received the answer, so, the client maybe send the command D0 again.
			 *  same reason for the following
			 */
			if (emStateMachine > _UDP_SET_WIFI_Auth_FA)
			{
				PRINT("my device is busy\n");
				break;
			}
			UDPSetWifiBuildIDPS(pCMDApp, &stIDPS, stAddr + u32AddrIndex);

			sendto(s32SockFd, pCMDApp, pCMDApp->u8DataLength + 3, MSG_NOSIGNAL, &stClientAddr, s32LenInOut);
			stQAAddr = stClientAddr;
			emStateMachine = _UDP_SET_WIFI_Auth_FA;
			break;
		}
		case 0xE8:
		{
			if (pCMDApp->u8ProductType != 0xA0)
			{
				PRINT("error product type\n");
				break;
			}
			if(emStateMachine != _UDP_SET_WIFI_QA)
			{
				PRINT("scan D0 command first or you have not AUTH\n");
				break;
			}

			if (memcmp(&stQAAddr, &stClientAddr, sizeof(struct sockaddr)) != 0)
			{
				PRINT("I don't know this IP\n");
				break;
			}
			UDPSetWifiResolveE8AndBuildE5(pCMDApp, g_c8Key, stAddr + u32AddrIndex);
			sendto(s32SockFd, pCMDApp, pCMDApp->u8DataLength + 3, MSG_NOSIGNAL, &stClientAddr, s32LenInOut);
			break;
		}
		case 0xE9:
		{
			if (pCMDApp->u8ProductType != 0xA0)
			{
				PRINT("error product type\n");
				break;
			}
			if(emStateMachine != _UDP_SET_WIFI_QA)
			{
				PRINT("scan D0 command first or you have not AUTH\n");
				break;
			}

			if (memcmp(&stQAAddr, &stClientAddr, sizeof(struct sockaddr)) != 0)
			{
				PRINT("I don't know this IP\n");
				break;
			}
			UDPSetWifiResolveE9AndBuildE0(pCMDApp, stAddr + u32AddrIndex);
			sendto(s32SockFd, pCMDApp, pCMDApp->u8DataLength + 3, MSG_NOSIGNAL, &stClientAddr, s32LenInOut);
			break;
		}
		case 0xFA:
		{
			if (pCMDApp->u8ProductType != 0xA0)
			{
				PRINT("error product type\n");
				break;
			}
			if(emStateMachine <= _UDP_SET_WIFI_Scan)
			{
				PRINT("scan D0 command first\n");
				break;
			}
			if (memcmp(&stQAAddr, &stClientAddr, sizeof(struct sockaddr)) != 0)
			{
				PRINT(" scan wifi D0 command first type\n");
				break;
			}
			if (emStateMachine > _UDP_SET_WIFI_Auth_FC)
			{
				PRINT("I think I have in the QA state\n");
				break;
			}
			UDPSetWifiResolveFAAndBuildFB(pCMDApp, c8Rand, g_c8ID, g_c8Key, stAddr + u32AddrIndex);
			sendto(s32SockFd, pCMDApp, pCMDApp->u8DataLength + 3, MSG_NOSIGNAL, &stClientAddr, s32LenInOut);
			emStateMachine = _UDP_SET_WIFI_Auth_FC;
			break;
		}
		case 0xFC:
		{
			if (pCMDApp->u8ProductType != 0xA0)
			{
				PRINT("error product type\n");
				break;
			}
			if(emStateMachine <= _UDP_SET_WIFI_Auth_FA)
			{
				PRINT("AUTH FA command first\n");
				break;
			}
			if (memcmp(&stQAAddr, &stClientAddr, sizeof(struct sockaddr)) != 0)
			{
				PRINT(" scan wifi D0 command first type\n");
				break;
			}
			if (emStateMachine > _UDP_SET_WIFI_QA)
			{
				PRINT("AUTH FC command first\n");
				break;
			}
			if (UDPSetWifiResolveFCAndBuildFD_E(pCMDApp, c8Rand, stAddr + u32AddrIndex) == 0xFD)
			{
				emStateMachine = _UDP_SET_WIFI_QA;
			}
			else
			{
				emStateMachine = _UDP_SET_WIFI_Scan;
				memset(&stQAAddr, 0, sizeof(stQAAddr));
			}
			sendto(s32SockFd, pCMDApp, pCMDApp->u8DataLength + 3, MSG_NOSIGNAL, &stClientAddr, s32LenInOut);
			break;
		}
		case 0xFE:
		{
			emStateMachine = _UDP_SET_WIFI_Scan;
			memset(&stQAAddr, 0, sizeof(stQAAddr));
			break;
		}
		default:
			break;
		}
	}
end:
	if (s32SockFd >=0 )
	{
		close(s32SockFd);
	}
	return 0;
}

