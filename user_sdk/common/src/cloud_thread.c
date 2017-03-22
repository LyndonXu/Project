/****************************************************************************
 * Copyright(c), 2001-2060, ******************************* 版权所有
 ****************************************************************************
 * 文件名称             : cloud_thread.c
 * 版本                 : 0.0.1
 * 作者                 : 许龙杰
 * 创建日期             : 2014年5月16日
 * 描述                 : 
 ****************************************************************************/
#include "common_define.h"
#include "json/json.h"
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include "../inc/common.h"

#if HAS_CROSS
#include "mainprocess.h"
#endif

#if defined USING_SELF_DNS_RESOLVE
#undef GetHostIPV4Addr
#define GetHostIPV4Addr(x, y, z)		GetHostIPV4AddrTimeout(x, 5000, y, z)
#endif


static int32_t s_s32CloudHandle = 0;
static bool s_boIsCloudOnline = false;

/* <TODO> I think I should get the key, ID and URL from other process or server */
static char s_c8ServerDomain [64] = COORDINATION_DOMAINA;
static char s_c8GatewayRegion [32] = DEFAULT_REGION;
static char s_c8GatewaySN [32] = DEFAULT_SN;
char g_c8ID [PRODUCT_ID_CNT] =
{
	0x31, 0x34, 0x30, 0x36, 0x31, 0x38, 0x30, 0x30,
	0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x32, 0x34,
};
char g_c8Key [XXTEA_KEY_CNT_CHAR] =
{
	0x2f, 0xe7, 0x8b, 0x0d, 0x91, 0x3d, 0x72, 0x7c,
	0x55, 0xb4, 0x0f, 0x48, 0x3b, 0x52, 0x5d, 0x2d
};

#define CUE_THREAD_CNT		2


const uint16_t c_u16SendTime[MAX_REPEAT_CNT] =
{
	1000, 2000, 4000, 8000, 15000
};

/*
 * why we should open a socket reading and writing it abidingly? think that, we send some message via a socket
 * and then close it. normally, we bind a port randomly(allocated by the system), after that we send a message
 * to a server, the server will get our IP address and port, and the server will echo a message via this
 * address:port, but unfortunately, we have closed the channel, how we can get the echo?
 *
 *
 */
void *ThreadKeepAlive(void *pArg)
{
	uint32_t u32RepeatCnt = 0;
	uint32_t u32TimeNeedToSleep = 0;/*CLOUD_KEEPALIVE_TIME * 1000;*/
	bool boIsSetTime = false;
	uint64_t u64SendTime = TimeGetTime();


	StCloudDomain stStat = {{_Cloud_IsOnline}};
	char c8Domain[64];
	int32_t s32Socket = -1;
	UnUDPMsg unUDPMsg;
	uint16_t u16QueueNum = 0;

	StUDPKeepalive *pUDPKA = UDPKAInit();

	if (pUDPKA == NULL)
	{
		//PrintLog("error memory\n");
		PRINT("error memory\n");
		goto end;
	}

	s32Socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (s32Socket < 0)
	{
		PrintLog("socket error: %s\n", strerror(errno));
		PRINT("socket error: %s\n", strerror(errno));
		return NULL;
	}

	GetSNOfGateway(s_c8GatewaySN, 31);
	/*
	 * why we don't bind the socket to the local IP address?
	 * because we don't know whether the gateway has passed the authentication
	 */

	while (!g_boIsExit)
	{
		if (s_boIsCloudOnline)
		{
			fd_set stSet;
			struct timeval stTimeout;

			stTimeout.tv_sec = 1;
			stTimeout.tv_usec = 0;
			FD_ZERO(&stSet);
			FD_SET(s32Socket, &stSet);

			if (select(s32Socket + 1, &stSet, NULL, NULL, &stTimeout) <= 0)
			{
				goto next;
			}
			else if (FD_ISSET(s32Socket, &stSet))
			{
				/* we need read some info from the server */
				struct sockaddr_in stRecvAddr = {0};
				int32_t s32RecvLen = 0;
				{
				/* I don't think we should check the return value */
				socklen_t s32Len = sizeof(struct sockaddr_in);
				s32RecvLen = recvfrom(s32Socket, &unUDPMsg, sizeof(UnUDPMsg),
						0, (struct sockaddr *)(&stRecvAddr), &s32Len);
				if (s32RecvLen <= 0 || (s32Len != sizeof (struct sockaddr_in)))
				{
					goto next;
				}
				}
				/* compare stRecvAddr with stServerAddr to check whether the message is mine */
				/* get the domain and port from the database */
				if (CloudGetDomainFromRegion(s_s32CloudHandle, _Region_UDP,
						s_c8GatewayRegion , c8Domain, sizeof(c8Domain)) != 0)
				{
					goto next;
				}
				/* translate the string */
				GetDomainPortFromString(c8Domain, stStat.c8Domain, 64, &(stStat.s32Port));
				{
				struct sockaddr_in stServerAddr = {0};
				stServerAddr.sin_family = AF_INET;
				stServerAddr.sin_port = htons(stStat.s32Port);
				/* get the IPV4 address of host via DNS(in this application, the host just has one IP address)  */
				if (GetHostIPV4Addr(stStat.c8Domain, NULL, &(stServerAddr.sin_addr)) != 0)
				{
					goto next;
				}
				else if(memcmp(&stServerAddr, &stRecvAddr, sizeof(struct sockaddr)) == 0)
				{
					/* well, now, I get a right message */
					if (memcmp(unUDPMsg.stHead.c8HeadName, UDP_MSG_HEAD_NAME, UDP_MSG_HEAD_NAME_LENGTH) != 0)
					{
						unUDPMsg.stHead.c8Var[0] = 0;
						PRINT("head name error: %s\n", unUDPMsg.stHead.c8HeadName);
						goto next;
					}
					/* my message */
					switch (unUDPMsg.stHead.u16Cmd)
					{
					case 1:		/* heartbeat */
					{
						uint64_t u64GetTime = TimeGetTime();
						StUDPHeartBeat *pData = (void *)(&(unUDPMsg.pData));

						UDPKAAddAReceivedTime(pUDPKA, pData->u16QueueNum, u64GetTime,
								unUDPMsg.stHead.u64TimeStamp);
						if (!boIsSetTime)
						{
#if HAS_CROSS
							struct timeval stTime;
							stTime.tv_sec = unUDPMsg.stHead.u64TimeStamp / 1000;
							stTime.tv_usec = (unUDPMsg.stHead.u64TimeStamp % 1000) * 1000;
							settimeofday(&stTime, NULL);
							UDPKAClearTimeDiff(pUDPKA);
#endif
							boIsSetTime = true;
						}
						PRINT("get echo: %d\n", pData->u16QueueNum);
						PRINT("server %s:%d, and the length is: %d\n",
								inet_ntoa(stRecvAddr.sin_addr), htons(stRecvAddr.sin_port),
								s32RecvLen);

						u32TimeNeedToSleep = c_u16SendTime[u32RepeatCnt - 1];
						u32TimeNeedToSleep = u32TimeNeedToSleep * 2 - 1000;
						{
							uint32_t u32Tmp = TimeGetTime() - u64SendTime;
							u32Tmp = c_u16SendTime[u32RepeatCnt - 1] - u32Tmp;
							u32TimeNeedToSleep = CLOUD_KEEPALIVE_TIME * 1000 - (u32TimeNeedToSleep - u32Tmp);
							PRINT("after recv a message, I need to sleep %u(ms) to send next message\n", u32TimeNeedToSleep);
						}
						u32RepeatCnt = 0;
#ifdef _DEBUG
						{
							int32_t i;
							for (i = 0; i < s32RecvLen; i++)
							{
								printf("0x%02hhx ", unUDPMsg.c8Buf[i]);
								if ((i & 0x07) == 0x07)
								{
									printf("\n");
								}
							}
							printf("\n");
						}
#endif
						break;
					}
					case 3 ... 4:		/* config message */
					{
#ifdef _DEBUG
						{
							int32_t i;
							printf("************** Get a config message *************************\n");
							for (i = 0; i < s32RecvLen; i++)
							{
								printf("0x%02hhx ", unUDPMsg.c8Buf[i]);
								if ((i & 0x07) == 0x07)
								{
									printf("\n");
								}
							}
							printf("\n");
						}
						PrintLog("%s", "I get a configure message\n");
#endif
#if HAS_CROSS
						{
							StInformUDP stMsg;
							StUDPConfig *pConfig = (StUDPConfig *)(&(unUDPMsg.pData));
							memcpy(stMsg.StConfig.pSN, pConfig->c8GatewaySN, sizeof(pConfig->c8GatewaySN));
							if (unUDPMsg.stHead.u16Cmd == 3)
							{
								stMsg.InformCMD = CMD_Gateway_CFG_A;
							}
							else
							{
								stMsg.InformCMD = CMD_Device_CFG_A;
								memcpy(stMsg.StConfig.pData, pConfig->c8ProductSN, sizeof(pConfig->c8ProductSN));
							}
							UnixSendAndReturn(&stMsg, &stMsg);
						}
#endif
						break;
					}
					default:
						PRINT("unkown command: %d\n", unUDPMsg.stHead.u16Cmd);
#ifdef _DEBUG
						{
							int32_t i;
							printf("************** Get a config message *************************\n");
							for (i = 0; i < s32RecvLen; i++)
							{
								printf("0x%02hhx ", unUDPMsg.c8Buf[i]);
								if ((i & 0x07) == 0x07)
								{
									printf("\n");
								}
							}
							printf("\n");
						}
						PrintLog("%s", "I get a message, but I down know the command\n");
#endif
						break;
					}
				}
				else
				{
					PRINT("Unkown server %s:%d, and the length is: %d\n",
						inet_ntoa(stRecvAddr.sin_addr), htons(stRecvAddr.sin_port),
						s32RecvLen);
#ifdef _DEBUG
					{
						int32_t i;
						printf("************** Get a config message *************************\n");
						for (i = 0; i < s32RecvLen; i++)
						{
							printf("0x%02hhx ", unUDPMsg.c8Buf[i]);
							if ((i & 0x07) == 0x07)
							{
								printf("\n");
							}
						}
						printf("\n");
					}
					PrintLog("%s", "I get a message from a unknown server\n");
#endif
					}
				}
			}
next:
			if (UPDKAIsTimeOut(pUDPKA))
			{

				PRINT("%s", "I cannot keep alive\n");
				PrintLog("%s", "I cannot keep alive\n");
				s_boIsCloudOnline = false;
				{
					StCloudDomain stStatTmp = {{_Cloud_IsNotOnline}};
					CloudSetStat(s_s32CloudHandle, &stStatTmp.stStat);
				}
				UDPKAReset(pUDPKA);
				continue;
			}

			if ((TimeGetTime() - u64SendTime) < u32TimeNeedToSleep)
			{
				int64_t s64TimeDiff = 0;
				if (UDPKAGetTimeDiff(pUDPKA, &s64TimeDiff) == 0)
				{
					PRINT("time diff is: %lld\n", s64TimeDiff);
#if HAS_CROSS
					if ((s64TimeDiff < -1000) || (s64TimeDiff > 1000))
					{
						struct timeval stTime;
						uint64_t u64Time = TimeGetTime();
						u64Time += s64TimeDiff;
						stTime.tv_sec = u64Time / 1000;
						stTime.tv_usec = (u64Time % 1000) * 1000;
						settimeofday(&stTime, NULL);
						u64Time /= 1000;
						PRINT("adjust time to: %s\n\n", ctime((void *)(&u64Time)));
						/* after we adjust the time difference, we clear this statistics */
						UDPKAClearTimeDiff(pUDPKA);
					}
#endif
				}
			}
			else
			{
				int32_t s32Err = CloudGetDomainFromRegion(s_s32CloudHandle, _Region_UDP,
						s_c8GatewayRegion , c8Domain, sizeof(c8Domain));
				if (s32Err != 0)
				{
					continue;
				}
				GetDomainPortFromString(c8Domain, stStat.c8Domain, 64, &(stStat.s32Port));

				{
				struct sockaddr_in stServerAddr = {0};
				stServerAddr.sin_family = AF_INET;
				stServerAddr.sin_port = htons(stStat.s32Port);
				s32Err = GetHostIPV4Addr(stStat.c8Domain, NULL, &(stServerAddr.sin_addr));
				if (s32Err != 0)
				{
					continue;
				}
				else
				{
					/* every time we send, we should check whether the local IP address is changed */
					s32Err = CloudGetStat(s_s32CloudHandle, &(stStat.stStat));
					if (s32Err != 0)
					{
						continue;
					}
					else
					{
						struct sockaddr stBindAddr = {0};
						struct sockaddr_in *pTmpAddr = (void *)(&stBindAddr);
						socklen_t s32Len = sizeof(struct sockaddr);
						/* get the current address to which the socket socket is bound */
						s32Err = getsockname(s32Socket, &stBindAddr, &s32Len);
						if (s32Err != 0)
						{
							/* what happened */
							continue;
						}
						else
						{
							struct in_addr stLocalInternetAddr = {0};
							if (inet_aton(stStat.stStat.c8ClientIPV4, &stLocalInternetAddr) == 0)
							{
								PRINT("client IP address error!\n");
								continue;
							}
							if (stLocalInternetAddr.s_addr != pTmpAddr->sin_addr.s_addr)
							{
								/*
								 * we find that the address has changed, we should re-bind the socket, but very sorry,
								 * there is no right system API can finish this directly, so, we close the socket
								 * and open a new socket, then, bind it
								 */
								PRINT("keepalive address re-bind to %s\n", stStat.stStat.c8ClientIPV4);

								close(s32Socket);
								s32Socket = socket(AF_INET, SOCK_DGRAM, 0);
								if (s32Socket < 0)
								{
									PrintLog("socket error: %s\n", strerror(errno));
									PRINT("socket error: %s\n", strerror(errno));
									return NULL;
								}
								pTmpAddr->sin_family = AF_INET;
								pTmpAddr->sin_port = htons(0);
								pTmpAddr->sin_addr = stLocalInternetAddr;
								if (bind(s32Socket, &stBindAddr, sizeof(struct sockaddr)))
								{
									/* may be the IP address has been changed */
									PRINT("bind error: %s\n", strerror(errno));
									PrintLog("bind error: %s\n", strerror(errno));
									s_boIsCloudOnline = false;
									continue;
								}
							}
						}
					}

					if (u32RepeatCnt >= MAX_REPEAT_CNT)
					{
						u32RepeatCnt = 0;
					}
					u32TimeNeedToSleep = c_u16SendTime[u32RepeatCnt];
					u32RepeatCnt++;
					PRINT("time that need to sleep is: %u\n", u32TimeNeedToSleep);

					memset(&unUDPMsg, 0, sizeof(UnUDPMsg));
					memcpy(unUDPMsg.stHead.c8HeadName, UDP_MSG_HEAD_NAME, UDP_MSG_HEAD_NAME_LENGTH);
					u64SendTime = unUDPMsg.stHead.u64TimeStamp = TimeGetTime();
					unUDPMsg.stHead.u16Cmd = 1;
					unUDPMsg.stHead.u16CmdLength = sizeof(StUDPHeartBeat);
					{
						StUDPHeartBeat *pData = (void *)(&(unUDPMsg.pData));
						pData->u16QueueNum = u16QueueNum;
						memcpy(pData->c8SN, s_c8GatewaySN, sizeof(pData->c8SN));
					}
					/* I don't think we should check the return value */
					s32Err = sendto(s32Socket,&unUDPMsg, sizeof(StUDPMsgHead) + sizeof(StUDPHeartBeat),
							MSG_NOSIGNAL, (struct sockaddr *)(&stServerAddr), sizeof(struct sockaddr));
					if (s32Err < 0)
					{
						PRINT("UDP send message error: %s\n", strerror(errno));
						PrintLog("UDP send message error: %s\n", strerror(errno));
						s_boIsCloudOnline = false;
						{
							StCloudDomain stStatTmp = {{_Cloud_IsNotOnline}};
							CloudSetStat(s_s32CloudHandle, &stStatTmp.stStat);
						}
						UDPKAReset(pUDPKA);
						continue;
					}
					UDPKAAddASendTime(pUDPKA, u16QueueNum, unUDPMsg.stHead.u64TimeStamp);
					PRINT("send a heart beat: %d at %lld\n", u16QueueNum, unUDPMsg.stHead.u64TimeStamp);
					u16QueueNum++;
#ifdef _DEBUG
					{

						int32_t i;
						for (i = 0; i < sizeof(StUDPMsgHead) + sizeof(StUDPHeartBeat); i++)
						{
							printf("0x%02hhx ", unUDPMsg.c8Buf[i]);
							if ((i & 0x07) == 0x07)
							{
								printf("\n");
							}
						}
						printf("\n\n");

					}
#endif
					}
				}
			}
		}
		else
		{
			sleep(1);
		}
	}
end:
	UDPKADestroy(pUDPKA);
	if (s32Socket >= 0)
	{
		close(s32Socket);
	}
	return NULL;
}

/*
 * thread for update
 */
void *ThreadUpdate(void *pArg)
{
	StCloudDomain stStat = {{_Cloud_IsNotOnline}};
	char c8Domain[64];
	uint64_t u64UpdateTime = 0;
	json_object *pObj = NULL;
	int32_t s32Err = 0;
	int32_t s32RepeatCnt = 0;

	json_object *ClouldGetLastVersion(StCloudDomain *pStat, int32_t *pErr);
	int32_t GetUpdateFile(StCloudStat *pCloudStat, json_object *pFileList, StMovingFileInfo **p2Info);

	while (!g_boIsExit)
	{
		sleep(1);
		if ((s_boIsCloudOnline) && ((TimeGetTime() - u64UpdateTime) > (CLOUD_UPDATE_TIME * 1000)))
		{
			/* <TODO> ...... */
			/* get update information */
			if (CloudGetDomainFromRegion(s_s32CloudHandle, _Region_HTTPS,
					s_c8GatewayRegion , c8Domain, sizeof(c8Domain)) != 0)
			{
				continue;
			}
			/* translate the string */
			GetDomainPortFromString(c8Domain, stStat.c8Domain, 64, &(stStat.s32Port));

			/* get cloud statu and our IP address */
			s32Err = CloudGetStat(s_s32CloudHandle, &(stStat.stStat));
			if (s32Err != 0)
			{
				continue;
			}

			if ((pObj = ClouldGetLastVersion(&stStat, &s32Err)) == NULL)
			{
				if ((s32RepeatCnt++) > 10)
				{
					u64UpdateTime = TimeGetTime();
					s32RepeatCnt = 0;
				}
				PRINT("Get last version error(0x%08x)!\n", s32Err);
			}
			else
			{

				StMovingFileInfo *pInfo = NULL;

				PRINT("\nreturn:\n%s\n", json_object_to_json_string_ext(pObj, JSON_C_TO_STRING_PRETTY));

				u64UpdateTime = TimeGetTime();

				s32Err = GetUpdateFile(&(stStat.stStat), pObj, &pInfo);
				if (s32Err != 0)
				{
					PRINT("GetUpdateFile(0x%08x)!\n", s32Err);
					goto next;
				}
				s32Err = UpdateFileCopyToRun(&pInfo);
				if (s32Err != 0)
				{
					PRINT("UpdateFileCopyToRun(0x%08x)!\n", s32Err);
					goto next;
				}
#if HAS_CROSS
#if 0
				s32Err = UpdateTheNewFile(pInfo, false);
				if (s32Err != 0)
				{
					PRINT("UpdateTheNewFile(0x%08x)!\n", s32Err);
				}
#else
				s32Err = UpdateTheNewFile(pInfo, true);
				if (s32Err != 0)
				{
					PRINT("UpdateTheNewFile(0x%08x)!\n", s32Err);
				}
				else
				{
					system("reboot");
				}
#endif
#endif
next:

				MovingFileInfoRelease(pInfo);
				json_object_put(pObj);
			}
		}

		if (s_boIsCloudOnline)
		{
			/* <TODO> check whether there is that a new MCU module has been added ...... */
		}
	}
	return NULL;
}

const PFUN_THREAD c_pFunThreadCue[CUE_THREAD_CNT] =
{
	ThreadKeepAlive, ThreadUpdate,
};

static int32_t AuthAndGetRegion(StCloudDomain *pStat, const char *pIPAddr, bool boIsCoordination)
{
	int32_t s32Err = 0;
	memcpy(pStat->stStat.c8ClientIPV4, pIPAddr, IPV4_ADDR_LENGTH);
	s32Err = CloudAuthentication(pStat, boIsCoordination, g_c8ID , g_c8Key );
	if ((!boIsCoordination) && (s32Err != 0))
	{
		boIsCoordination = true;
		s32Err = CloudAuthentication(pStat, boIsCoordination, g_c8ID , g_c8Key );
	}
	if (s32Err != 0)
	{
		/* oh no, I can't through the authentication with the cloud */
		PRINT("CloudAuthentication error: 0x%08x\n", s32Err);
	}
	else
	{
		StRegionMapping *pMap = NULL;
		uint32_t u32Cnt = 0;

		if ((s32Err = CloudGetSelfRegion(pStat, s_c8GatewayRegion, 16)) == 0)
		{
			PRINT("gateway's region is: %s\n", s_c8GatewayRegion);
		}
		else
		{
			return s32Err;
		}
		if ((s32Err = CloudGetRegionMapping(pStat, &pMap, &u32Cnt)) == 0)
		{
			uint32_t i = 0;
			for (i = 0; i < u32Cnt; i++)
			{
				PRINT("region: %s\n", pMap[i].c8Region);
				PRINT("cloud: %s\n", pMap[i].c8Cloud);
				PRINT("heartbeat: %s\n\n", pMap[i].c8Heartbeat);
				CloudSaveDomainViaRegion(s_s32CloudHandle, _Region_HTTPS, pMap[i].c8Region, pMap[i].c8Cloud);
				CloudSaveDomainViaRegion(s_s32CloudHandle, _Region_UDP, pMap[i].c8Region, pMap[i].c8Heartbeat);
			}
			free(pMap);
		}
		else
		{
			return s32Err;
		}

		PrintLog("IP %s through the authentication of the cloud %s\n", pStat->stStat.c8ClientIPV4, pStat->c8Domain);
		CloudSetStat(s_s32CloudHandle, &(pStat->stStat));
		s_boIsCloudOnline = true;
	}
	return s32Err;
}

static void SetCloudLed(int32_t s32Status)
{
	int32_t s32Fd = open(DEVNAME, O_RDWR);
	if(s32Fd >= 0)
	{
		ioctl(s32Fd, s32Status, NULL);
        close(s32Fd);
	}
}

/*
 * thread for authentication
 */
void *ThreadCloud(void *pArg)
{
	int32_t s32Err = 0;

	pthread_t s32TidCloud[CUE_THREAD_CNT] = {0};

	bool boIsCoordination = false;

	StCloudDomain stStat = {{_Cloud_IsOnline}};

	SetCloudLed(HG3_LED_Cld_Rd_Lt);

	/* get my region from information file */
	GetRegionOfGateway(s_c8GatewayRegion , sizeof(s_c8GatewayRegion) - 1);
	PRINT("region of gateway is: %s\n", s_c8GatewayRegion );
	GetServerDomainForGateway(s_c8ServerDomain, sizeof(s_c8ServerDomain) - 1);
	PRINT("server domain for gateway is: %s\n", s_c8ServerDomain );
#if HAS_CROSS
	{
#if defined _DEBUG
		int32_t u32Cnt = 5;
#endif
		int32_t s32Err;
		StInformUDP stMsg;
#if defined _DEBUG
		while (((u32Cnt--) != 0) && (!g_boIsExit))
#else
		while (!g_boIsExit)
#endif
		{
			stMsg.InformCMD = CMD_Request_ID_K;
			s32Err = UnixSendAndReturn(&stMsg, &stMsg);
			if ((s32Err == 0) && (stMsg.InformCMD == CMD_Request_ID_K))
			{
				memcpy(g_c8ID, stMsg.StIDKey.pID, PRODUCT_ID_CNT);
				memcpy(g_c8Key, stMsg.StIDKey.pKey, XXTEA_KEY_CNT_CHAR);
				break;
			}
			sleep(1);
		}
		if (g_boIsExit)
		{
			return NULL;
		}
	}
#endif

	s_s32CloudHandle = CloudInit(&s32Err);
	if (s_s32CloudHandle == 0)
	{
		PRINT("CloudInit error: 0x%08x", s32Err);
		return NULL;
	}

	/* get the cloud state and domain if cloud is online */
	{
	StCloudDomain stStatTmp = {{_Cloud_IsNotOnline}};
	CloudGetStat(s_s32CloudHandle, &stStatTmp.stStat);
	if (stStatTmp.stStat.emStat == _Cloud_IsOnline)
	{
		stStat = stStatTmp;
		PRINT("cloud is on line IP %s\n", stStat.stStat.c8ClientIPV4);
		s_boIsCloudOnline = true;
	}
	s32Err = CloudGetDomainFromRegion(s_s32CloudHandle, _Region_HTTPS,
			s_c8GatewayRegion , s_c8ServerDomain , sizeof(s_c8ServerDomain ));
	if (s32Err == 0)
	{
		PRINT("s_c8ServerDomain: %s\n", s_c8ServerDomain);
		boIsCoordination = true;
	}
	}

	GetDomainPortFromString(s_c8ServerDomain , stStat.c8Domain, 64, &(stStat.s32Port));
	PRINT("Domain: %s, Port: %d\n", stStat.c8Domain, stStat.s32Port);

	/* create correlation function */
	{
	int32_t i;
	for (i = 0; i < CUE_THREAD_CNT; i++)
	{
		MakeThread(c_pFunThreadCue[i], NULL, false, s32TidCloud + i, false);
	}
	}
#if HAS_CROSS
	do
	{
		enum
		{
			_Auth_Clear = 0,
			_Auth_Wifi_Connect,
			_Auth_Wifi_Check_status,
			_Auth_SetIP,
			_Auth_Auth,
		};
		bool boIsETH = true;
		uint32_t u32Step = _Auth_Clear;
		uint32_t u32RepeatCnt = 0;
		while (!g_boIsExit)
		{
			sleep(1);
			if (!s_boIsCloudOnline) /* cloud in not online, authenticate */
			{
				StIPV4Addr stIPV4Addr = {{0}};
				SetCloudLed(HG3_LED_Cld_Rd_Fl);
				PRINT("boIsETH: %s, u32Step: %d\n", boIsETH ? "true" : "false", u32Step);
				if (boIsETH)
				{
					/* clear WIFI configure */
					if (u32Step == _Auth_Clear)
					{
						int32_t s32Return = 0;
						s32Err = MCSSendToServerAndGetEcho(SUNDRIES_SOCKET, _MCS_Cmd_Inner_ClearWifi,
								NULL, 0, &s32Return, sizeof(int32_t));
						PRINT("s32Err: %08x, s32Return: %08x\n", s32Err, s32Return);
						if (s32Err == 0)
						{
							u32Step = _Auth_SetIP;
						}
						else
						{
							goto next;
						}
					}
					/* set ETH IP address */
					if (u32Step == _Auth_SetIP)
					{
						int32_t s32Return = 0;
						s32Err = MCSSendToServerAndGetEcho(SUNDRIES_SOCKET, _MCS_Cmd_Inner_LoadEthConfig,
								NULL, 0, &s32Return, sizeof(int32_t));
						PRINT("s32Err: %08x, s32Return: %08x\n", s32Err, s32Return);
						if (s32Err == 0)
						{
							u32Step = _Auth_Auth;
						}
						else
						{
							goto next;
						}

					}
					/* AUTH */
					if (u32Step == _Auth_Auth)
					{
						s32Err = GetInterfaceIPV4Addr(ETH_LAN_NAME, &stIPV4Addr);
						if (s32Err < 0)
						{
							PRINT("s32Err: %08x\n", s32Err);
							goto next;
						}
						PRINT("authentication via LAN\n");
						s32Err = AuthAndGetRegion(&stStat, stIPV4Addr.c8IPAddr, boIsCoordination);

						/* clear status */
						u32Step = 0;
						u32RepeatCnt = 0;
						if (!s_boIsCloudOnline)	/* some error happened, changed the interface */
						{
							boIsETH = !boIsETH;
						}
					}
				}
				else
				{
					/* clear the ETH configure */
					if (u32Step == _Auth_Clear)
					{
						int32_t s32Return = 0;
						s32Err = MCSSendToServerAndGetEcho(SUNDRIES_SOCKET, _MCS_Cmd_Inner_ClearEth,
								NULL, 0, &s32Return, sizeof(int32_t));
						PRINT("s32Err: %08x, s32Return: %08x\n", s32Err, s32Return);
						if (s32Err == 0)
						{
							u32Step = _Auth_Wifi_Connect;
						}
						else
						{
							goto next;
						}
					}

					/* connect the WIFI to the router */
					if (u32Step == _Auth_Wifi_Connect)
					{
						int32_t s32Return = 0;
						s32Err = MCSSendToServerAndGetEcho(SUNDRIES_SOCKET, _MCS_Cmd_Inner_LoadWifiConfig_Connect,
								NULL, 0, &s32Return, sizeof(int32_t));
						PRINT("s32Err: %08x, s32Return: %08x\n", s32Err, s32Return);
						if (s32Err == 0)
						{
							if (s32Return > 0)
							{
								u32Step = _Auth_SetIP;		/* the WIFI has completed to connect  */
							}
							else
							{
								u32Step = _Auth_Wifi_Check_status;
							}
						}
						else
						{
							goto next;
						}
					}
					/* check the WIFI status */
					if (u32Step == _Auth_Wifi_Check_status)
					{
						int32_t s32Return = 0;
						s32Err = MCSSendToServerAndGetEcho(SUNDRIES_SOCKET, _MCS_Cmd_Inner_GetWifiConnectStatus,
								NULL, 0, &s32Return, sizeof(int32_t));
						PRINT("s32Err: %08x, s32Return: %08x\n", s32Err, s32Return);
						if ((s32Err < 0) || (s32Return < 0))
						{
							goto next;
						}
						else
						{
							u32Step = _Auth_SetIP;
						}
					}
					/* set WIFI IP address */
					if (u32Step == _Auth_SetIP)
					{
						int32_t s32Return = 0;
						s32Err = MCSSendToServerAndGetEcho(SUNDRIES_SOCKET, _MCS_Cmd_Inner_LoadWifiConfig_SetIp,
								NULL, 0, &s32Return, sizeof(int32_t));
						PRINT("s32Err: %08x, s32Return: %08x\n", s32Err, s32Return);
						if (s32Err == 0)
						{
							u32Step = _Auth_Auth;
						}
						else
						{
							goto next;
						}
					}
					/* AUTH */
					if (u32Step == _Auth_Auth)
					{
						s32Err = GetInterfaceIPV4Addr(ETH_WIFI_NAME, &stIPV4Addr);
						if (s32Err < 0)
						{
							PRINT("s32Err: %08x\n", s32Err);
							goto next;
						}
						PRINT("authentication via Wifi\n");
						s32Err = AuthAndGetRegion(&stStat, stIPV4Addr.c8IPAddr, boIsCoordination);
						u32Step = 0;
						u32RepeatCnt = 0;
						if (!s_boIsCloudOnline)	/* some error happened, changed the interface */
						{
							boIsETH = !boIsETH;
						}
					}

				}

next:
				u32RepeatCnt++;
				if (u32RepeatCnt > 10)
				{
					u32RepeatCnt = 0;
					boIsETH = !boIsETH;
					u32Step = 0;
				}
			}
			else
			{
				SetCloudLed(HG3_LED_Cld_Gn_Lt);
			}
		}
	}while(0);
#else
	while (!g_boIsExit)
	{
		sleep(1);
		if (!s_boIsCloudOnline) /* cloud in not online, authenticate */
		{
			StIPV4Addr stIPV4Addr[4];
			uint32_t u32Cnt = 4;
			int32_t s32LanAddr = -1, s32WifiAddr = -1;
			uint32_t i;

			/* list the network */
			s32Err = GetIPV4Addr(stIPV4Addr, &u32Cnt);
			/* get the LAN address and WIFI address */
			for (i = 0; i < u32Cnt; i++)
			{
				if (strcmp(stIPV4Addr[i].c8Name, ETH_LAN_NAME) == 0)
				{
					s32LanAddr = i; /* well, the LAN is connected */
				}
				else if (strcmp(stIPV4Addr[i].c8Name, ETH_WIFI_NAME) == 0)
				{
					s32WifiAddr = i; /* WIFI is connected */
				}
			}

			/* first, we authentication ourself via LAN */
			if (s32LanAddr != -1)
			{
				PRINT("authentication via LAN\n");
				s32Err = AuthAndGetRegion(&stStat, stIPV4Addr[s32LanAddr].c8IPAddr, boIsCoordination);
			}
			/* if the cannot authentication via LAN, I will try via WIFI */
			if ((!s_boIsCloudOnline) && (s32WifiAddr != -1))
			{
				PRINT("authentication via WIFI\n");
				s32Err = AuthAndGetRegion(&stStat, stIPV4Addr[s32WifiAddr].c8IPAddr, boIsCoordination);
			}
		}
	}
#endif
	{
	int32_t i;
	for (i = 0; i < CUE_THREAD_CNT; i++)
	{
		pthread_join(s32TidCloud[i], NULL);
	}
	}
#if defined _DEBUG
	CloudTombDestroy(s_s32CloudHandle);
#else
	CloudDestroy(s_s32CloudHandle);
#endif
	SetCloudLed(HG3_LED_Cld_Rd_Lt);
	return NULL;
}
