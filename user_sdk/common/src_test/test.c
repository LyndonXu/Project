/****************************************************************************
 * Copyright(c), 2001-2060, ******************************* 版权所有
 ****************************************************************************
 * 文件名称             : test.c
 * 版本                 : 0.0.1
 * 作者                 : 许龙杰
 * 创建日期             : 2014年5月7日
 * 描述                 : 
 ****************************************************************************/
#include <common.h>
#include "common_define.h"

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
//#define JSON_TEST
//#define STAT_TEST
//#define RUN_A_PROCESS_TEST
//#define UPDATE_COPY_FILE_TEST
//#define CRC32_FILE_TEST
//#define CRC32_TABLE_TEST
//#define KEEPALIVE_TEST
//#define UPDKA_TEST
//#define DB_TEST
//#define MMAP_TEST
//#define SEND_FD_TEST
//#define AUTH_TEST
//#define XXTEA_TEST
//#define HTTPS_TEST
//#define HTTPS_POLL_TEST
//#define GET_HOST_IP_TEST
//#define TMPFILE_TEST
//#define PROCESS_LIST_TEST
//#define TRAVERSAL_DIR_TEST
//#define LOG_FILE_TEST
//#define USAGE_TEST
//#define LOG_SERVER_TEST
//#define SLO_TEST
//#define SLO_TEST_1
//#define LOCK_TEST
//#define COMMON_TEST
//#define GET_REGION_TEST
//#define IDPS_TEST
//#define NET_SSTAT_TEST
//#define UDP_STATISTICS_TEST
//#define GET_IPV4_ADDR_TEST
//#define CLOUD_STATE_TEST
//#define SEM_TEST
//#define UPDATE_TEST
//#define UNIX_SOCKET_TEST
//#define IPC_MSG_TEST
//#define STRING_DISTANCE_TEST
//#define GETOPT_TEST
//#define HASH_TEST
//#define THREAD_TEST
//#define LOAD_CONSIG_TEST
//#define WPA_CLI_TEST
//#define MSC_SERVER_CLIENT_TEST
//#define ETH_SET_WIFI_TEST
//#define ZELLER_FORMULA_TEST
//#define M3_UPDATE_TEST
//#define NET_STATUS_TEST
//#define TIMER_TASK_TEST
//#define CLOCK_MONOTONIC_TEST
//#define PROCESS_NAME_FROM_PID_TEST
//#define GET_SYS_TIME_TEST
//#define PROCESS_IS_RUNNING_TEST
//#define DOUBLE_NUMBER_TEST
//#define LOG_MODULE_TEST
//#define CLOUD_MODULE_TEST
//#define PROCESS_STATISTICS_MODULE_TEST
//#define ADD_LINE_NUMBER
//#define GETADDRINFO_TEST
//#define MINE_TEST
//#define UART_TEST
#define CMD_WITH_CB

#if defined CMD_WITH_CB
#define UNIX_TEST		"/tmp/CMD_WITH_CB.socket"
#define SEND_FILE		"/home/lyndon/workspace/config"
#define WRITE_FILE		"test_recv.text"
char c8Buf[1 * 1024 * 1024];

int32_t WriteFileCB(void *pData, uint32_t u32Len, void *pContext)
{
	FILE *pFile = fopen(WRITE_FILE, "ab+");
	if (pFile != NULL)
	{
		fwrite(pData, 1, u32Len, pFile);
		fclose(pFile);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	SignalRegister();
	/* client */
	if (argc > 1)
	{
		int32_t s32Client = -1;
		int32_t s32SendSize = 0;
		int32_t s32Err = 0;
		FILE *pFile;
		s32Client = ClientConnect(UNIX_TEST);
		if(s32Client < 0)
		{
			PRINT("ClientConnect error: 0x%08x\n", s32Client);
			exit(-1);
		}

		pFile = fopen(SEND_FILE, "rb");
		if (pFile != NULL)
		{
			s32SendSize = fread(c8Buf, 1, 1 * 1024 * 1024, pFile);
			fclose(pFile);
		}


		s32Err = MCSSyncSend(s32Client, 2000, 12345, s32SendSize, c8Buf);
		if (s32Err != 0)
		{
			PRINT("MCSSyncSendData error: 0x%08x\n", s32Err);
		}
		close(s32Client);

	}
	else /* server */
	{
		int32_t s32Err = 0;
		int32_t s32Server = ServerListen(UNIX_TEST);

		if (s32Server < 0)
		{
			PRINT("ServerListen error: 0x%08x\n", s32Server);
			exit(-1);
		}
		while (!g_boIsExit)
		{
			fd_set stSet;
			struct timeval stTimeout;
			int32_t s32Client = -1;

			stTimeout.tv_sec = 1;
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

			if(s32Client < 0)
			{
				PRINT("ServerAccept error: 0x%08x\n", s32Client);
				ServerRemove(s32Server, UNIX_TEST);
				exit(-1);
			}
			s32Err = MCSSyncReceiveCmdWithCB(s32Client, 1234, 5000,
					WriteFileCB, NULL);
			PRINT("MCSSyncReceiveCmdWithCB error: 0x%08x\n", s32Err);
			close(s32Client);
		}
		ServerRemove(s32Server, UNIX_TEST);
	}
	return 0;
}

#elif defined UART_TEST

#include <termios.h>

int main()
{
	int32_t s32Err = 0;
	int32_t s32FDUart = open("/dev/ttyUSB0", O_RDWR);
	if (s32FDUart < 0)
	{
		PRINT("error is: %s\n", strerror(errno));
		return 0;
	}

	if ((s32Err = UARTInit(s32FDUart, B115200, 0, 8, 1, 0, 10)) < 0)
	{
		PRINT("UARTInit error: 0x%08x\n", s32Err);
	}
	else
	{
		uint8_t u8Buf[64] = { 0xAA, 0x00, 0x0C, 0x80, 0x00, 0x00, 0x02, 0x24};
		int32_t s32ReadCnt;
		write(s32FDUart, u8Buf, 8);
		tcflush(s32FDUart, TCIOFLUSH);
		sleep(1);
		s32ReadCnt = read(s32FDUart, u8Buf, 64);
		if (s32ReadCnt > 0)
		{
			int32_t i;
			for (i = 0; i < s32ReadCnt; i++)
			{
				printf("0x%02x ", u8Buf[i]);
			}
			PRINT("\nget data: %d\n", s32ReadCnt);
		}

	}
	close(s32FDUart);
	return 0;
}

#elif defined GETADDRINFO_TEST
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <linux/sockios.h>
#include <fcntl.h>
#include <sys/ioctl.h>

int main(int argc, const char *argv[])
{

	if (argc < 2)
	{
		PRINT("usage: %s <host name>\n", argv[0]);
	}
	else
	{
		struct addrinfo stFilter = {0};
		struct addrinfo *pRslt = NULL;
		const char *pHost = argv[1];
		int32_t s32Err = 0;

		stFilter.ai_family = PF_INET;
		stFilter.ai_flags = AI_ADDRCONFIG;
		stFilter.ai_socktype = SOCK_STREAM;
		PRINT("begin getaddrinfo\n");
		s32Err = getaddrinfo(pHost, NULL, &stFilter, &pRslt);
		if (s32Err == 0)
		{
			struct addrinfo *pIterator;
			for (pIterator = pRslt; pIterator != NULL; pIterator = pIterator->ai_next)
			{
				PRINT("ai_flags: 0x%08x\n", pIterator->ai_flags);
				PRINT("ai_family: 0x%08x\n", pIterator->ai_family);
				PRINT("ai_socktype: 0x%08x\n", pIterator->ai_socktype);
				PRINT("ai_protocol: 0x%08x\n", pIterator->ai_protocol);
				struct sockaddr_in *pTmp =  (struct sockaddr_in *)(pIterator->ai_addr);
				PRINT("server ip: %s\n", inet_ntoa(pTmp->sin_addr));
				PRINT("ai_canonname: %s\n\n", pIterator->ai_canonname);
			}
			freeaddrinfo(pRslt);
			return 0;
		}
		else
		{
			PRINT("getaddrinfo s32Err = %d, error: %s\n", s32Err, gai_strerror(s32Err));
			s32Err = 0 - s32Err;
			return MY_ERR(_Err_Unkown_Host + s32Err);
		}
	}
	return 0;


}


#elif defined ADD_LINE_NUMBER
int main(int argc, const char *argv[])
{
	FILE *pIn, *pOut;
	int32_t s32Cnt = 1;
	char c8Buf[4096];
	if (argc < 3)
	{
		PRINT("usage %s <file need to add line> <file I will write>\n", argv[0]);
		return -1;
	}

	pIn = fopen(argv[1], "rb");
	pOut = fopen(argv[2], "wb+");
	if ((pIn == NULL) || (pOut == NULL))
	{
		goto end;
	}

	while (feof(pIn) == 0)
	{
		char *pTmp = fgets(c8Buf, 4096, pIn);
		if (pTmp == NULL)
		{
			break;
		}
		fprintf(pOut, "% 4d ", s32Cnt++);
		fprintf(pOut, "%s", c8Buf);
	}


end:
	if (pIn != NULL)
	{
		fclose(pIn);
	}
	if (pOut != NULL)
	{
		fclose(pOut);
	}
	return 0;
}

#elif defined PROCESS_STATISTICS_MODULE_TEST

int main(int argc, const char *argv[])
{
	pthread_t s32StatPid;
	int32_t s32Tmp;
	SignalRegister();
	PRINT("my pid is: %d\n", getpid());
	if (argc <= 1)
	{
		s32Tmp = MakeThread(ThreadStat, NULL, false, &s32StatPid, false);
		if (s32Tmp != 0)
		{
			PRINT("MakeThread error: 0x%08x\n", s32Tmp);
			return -1;
		}
	}
	else
	{
		PRINT("add a new process\n");
		ProcessStatisticNew();
	}
	s32Tmp = 0;
	while (!g_boIsExit)
	{
		sleep(1);
		if ((argc > 2) && ((s32Tmp % 5) == 0))
		{
			PRINT("update this process\n");
			ProcessStatisticUpdate();
		}
		s32Tmp++;
	}
	if (argc <= 1)
	{
		pthread_join(s32StatPid, NULL);
	}
	else if (argc > 2)
	{
		ProcessStatisticDelete();
	}
	return 0;
}

#elif defined CLOUD_MODULE_TEST

int main(int argc, const char *argv[])
{
	pthread_t s32CloudPid;
	int32_t s32Err;

	system("mkdir -p "WORK_DIR);
	SignalRegister();
	SSLInit();
	s32Err = MakeThread(ThreadCloud, NULL, false, &s32CloudPid, false);

	if (s32Err != 0)
	{
		PRINT("MakeThread error: 0x%08x\n", s32Err);
		return -1;
	}

	while (!g_boIsExit)
	{
		sleep(1);
	}
	pthread_join(s32CloudPid, NULL);
	SSLDestory();
	return 0;
}


#elif defined LOG_MODULE_TEST

int main(int argc, const char *argv[])
{
	pthread_t s32LogServerPid = 0;
	int32_t s32Err;
	int32_t i;
	int32_t s32Pid = getpid();

	system("mkdir -p "LOG_DIR);
	SignalRegister();

	s32Err = MakeThread(ThreadLogServer, NULL, false, &s32LogServerPid, false);

	if (s32Err != 0)
	{
		PRINT("MakeThread error: 0x%08x\n", s32Err);
		return -1;
	}

	if (argc < 2)
	{
		sleep(2);	/* wait the log server to setup */
		for (i = 0; i < 10; i++)
		{
			int32_t j;
			for (j = 0; j < 24; j++)
			{
				int32_t k;
				for (k = 0; k < 60; k++)
				{
					int32_t m;
					for (m = 0; m < 6; m++)
					{
						/* 10s, a message */
						char c8Buf[128];
						int32_t s32Client;
						c8Buf[0] = 0;
						sprintf(c8Buf, LOG_DATE_FORMAT"[%s][%d] %s\n",
							2014, 5, i + 1,
							j, k, m * 10, rand() % 1000,
							PROJECT_NAME,
							s32Pid,
							"Test OOOOOOOOOOOOOOOOOOOOOOOOOOOOK!");
						s32Client = ClientConnect(LOG_SOCKET);
						if (s32Client < 0)
						{
							PRINT("ClientConnect error: 0x%08x\n", s32Client);
							goto end;
						}

						s32Err = MCSSyncSendData(s32Client, 1000, strlen(c8Buf), c8Buf);
						close(s32Client);
					}
				}

			}
		}
		sleep(2);	/* wait the log server to receive all the message */
	}
end:
	g_boIsExit = true;
	pthread_join(s32LogServerPid, NULL);

	return 0;
}

#elif defined DOUBLE_NUMBER_TEST

int main(int argc, const char *argv[])
{
	uint32_t u32Cnt = 0, j;
	uint32_t u32Buf[4096];
	double i, d64Max;

	d64Max = 255.0 / 10.0;
	for (i = 0.2; i < (10.0 + 0.000000001); i += 0.1)
	{
		double tmp;
		uint32_t u32Num, u32Tmp;
		u32Num = i * d64Max;
		do
		{
			tmp = u32Tmp = u32Num;
			if (((tmp / d64Max) < (i + 0.000000001)) && ((tmp / d64Max) > (i- 0.000000001)))
			{
				break;
			}
			u32Num++;
			tmp = u32Num;
			tmp /= d64Max;
		}while(tmp < (i + 0.05));
		PRINT("number for %0.1f is %d\n", i, u32Tmp);
		u32Buf[u32Cnt++] = u32Tmp;
	}
	PRINT("u32Cnt: %d\n", u32Cnt);
	printf("const u8 c_u8SwitchTime[%d] = \n{\n\t", u32Cnt);
	for (j = 0; j < u32Cnt; j++)
	{
		printf("0x%02X, ", u32Buf[j]);
		if ((j & 0x07) == 0x07)
		{
			printf("\n\t");
		}
	}
	printf("\n}\n");

	u32Cnt = 0;
	d64Max = 255.0 / 30.0;
	for (i = 1.0; i < (30.0 + 0.000000001); i += 1.0)
	{
		double tmp;
		uint32_t u32Num, u32Tmp;
		u32Num = i * d64Max;
		do
		{
			tmp = u32Tmp = u32Num;
			if (((tmp / d64Max) < (i + 0.000000001)) && ((tmp / d64Max) > (i- 0.000000001)))
			{
				break;
			}
			u32Num++;
			tmp = u32Num;
			tmp /= d64Max;
		}while(tmp < (i + 0.5));
		PRINT("number for %0.1f is %d\n", i, u32Tmp);
		u32Buf[u32Cnt++] = u32Tmp;
	}
	PRINT("u32Cnt: %d\n", u32Cnt);
	printf("const u8 c_u8DelayTime[%d] = \n{\n\t", u32Cnt);
	for (j = 0; j < u32Cnt; j++)
	{
		printf("0x%02X, ", u32Buf[j]);
		if ((j & 0x07) == 0x07)
		{
			printf("\n\t");
		}
	}
	printf("\n}\n");
	return 0;
}


#elif defined PROCESS_IS_RUNNING_TEST
int main(int argc, const char *argv[])
{
	if (argc == 1)
	{
		if(AlreadyRunningUsingLockFile(LOG_LOCK_FILE))
		{
			PRINT("the process is running\n");
			return 0;
		}
		PRINT("press enter key to exit\n");
		getchar();
	}
	else
	{
		int32_t s32Err = AlreadyRunningUsingName(argv[1]);
		PRINT("%s running status: 0x%08x(%d)\n", argv[1], s32Err, s32Err);
	}
	return 0;
}


#elif defined GET_SYS_TIME_TEST

int main(int argc, const char *argv[])
{
	system("date +%s\\ %N");
	PRINT("\n%lld\n", TimeGetTime());

	system("cat /proc/uptime");
	PRINT("\n%lld\n", TimeGetSetupTime());
	return 0;
}


#elif defined PROCESS_NAME_FROM_PID_TEST

int main(int argc, const char *argv[])
{
	int32_t s32ID;
	char c8Name[_POSIX_PATH_MAX];
	int32_t s32Err;

	if (argc != 2)
	{
		PRINT("usage: %s pid of a process\n", argv[0]);
		return -1;
	}
	s32ID = atoi(argv[1]);

	s32Err = GetProcessNameFromPID(c8Name, _POSIX_PATH_MAX, s32ID);
	if (s32Err < 0)
	{
		PRINT("error: 0x%08x\n", s32Err);
	}
	else
	{
		PRINT("the name of %d is: %s\n", s32ID, c8Name);
	}

	return 0;
}

#elif defined CLOCK_MONOTONIC_TEST
int main()
{
	struct timespec stTime;

	while(1)
	{
		clock_gettime(CLOCK_MONOTONIC, &stTime);
		printf("%d, %d\n", (int32_t)stTime.tv_sec, stTime.tv_nsec / 1000000);
	}

	return 0;
}

#elif defined TIMER_TASK_TEST

int32_t FunTest(void *pArg)
{
	PRINT("**************%s(%d)(%lld)****************\n", __FUNCTION__, (int32_t)pArg, TimeGetSetupTime());
	return -1;
}

int main()
{
	int s32Cnt = 0;
	int s32Handle = TimeTaskInit("time.task", 40, NULL);

	signal(SIGINT, ProcessStop);
	signal(SIGTERM, ProcessStop);

	sleep(2);
	for (s32Cnt = 0; s32Cnt < 16; s32Cnt++)
	{
		TimeTaskAddATask(s32Handle, FunTest, (void *)s32Cnt, 25 * (s32Cnt + 1), _Time_Task_Periodic);
	}
	while (!g_boIsExit)
	{
		sleep(1);
	}
	TimeTaskDestory(s32Handle);
	return 0;
}


#elif defined NET_STATUS_TEST


int main()
{

	PRINT("status: %08x\n", GetNetLinkStatus(ETH_LAN_NAME));

	return 0;
}

#elif defined M3_UPDATE_TEST

#define CYCLE_BUF_LENGTH		(4096 * 2)
#define READ_BUF_LENGTH			(4096)
typedef struct _tagStCycleBuf
{
	char *pBuf;
	uint32_t u32TotalLength;
	uint32_t u32Write;
	uint32_t u32Read;
	uint32_t u32Using;
}StCycleBuf;

/*inline uint32_t u32GetRemainLength(StCycleBuf *pCycleBuf)
{

}*/

int32_t CheckSUPCheckSum(StSUP *pSup)
{
	uint8_t u8CheckSum = 0;
	uint8_t *pTmp;
	uint32_t u32DataLength = 0;
	uint32_t i;
	if (pSup == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	u32DataLength = pSup->u8DataLengthMSB;
	u32DataLength <<= 8;
	u32DataLength += pSup->u8DataLengthLSB;
	pTmp = (uint8_t *)pSup;
	pTmp += 2;
	for (i = 0; i < u32DataLength + 2; i++)
	{
		u8CheckSum += pTmp[i];
	}

	if (u8CheckSum == pTmp[i])
	{
		return 0;
	}
	else
	{
		pTmp[i] = u8CheckSum;
		return MY_ERR(_Err_Common);
	}

}

void *SUPGetOneMsg(StCycleBuf *pCycleBuf, const char *pData, uint32_t u32DataLength, uint32_t *pLength, int32_t *pErr)
{
	char *pBuf = NULL;
	int32_t s32Err = 0;
#if 0
	if ((pCycleBuf == NULL) || (pData == NULL) || (pLength == NULL))
	{
		s32Err = MY_ERR(_Err_InvalidParam);
		goto end;
	}
	if (((pCycleBuf->u32TotalLength - pCycleBuf->u32Using) < u32DataLength)
			|| (u32DataLength == 0))
	{
		PRINT("data too long\n");
		s32Err = MY_ERR(_Err_InvalidParam);
		goto end;
	}
	do
	{
		uint32_t u32Tmp = pCycleBuf->u32Write + u32DataLength;
		if (u32Tmp > pCycleBuf->u32TotalLength)
		{
			uint32_t u32CopyLength = pCycleBuf->u32TotalLength - pCycleBuf->u32Write;
			memcpy(pCycleBuf->pBuf + pCycleBuf->u32Write, pData, u32CopyLength);
			memcpy(pCycleBuf->pBuf, pData + u32CopyLength, u32DataLength - u32CopyLength);
			pCycleBuf->u32Write = u32DataLength - u32CopyLength;
		}
		else
		{
			memcpy(pCycleBuf->pBuf + pCycleBuf->u32Write, pData, u32DataLength);
			pCycleBuf->u32Write += u32DataLength;
		}
		pCycleBuf->u32Using += u32DataLength;
	}while(0);		/* copy data */
#else
	if ((pCycleBuf == NULL) || (pLength == NULL))
	{
		s32Err = MY_ERR(_Err_InvalidParam);
		goto end;
	}
	if (((pCycleBuf->u32TotalLength - pCycleBuf->u32Using) < u32DataLength)
			|| (u32DataLength == 0))
	{
		PRINT("data too long\n");
		s32Err = MY_ERR(_Err_InvalidParam);
	}
	if (u32DataLength != 0)
	{
		if (pData == NULL)
		{
			s32Err = MY_ERR(_Err_InvalidParam);
			goto end;
		}
		else	/* copy data */
		{
			uint32_t u32Tmp = pCycleBuf->u32Write + u32DataLength;
			if (u32Tmp > pCycleBuf->u32TotalLength)
			{
				uint32_t u32CopyLength = pCycleBuf->u32TotalLength - pCycleBuf->u32Write;
				memcpy(pCycleBuf->pBuf + pCycleBuf->u32Write, pData, u32CopyLength);
				memcpy(pCycleBuf->pBuf, pData + u32CopyLength, u32DataLength - u32CopyLength);
				pCycleBuf->u32Write = u32DataLength - u32CopyLength;
			}
			else
			{
				memcpy(pCycleBuf->pBuf + pCycleBuf->u32Write, pData, u32DataLength);
				pCycleBuf->u32Write += u32DataLength;
			}
			pCycleBuf->u32Using += u32DataLength;

		}
	}
#endif

	do
	{
		uint32_t i;
		bool boIsBreak = false;

		for (i = 0; i < pCycleBuf->u32Using; i++)
		{
			uint32_t u32ReadIndex = i + pCycleBuf->u32Read;
			u32ReadIndex %= pCycleBuf->u32TotalLength;
			if (pCycleBuf->pBuf[u32ReadIndex] == ((char)0xF5))
			{
				uint32_t u32MSB = pCycleBuf->pBuf[(u32ReadIndex + offsetof(StSUP, u8DataLengthMSB)) % pCycleBuf->u32TotalLength];
				uint32_t u32LSB = pCycleBuf->pBuf[(u32ReadIndex + offsetof(StSUP, u8DataLengthLSB)) % pCycleBuf->u32TotalLength];
				u32MSB &= 0xFF;
				u32LSB &= 0xFF;
				u32MSB = (u32MSB << 8) + u32LSB;
				PRINT("the data length is: %d\n", u32MSB);
				if (u32MSB > (pCycleBuf->u32TotalLength / 2))	/* maybe the message is wrong */
				{
					pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + 1));
					pCycleBuf->u32Read += (i + 1);
					pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
				}
				else if (((int32_t)(u32MSB)) < ((int32_t)((pCycleBuf->u32Using - (i + 1) - SUP_LENGTH_BYTES_CNT)))) /* good, I may got a message */
				{
					char c8CheckSum = 0, *pBufTmp, c8Tmp;
					uint32_t j, u32Start, u32End;
					uint32_t u32CmdLength = u32MSB + offsetof(StSUP, u8UUID) + 1;
					pBuf = malloc(u32CmdLength);
					if (pBuf == NULL)
					{
						s32Err = MY_ERR(_Err_Mem); /* big problem */
						goto end;
					}
					pBufTmp = pBuf + offsetof(StSUP, u8DataLengthMSB);
					u32Start = i + pCycleBuf->u32Read + offsetof(StSUP, u8DataLengthMSB);
/*
					u32End = u32MSB + SUP_LENGTH_BYTES_CNT + u32Start;
							= u32MSB + offsetof(StSUP, u8UUID) - offsetof(StSUP, u8DataLengthMSB) + u32Start;
							= u32MSB + offsetof(StSUP, u8UUID) - offsetof(StSUP, u8DataLengthMSB) + u32ReadIndex + offsetof(StSUP, u8DataLengthMSB);
							= u32MSB + offsetof(StSUP, u8UUID) + u32ReadIndex;
*/
					u32End = u32MSB + offsetof(StSUP, u8UUID) + i + pCycleBuf->u32Read;
					PRINT("start: %d, end: %d\n", u32Start, u32End);
					for (j = u32Start; j < u32End; j++)
					{
						c8Tmp = pCycleBuf->pBuf[j % pCycleBuf->u32TotalLength];
						c8CheckSum += c8Tmp;
						*pBufTmp++ = c8Tmp;
					}
					c8Tmp = pCycleBuf->pBuf[u32End % pCycleBuf->u32TotalLength];
					if (c8CheckSum == c8Tmp) /* good message */
					{
						boIsBreak = true;
						pBuf[0] = pCycleBuf->pBuf[u32ReadIndex];
						pBuf[1] = pCycleBuf->pBuf[(u32ReadIndex + 1) % pCycleBuf->u32TotalLength];
						*pBufTmp = c8Tmp;

						pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + u32CmdLength));
						pCycleBuf->u32Read = i + pCycleBuf->u32Read + u32CmdLength;
						pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
						PRINT("get a command: %d\n", u32MSB);
						PRINT("u32Using: %d, u32Read: %d, u32Write: %d\n", pCycleBuf->u32Using, pCycleBuf->u32Read, pCycleBuf->u32Write);
						*pLength = u32CmdLength;
					}
					else
					{
						free(pBuf);
						pBuf = NULL;
						PRINT("get a wrong command: %d\n", u32MSB);
						pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + 1));
						pCycleBuf->u32Read += (i + 1);
						pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
					}
				}
				else	/* message not enough long */
				{
					pCycleBuf->u32Using = (pCycleBuf->u32Using - i);
					pCycleBuf->u32Read += i;
					pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
					boIsBreak = true;
				}
				break;
			}
		}
		if ((i == pCycleBuf->u32Using) && (!boIsBreak))
		{
			PRINT("cannot find F5, i = %d\n", pCycleBuf->u32Using);
			pCycleBuf->u32Using = 0;
			pCycleBuf->u32Read += i;
			pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
		}

		if (boIsBreak)
		{
			break;
		}
	}while(((int32_t)pCycleBuf->u32Using) > 0);

	if (pCycleBuf->u32Write + u32DataLength)

end:
	if (pErr != NULL)
	{
		*pErr = s32Err;
	}
	return pBuf;
}

#if 1

#define SERVER_SOCKET	"/tmp/tmp.socket"

enum
{
	_M3_Update_GetDivceInfo,
	_M3_Update_BeginUpdate,
	_M3_Update_ServerEcho,



	_M3_Update_Reserved,
};

void *ThreadRead(void *pArg)
{
	int32_t s32Device = (int32_t)pArg;
	int32_t s32Err;
	char c8Buf[4096];
	StCycleBuf stBuf = {0};

	stBuf.pBuf = malloc(CYCLE_BUF_LENGTH);
	if (stBuf.pBuf == NULL)
	{
		return NULL;
	}
	stBuf.u32TotalLength = CYCLE_BUF_LENGTH;

	while (!g_boIsExit)
	{
		char *pBuf = NULL;
		uint32_t u32Length = 0;
		int32_t s32Cnt = 0;
		int32_t s32Client = 0;

		s32Cnt = read(s32Device, c8Buf, 4096);

		if (s32Cnt <= 0)
		{
			continue;
		}
		do
		{
			int32_t i;
			PRINT("get some data from M3:");
			for (i = 0; i < s32Cnt; i++)
			{
				if ((i & 0x0F) == 0)
				{
					printf("\n");
				}
				printf("%02hhX ", c8Buf[i]);
			}
			printf("\n");

		}while(0);


		while(1)
		{
			pBuf = SUPGetOneMsg(&stBuf, c8Buf, s32Cnt, &u32Length, &s32Err);
			if (pBuf == NULL)
			{
				break;
			}

			s32Client = ClientConnect(SERVER_SOCKET);
			if (s32Client < 0)
			{
				PRINT("ClientConnect %s error: 0x%08x\n", SERVER_SOCKET, s32Client);
				free(pBuf);
			}

			s32Err = MCSSyncSendData(s32Client, 1000, sizeof(pBuf), &pBuf);
			if (s32Err < 0)
			{
				PRINT("MCSSyncSendData error: 0x%08x\n", s32Err);
				free(pBuf);
			}
			close(s32Client);

			s32Cnt = 0;
		}
	}


	return NULL;

}

int main(int argc, const char *argv[])
{
	int32_t s32Device = -1, s32Update = -1;
	int32_t s32Server = -1;
	pthread_t s32ReadThreadId = 0;
	int32_t s32Err;
	int32_t s32UpdateStatus = _M3_Update_GetDivceInfo;
	int32_t s32RepeatCnt = 0;
	char c8Buf[4096];
	StSUP *pSUP = (StSUP *)c8Buf;
	uint32_t u32M3SoftVersoin = 0;
	uint32_t u32UpdateFileCRC32 = ~0;
	uint32_t u32UpdateFileLength = 0;
	uint32_t u32DataLength = sizeof(StSUP);

	if (argc < 3)
	{
		printf("%s device_name update_file\n", argv[0]);
		return 0;
	}

	s32Device = open(argv[1], O_RDWR);
	if (s32Device < 0)
	{
		PRINT("open %s error: %s\n", argv[1], strerror(errno));
		goto end;
	}

	s32Update = open(argv[2], O_RDONLY);
	if (s32Update < 0)
	{
		PRINT("open %s error: %s\n", argv[2], strerror(errno));
		goto end;
	}
	u32UpdateFileLength = lseek(s32Update, 0, SEEK_END);
	if (u32UpdateFileLength == ~0)
	{
		u32UpdateFileLength = 0;
	}
	CRC32File(argv[2], &u32UpdateFileCRC32);


	s32Server = ServerListen(SERVER_SOCKET);
	if (s32Server < 0)
	{

		PRINT("ServerListen error: 0x%08x\n", s32Server);
		goto end;
	}

	s32Err = MakeThread(ThreadRead, (void *)s32Device, false, &s32ReadThreadId, false);
	if (s32Err < 0)
	{
		PRINT("make thread error: 0x%08x\n", s32Err);
		goto end;
	}
	pSUP->u8CommandHead = 0xF5;
	pSUP->u8PortNum = 0;
	pSUP->u8DataLengthMSB = 0;
	pSUP->u8DataLengthLSB = 0x07;
	memset(pSUP->u8UUID, 0, sizeof(pSUP->u8UUID));
	pSUP->u8CommandID = 0xA0;
	CheckSUPCheckSum(pSUP);
	write(s32Device, c8Buf, sizeof(StSUP) + 1);

	while (!g_boIsExit)
	{
		fd_set stSet;
		struct timeval stTimeout;

		int32_t s32Client = -1;
		stTimeout.tv_sec = 5;
		stTimeout.tv_usec = 0;
		FD_ZERO(&stSet);
		FD_SET(s32Server, &stSet);

		if (select(s32Server + 1, &stSet, NULL, NULL, &stTimeout) <= 0)
		{
			switch (s32UpdateStatus)
			{
				case _M3_Update_GetDivceInfo:
				{
					pSUP->u8CommandHead = 0xF5;
					pSUP->u8PortNum = 0;
					pSUP->u8DataLengthMSB = 0;
					pSUP->u8DataLengthLSB = 0x07;
					memset(pSUP->u8UUID, 0, sizeof(pSUP->u8UUID));
					pSUP->u8CommandID = 0xA0;
					CheckSUPCheckSum(pSUP);
					write(s32Device, c8Buf, sizeof(StSUP) + 1);
					break;
				}
				case _M3_Update_BeginUpdate:
				{
					if (s32RepeatCnt > 3)
					{
						s32RepeatCnt = 0;
						s32UpdateStatus = _M3_Update_GetDivceInfo;
						PRINT("begin update command has no echo\n");
						break;
					}
					pSUP->u8CommandHead = 0xF5;
					pSUP->u8PortNum = 0;
					pSUP->u8DataLengthMSB = 0;
					pSUP->u8DataLengthLSB = 0x07;
					memset(pSUP->u8UUID, 0, sizeof(pSUP->u8UUID));
					pSUP->u8CommandID = 0xA1;
					CheckSUPCheckSum(pSUP);
					write(s32Device, c8Buf, sizeof(StSUP) + 1);
					s32RepeatCnt++;
					break;
				}
				default:
				{
					if (u32DataLength != 0)
					{
						write(s32Device, c8Buf, u32DataLength + 1);
						s32RepeatCnt++;
						if (s32RepeatCnt >= 3)
						{
							s32RepeatCnt = 0;
							u32DataLength = 0;
							//s32UpdateStatus = _M3_Update_GetDivceInfo;
							PRINT("the M3 is in update status, but I cannot get more command from M3\n");
						}
					}
					break;
				}
			}
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
				StSUP *pM3SUP = NULL;
				char *pM3Buf;
				memcpy(&pM3SUP, pMCSStream, sizeof(StSUP *));
				pM3Buf = (char *)(pM3SUP);
				do
				{
					uint32_t u32Tmp = pM3SUP->u8DataLengthMSB;
					u32Tmp <<= 8;
					u32Tmp += pM3SUP->u8DataLengthLSB;
					PRINT("Command header: %02hhX\n", pM3SUP->u8CommandHead);
					PRINT("Port number: %02hhX\n", pM3SUP->u8PortNum);
					PRINT("MSB: %02hhX\n", pM3SUP->u8DataLengthMSB);
					PRINT("LSB: %02hhX\n", pM3SUP->u8DataLengthLSB);
					PRINT("UID: %02hhX %02hhX %02hhX %02hhX %02hhX %02hhX\n",
							pM3SUP->u8UUID[0], pM3SUP->u8UUID[1], pM3SUP->u8UUID[2],
							pM3SUP->u8UUID[3], pM3SUP->u8UUID[4], pM3SUP->u8UUID[5]);
					PRINT("Command ID: %02hhX\n", pM3SUP->u8CommandID);

					if (u32Tmp >= 0x07)
					{
						uint32_t i = 0;
						char *pTmp = pM3Buf + sizeof(StSUP);
						PRINT("Data is:");

						for (i = 0; i < u32Tmp - 0x07; i++)
						{
							if ((i & 0x0F) == 0)
							{
								printf("\n");
							}
							printf("%02hhX ", pTmp[i]);
						}
						printf("\n");
					}
				}while(0);

				s32RepeatCnt = 0;

				switch (pM3SUP->u8CommandID)
				{
					case 0xA0:
					{
						uint8_t *pTmp = (uint8_t *)pM3Buf + sizeof(StSUP) + 3 + 16;
						if (pTmp[3 + 3] != 0)
						{
							PRINT("M3 does not want to update\n");
							break;
						}
						u32M3SoftVersoin = pTmp[0];
						u32M3SoftVersoin <<= 8;
						u32M3SoftVersoin += pTmp[1];
						u32M3SoftVersoin <<= 8;
						u32M3SoftVersoin += pTmp[2];
						PRINT("M3 softer version: %06X\n", u32M3SoftVersoin);

						s32UpdateStatus = _M3_Update_BeginUpdate;
						pSUP->u8CommandHead = 0xF5;
						pSUP->u8PortNum = 0;
						pSUP->u8DataLengthMSB = 0;
						pSUP->u8DataLengthLSB = 0x07;
						memset(pSUP->u8UUID, 0, sizeof(pSUP->u8UUID));
						pSUP->u8CommandID = 0xA1;
						CheckSUPCheckSum(pSUP);
						write(s32Device, c8Buf, sizeof(StSUP) + 1);
						break;
					}
					case 0xA1:
					{
						s32UpdateStatus = _M3_Update_ServerEcho;
						break;
					}
					case 0xA2:
					{
						uint8_t *pTmp = (uint8_t *)pSUP + sizeof(StSUP);
						uint32_t u32Tmp;

						u32DataLength = sizeof(StSUP);

						u32Tmp = u32M3SoftVersoin + 1;
						pTmp[0] = (u32Tmp >> 16) & 0xFF;
						pTmp[1] = (u32Tmp >> 8) & 0xFF;
						pTmp[2] = (u32Tmp) & 0xFF;
						u32DataLength += 3;

						u32Tmp = u32UpdateFileLength;
						pTmp[3] = (u32Tmp >> 16) & 0xFF;
						pTmp[4] = (u32Tmp >> 8) & 0xFF;
						pTmp[5] = (u32Tmp) & 0xFF;
						u32DataLength += 3;

						u32Tmp = u32UpdateFileCRC32;
						pTmp[6] = (u32Tmp >> 24) & 0xFF;
						pTmp[7] = (u32Tmp >> 16) & 0xFF;
						pTmp[8] = (u32Tmp >> 8) & 0xFF;
						pTmp[9] = (u32Tmp) & 0xFF;
						u32DataLength += 4;

						u32Tmp = u32DataLength - 4;
						pSUP->u8CommandHead = 0xF5;
						pSUP->u8PortNum = 0;
						pSUP->u8DataLengthMSB = (u32Tmp >> 8) & 0xFF;
						pSUP->u8DataLengthLSB = (u32Tmp) & 0xFF;
						memset(pSUP->u8UUID, 0, sizeof(pSUP->u8UUID));
						pSUP->u8CommandID = 0xA2;

						CheckSUPCheckSum(pSUP);
						write(s32Device, c8Buf, u32DataLength + 1);
						//u32DataLength = 0;
						s32UpdateStatus = _M3_Update_ServerEcho;
						break;
					}
					case 0xA3:
					{
						uint8_t *pTmp = (uint8_t *)pSUP + sizeof(StSUP);
						uint8_t *pM3Tmp = (uint8_t *)pM3Buf + sizeof(StSUP);
						uint32_t u32Tmp;
						uint32_t u32Block = pM3Tmp[0];
						uint32_t i;

						u32DataLength = sizeof(StSUP);

						u32Block <<= 8;
						u32Block += pM3Tmp[1];
						if (u32Block * 128 > u32UpdateFileLength)
						{
							PRINT("block number error\n");
							break;
						}

						lseek(s32Update, u32Block * 128, SEEK_SET);
						for (i = 0; i < 16; i++)
						{
							s32Err = read(s32Update, pTmp + 4, 128);
							if (s32Err <= 0)
							{
								break;
							}
							if (s32Err < 128)
							{
								memset(pTmp + 4 + s32Err, 0xFF, 128 - s32Err);
							}
							u32Tmp = u32Block + i;
							pTmp[0] = (u32Tmp >> 8) & 0xFF;
							pTmp[1] = (u32Tmp) & 0xFF;

							u32Tmp = CRC16(pTmp + 4, 128);
							pTmp[2] = (u32Tmp >> 8) & 0xFF;
							pTmp[3] = (u32Tmp) & 0xFF;
							pTmp += (128 + 4);
							u32DataLength += (128 + 4);
						}

						u32Tmp = u32DataLength - 4;
						pSUP->u8CommandHead = 0xF5;
						pSUP->u8PortNum = 0;
						pSUP->u8DataLengthMSB = (u32Tmp >> 8) & 0xFF;
						pSUP->u8DataLengthLSB = (u32Tmp) & 0xFF;
						memset(pSUP->u8UUID, 0, sizeof(pSUP->u8UUID));
						pSUP->u8CommandID = 0xA3;

						CheckSUPCheckSum(pSUP);
						write(s32Device, c8Buf, u32DataLength + 1);
						s32UpdateStatus = _M3_Update_ServerEcho;
						break;
					}
					case 0xA4:
					{
						uint8_t *pM3Tmp = (uint8_t *)pM3Buf + sizeof(StSUP);
						uint32_t u32Tmp;

						u32DataLength = sizeof(StSUP);

						PRINT("update: %hhd\n", pM3Tmp[0]);

						u32Tmp = u32DataLength - 4;
						pSUP->u8CommandHead = 0xF5;
						pSUP->u8PortNum = 0;
						pSUP->u8DataLengthMSB = (u32Tmp >> 8) & 0xFF;
						pSUP->u8DataLengthLSB = (u32Tmp) & 0xFF;
						memset(pSUP->u8UUID, 0, sizeof(pSUP->u8UUID));
						pSUP->u8CommandID = 0xA4;

						CheckSUPCheckSum(pSUP);
						write(s32Device, c8Buf, u32DataLength + 1);
						s32UpdateStatus = _M3_Update_GetDivceInfo;
						exit(0);
						break;
					}

					default:
					{
						PRINT("I don't known this command\n");
						break;
					}

				}
				free(pM3SUP);
				MCSSyncFree(pMCSStream);
			}
			else
			{
				PRINT("MCSSyncReceive error 0x%08x\n", s32Err);
			}
			close(s32Client);
		}
	}
end:
	if (s32ReadThreadId != 0)
	{
		pthread_join(s32ReadThreadId, NULL);
	}
	if (s32Server > 0)
	{
		ServerRemove(s32Server, SERVER_SOCKET);
	}
	if (s32Update > 0)
	{
		close(s32Update);
	}
	if (s32Device > 0)
	{
		close(s32Device);
	}
	return 0;
}
#endif

#if 0
int main(int argc, const char *pArgv[])
{
	uint32_t i;
	StCycleBuf stBuf = {0};
	char c8Buf[CYCLE_BUF_LENGTH];
	char *pBuf = NULL;

	pBuf = stBuf.pBuf = malloc(CYCLE_BUF_LENGTH);
	stBuf.u32TotalLength = CYCLE_BUF_LENGTH;

	c8Buf[0] = 0xF5;
	c8Buf[1] = 0x00;
	for (i = 0; i < 50; i++)
	{
		char c8CheckSum = 0, *pTmp;
		uint32_t j, u32CmdLen, u32GetCmdLen = 0;
		u32CmdLen = rand() % ((CYCLE_BUF_LENGTH / 2) - 6);
		u32CmdLen += 1;
		c8Buf[2] = ((u32CmdLen >> 8) & 0xFF);
		c8Buf[3] = (u32CmdLen & 0xFF);
		c8CheckSum += c8Buf[2];
		c8CheckSum += c8Buf[3];
		for (j = 0; j < u32CmdLen; j++)
		{
			c8Buf[4 + j] = rand();
			c8CheckSum += c8Buf[4 + j];
		}
		c8Buf[4 + j] = c8CheckSum;
		PRINT("(%d) insert a command length: %d\n", i, u32CmdLen);
		pTmp = SUPGetOneMsg(&stBuf, c8Buf, u32CmdLen + 5, &u32GetCmdLen, NULL);
		free(pTmp);
		pTmp = SUPGetOneMsg(&stBuf, c8Buf, u32CmdLen, &u32GetCmdLen, NULL);
		free(pTmp);
	}
	PRINT("free stBuf.pBuf\n");
	free(pBuf);
	PRINT("exit\n");
	return 0;

}
#endif

#elif defined ZELLER_FORMULA_TEST

typedef struct tm StTime;
#define DATE_END_YEAR		2039
#define DATE_START_YEAR		1970
#define DATE_BASE_YEAR		1900

const uint16_t u16YearDay[2][13] =
{
	{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365,},
	{0, 31, 60, 91, 121, 152, 182, 213, 253, 274, 305, 335, 366,}
};

static bool IsLeapYear(int32_t s32Year)
{
	if((((s32Year % 4) == 0) && ((s32Year % 100) != 0)) ||
		((s32Year % 400) == 0))
	{
		return true;
	}
	else
	{
		return false;
	}
}

static void GetWeek(StTime *pTime)
{
	/* Zeller's */
	uint32_t u32Week = 0;
	uint32_t C, Y, M, D, a;
	C = (pTime->tm_year + DATE_BASE_YEAR) / 100;
	a = (14 - (pTime->tm_mon + 1)) / 12;
	Y = (pTime->tm_year + DATE_BASE_YEAR) % 100 - a;
	M = (pTime->tm_mon + 1) + 12 * a - 2;
	D = pTime->tm_mday;
	u32Week = C / 4 - 2 * C + Y + Y / 4 + (13 * M - 1 ) / 5 + D;
	if(u32Week & 0x80000000)/* week is negative */
	{
		u32Week--;
		u32Week = ~ u32Week;
		u32Week = u32Week % 7;
		u32Week = 7 - u32Week;
	}
	else
	{
		u32Week = u32Week % 7;
	}
	pTime->tm_wday = u32Week;
}

int32_t GetDateFromSecond(uint32_t u32Second, StTime *pTime, int32_t s32TimeZone)
{
	uint32_t u32TotalDay, u32Tmp;
	uint32_t u32Year, u32Month;
	bool boIsLeap;
	if (pTime == NULL)
	{
		return -1;
	}
	u32Second = u32Second - s32TimeZone * 3600;

	u32Tmp = u32Second % 86400;
	pTime->tm_hour = u32Tmp / 3600;
	u32Tmp %= 3600;
	pTime->tm_min = u32Tmp / 60;
	pTime->tm_sec = u32Tmp % 60;

	u32TotalDay = u32Second / 86400 + 1;

	u32Year = DATE_START_YEAR;

	while(1)
	{
		boIsLeap = IsLeapYear(u32Year);
		u32Tmp = u16YearDay[boIsLeap][12];
		if(u32TotalDay > u32Tmp)
		{
			u32TotalDay -= u32Tmp;
			u32Year++;
		}
		else
		{
			break;
		}
	}

	pTime->tm_year = u32Year - DATE_BASE_YEAR;
	pTime->tm_yday = u32TotalDay;

	for (u32Month = 1; u32Month < 13; u32Month++)
	{
		if(u32TotalDay <= u16YearDay[boIsLeap][u32Month])
		{
			u32TotalDay -= u16YearDay[boIsLeap][u32Month - 1];
			break;
		}
	}

	pTime->tm_mon = u32Month - 1;
	pTime->tm_mday = u32TotalDay;

	GetWeek(pTime);

	return 0;
}
uint32_t GetSecondFromDate(StTime *pTime, int32_t s32TimeZone)
{
	uint32_t u32Second, u32Tmp;
	bool boIsLeap;
	if(pTime == NULL)
	{
		return (uint32_t)(-1);
	}

	if (pTime->tm_year > (DATE_END_YEAR - DATE_BASE_YEAR + 1))
	{
		return -1;
	}

	u32Second = 0;
	u32Tmp = DATE_START_YEAR;
	while(u32Tmp < ((uint32_t)pTime->tm_year + DATE_BASE_YEAR))
	{
		boIsLeap = IsLeapYear(u32Tmp);

		u32Second += ((uint32_t)86400 * u16YearDay[boIsLeap][12]);

		u32Tmp++;
	}
	boIsLeap = IsLeapYear(pTime->tm_year);

	/* if the pTime is valid we just can let u32Tmp = pTime->u16Yday */
	u32Tmp = u16YearDay[boIsLeap][pTime->tm_mon] + pTime->tm_mday;
	u32Tmp -= 1;

	u32Second += ((uint32_t)86400 * u32Tmp);
	u32Second += ((uint32_t)3600 * pTime->tm_hour);
	u32Second += ((uint32_t)60 * pTime->tm_min);
	u32Second += pTime->tm_sec;
	u32Second = u32Second + s32TimeZone * 3600;
	return u32Second;
}
int main()
{
	time_t s32Time = time(NULL);
	StTime stTime = {0};
	GetDateFromSecond(s32Time, &stTime, -8);

	PRINT("%s\n", ctime(&s32Time));
	PRINT("%s\n", asctime(&stTime));

	PRINT("%ld\n", s32Time);
	PRINT("%d\n", GetSecondFromDate(&stTime, -8));
	return 0;
}


#elif defined ETH_SET_WIFI_TEST

int main(int argc, const char *argv[])
{
	void *ThreadSundries(void *pArg);
	void *ThreadEthSetWifi(void *pArg);

	pthread_t s32ThreadId = 0;

	signal(SIGINT, ProcessStop);
	signal(SIGTERM, ProcessStop);

	MakeThread(ThreadSundries, NULL, false, &s32ThreadId, false);

	ThreadEthSetWifi(NULL);

	pthread_join(s32ThreadId, NULL);

	return 0;
}

#elif defined MSC_SERVER_CLIENT_TEST
#include "common_define.h"
#include <common.h>
int main(int argc, const char *argv[])
{
	signal(SIGINT, ProcessStop);
	signal(SIGTERM, ProcessStop);
	if (argc > 1)
	{
		void *ThreadSundries(void *pArg);
		ThreadSundries(NULL);
	}
	else
	{
#if 0
		StMCIWIFISSIDAndPSK stNULL = {NULL, NULL};
		int32_t s32Return = 0;
		int32_t s32Err = 0;
		s32Err = MCSSendToSererverAndGetEcho(SUNDRIES_SOCKET, _MCS_Cmd_Inner_SetWIFISSIDAndPSK,
				&stNULL, sizeof(StMCIWIFISSIDAndPSK), &s32Return, sizeof(int32_t));
#else
		int32_t s32Return = 0;
		int32_t s32Err = 0;
		s32Err = MCSSendToSererverAndGetEcho(SUNDRIES_SOCKET, _MCS_Cmd_Inner_GetWIFIConnectStatus,
				NULL, 0, &s32Return, sizeof(int32_t));

#endif
		PRINT("s32Err = 0x%08x, s32Return = 0x%08x\n", s32Err, s32Return);

	}
	return 0;
}
#elif defined WPA_CLI_TEST
int main(int argc, const char *argv[])
{
	int32_t WpaClientTest(int argc, const char *argv[]);

	return WpaClientTest(argc, argv);
};
#elif defined LOAD_CONSIG_TEST

int main()
{
	return 0;
}

#elif !HAS_CROSS && defined TEST_MQUEUE
#include "common_define.h"
#include <common.h>

#include <mqueue.h>

int main()
{
	int32_t s32Err = 0;
	struct timespec stTime;
	struct mq_attr stAttr;
	mq_unlink("/mymq");
	mqd_t mqdMsgQueue = mq_open("/mymq", O_CREAT | O_RDWR, 0777, NULL);
	if (mqdMsgQueue < 0)
	{
		PRINT("mq_open error: %s\n", strerror(errno));
		return 0;
	}
	mq_getattr(mqdMsgQueue, &stAttr);
	stTime.tv_sec = time(NULL) + 1;
	stTime.tv_nsec = 0;
	s32Err = mq_timedsend(mqdMsgQueue, "Hello world\n", strlen("Hello world\n") + 1, 0, &stTime);
	if (s32Err < 0)
	{
		PRINT("mq_timedsend error: %s\n", strerror(errno));
	}
	do
	{
		char c8Buf[stAttr.mq_msgsize];
		stTime.tv_sec = time(NULL) + 2;
		s32Err = mq_timedreceive(mqdMsgQueue, c8Buf, stAttr.mq_msgsize, NULL, &stTime);
		if (s32Err < 0)
		{
			PRINT("mq_timedreceive error: %s\n", strerror(errno));
		}
		else
		{
			PRINT("mq_timedreceive: %s(%d)\n", c8Buf, s32Err);
		}
	}while(s32Err > 0);
	getchar();

	mq_close(mqdMsgQueue);
	mq_unlink("/mymq");
	return 0;
}


#elif defined THREAD_TEST

#include "common_define.h"
#include <common.h>

void *ThreadFun(void *pArg)
{
	while(1);
	return NULL;
}

int main()
{
	int32_t s32Err = 0;
    pthread_attr_t stAttr;
    size_t s32StackSize = 0;;

    s32Err = pthread_attr_init(&stAttr);
    if (s32Err != 0)
    {
        return MY_ERR(_Err_SYS + s32Err);
    }
    pthread_attr_getstacksize(&stAttr, &s32StackSize);
    PRINT("s32StackSize: %d\n", s32StackSize);
    pthread_attr_destroy(&stAttr);
	return 0;
}


#elif defined HASH_TEST
typedef struct _tagStProcessStatisticInfo
{
	StSYSInfo stSYSInfo;				/* 当前的系统状态 */
	StProcessStat stThreadStat;			/* 进程/线程的当前状态 */
	StProcessInfo stThreadInfo;			/* 保存进程名字等一些信息 */
	StHashHandle *pHandle;				/* 哈希表的句柄 */
	UnProcessInfo *pProcessInfo;		/* 在统计结束后, 检查各个进程的状态时使用*/
}StProcessStatisticInfo;


const char *pProcessName[6] =
{
	"main_proc",
	"upgrade",
	"communication",
	"firefox",
	"test_upgrade",
	"test",
};

/* the callback of ProcessStatisticUpdateAThread called by TraversalDirThreadCallback */
/*
1, 采样两个足够短的时间间隔的Cpu快照，分别记作t1,t2，其中t1、t2的结构均为：
	(user, nice, system, idle, iowait, irq, softirq, stealstolen, guest)的9元组;
2,   计算总的Cpu时间片totalCpuTime
	a)         把第一次的所有cpu使用情况求和，得到s1;
	b)         把第二次的所有cpu使用情况求和，得到s2;
	c)         s2 - s1得到这个时间间隔内的所有时间片，即totalCpuTime = j2 - j1 ;
3, 计算空闲时间idle
	idle对应第四列的数据，用第二次的第四列 - 第一次的第四列即可
	idle=第二次的第四列 - 第一次的第四列
4, 计算cpu使用率
	pcpu =100* (total-idle)/total
*/
static int32_t UpdateAThreadCallback(int32_t s32Flag, const StHashCursor* pCursor, void *pData, void *pContext)
{
	StProcessStatisticInfo *pOutsideInfo = (StProcessStatisticInfo *)pContext;
	StProcessInfo *pThreadInfo = &(((UnProcessInfo *)pData)->stThreadInfo);
	StProcessStat *pThreadStat = &(pOutsideInfo->stThreadStat);

	pThreadInfo->u32StatisticTimes = pOutsideInfo->stThreadInfo.u32StatisticTimes;
	if (s32Flag == _Hash_Traversal_Thread_New)
	{
		memcpy(pThreadInfo->c8Name, pOutsideInfo->stThreadInfo.c8Name, 64);
		pThreadInfo->u32Pid = pOutsideInfo->stThreadInfo.u32Pid;
		pThreadInfo->u32SysTime = pThreadStat->u32Stime;
		pThreadInfo->u32UserTime = pThreadStat->u32Utime;
		pThreadInfo->u32RSS = pThreadStat->s32Rss;
		pThreadInfo->u16CPUUsage = 0;
		pThreadInfo->u16CPUAverageUsage = 0;
		pThreadInfo->u16MemUsage = 0;
		pThreadInfo->u16MemAverageUsage = 0;
	}
	else
	{
		StSYSInfo *pSYSInfo = &(pOutsideInfo->stSYSInfo);
		uint64_t u64Tmp2 = pThreadInfo->u32SysTime + pThreadInfo->u32UserTime;
		uint64_t u64Tmp;
		pThreadInfo->u32SysTime = pThreadStat->u32Stime;
		pThreadInfo->u32UserTime = pThreadStat->u32Utime;

		u64Tmp = pThreadInfo->u32SysTime + pThreadInfo->u32UserTime;

		u64Tmp = (u64Tmp - u64Tmp2) * 10000 /
				(pSYSInfo->stCPU.u64Total - pSYSInfo->stCPU.u64PrevTotal);
		u64Tmp *= sysconf(_SC_NPROCESSORS_ONLN);
		pThreadInfo->u16CPUUsage = u64Tmp;
		u64Tmp2 = pThreadInfo->u16CPUAverageUsage;
		u64Tmp = u64Tmp2 * (100 - AVERAGE_WEIGHT) + u64Tmp * AVERAGE_WEIGHT;
		u64Tmp /= 100;
		pThreadInfo->u16CPUAverageUsage = u64Tmp;

		u64Tmp = pThreadInfo->u32RSS = pThreadStat->s32Rss;
		u64Tmp = ((u64Tmp / 1024 ) * 10000) / pSYSInfo->stMem.u32MemTotal;
		pThreadInfo->u16MemUsage = u64Tmp;

		u64Tmp2 = pThreadInfo->u16MemAverageUsage;
		u64Tmp = u64Tmp2 * (100 - AVERAGE_WEIGHT) + u64Tmp * AVERAGE_WEIGHT;
		u64Tmp /= 100;
		pThreadInfo->u16MemAverageUsage = u64Tmp;

		PRINT("Process(%s_%d) CPU: %d.%02d, %d.%02d \tMem: %d.%02d, %d.%02d\n",
				pThreadInfo->c8Name,
				pThreadInfo->u32Pid,
				pThreadInfo->u16CPUUsage / 100, pThreadInfo->u16CPUUsage % 100,
				pThreadInfo->u16CPUAverageUsage / 100, pThreadInfo->u16CPUAverageUsage % 100,
				pThreadInfo->u16MemUsage / 100, pThreadInfo->u16MemUsage % 100,
				pThreadInfo->u16MemAverageUsage / 100, pThreadInfo->u16MemAverageUsage % 100);

	}
	return 0;
}

/* the callback of TraversalDir called by TraversalDirCallback */
static int32_t TraversalDirThreadCallback(const char *pCurPath, struct dirent *pInfo, void *pContext)
{
	StProcessStatisticInfo *pOutsideInfo = (StProcessStatisticInfo *)pContext;
	int32_t s32Err;
	int32_t s32Pid;
	char c8Name[_POSIX_PATH_MAX];

	if (pCurPath[strlen(pCurPath) - 1] == '/')
	{
		sprintf(c8Name, "%s%s/%s", pCurPath, pInfo->d_name, "stat");
	}
	else
	{
		sprintf(c8Name, "%s/%s/%s", pCurPath, pInfo->d_name, "stat");
	}
	/* /proc/<PID>/task/<TID>/stat */
	s32Err = GetStatFileInfo(c8Name, &(pOutsideInfo->stThreadStat));
	if (s32Err != 0)
	{
		return s32Err;
	}
	s32Pid = atoi(pInfo->d_name);
	pOutsideInfo->stThreadInfo.u32Pid = s32Pid;
	ProcessStatisticUpdateAThread(&(pOutsideInfo->pHandle), pOutsideInfo->stThreadInfo.c8Name,
			s32Pid, UpdateAThreadCallback, pContext);
	return 0;
}

/* the callback of TraversalDir  */
static int32_t TraversalDirCallback(const char *pCurPath, struct dirent *pInfo, void *pContext)
{
	/* not a directory */
	int32_t s32Pid;
#if !HAS_CROSS
	if ((pInfo->d_type & DT_DIR) == 0)
	{
		return 0;
	}
#endif
	/* not a process directory */
	if ((pInfo->d_name[0] > '9') || (pInfo->d_name[0] < '0'))
	{
		return 0;
	}
	s32Pid = atoi(pInfo->d_name);
	if (s32Pid > PROCESS_STATISTIC_BASE_PID)
	{
		char c8Name[_POSIX_PATH_MAX];
		StProcessStatisticInfo *pOutsideInfo = (StProcessStatisticInfo *)pContext;
		int32_t s32Err;

		s32Err = GetProcessNameFromPID(c8Name, _POSIX_PATH_MAX, s32Pid);
		if (s32Err != 0)
		{
			return 0;
		}

		if (ProcessStatisticSearchAProcess(pOutsideInfo->pHandle, c8Name, NULL) == NULL)
		{
			/* this process is not in the list */
			return 0;
		}
		/* the process is in the hash list */
		/* save the name */
		strcpy(pOutsideInfo->stThreadInfo.c8Name, c8Name);

		if (pCurPath[strlen(pCurPath) - 1] == '/')
		{
			sprintf(c8Name, "%s%s/%s", pCurPath, pInfo->d_name, "task/");
		}
		else
		{
			sprintf(c8Name, "%s/%s/%s", pCurPath, pInfo->d_name, "task/");
		}
		return TraversalDir(c8Name, false, TraversalDirThreadCallback, pContext);
	}

	return 0;
}


/* the callback of ProcessStatisticTraversal */
static int32_t TraversalCB(int32_t s32Flag, const StHashCursor *pCursor, void *pData, void *pContext)
{
	StProcessStatisticInfo *pOutsideInfo = (StProcessStatisticInfo *)pContext;

	if (s32Flag == _Hash_Traversal_Process)
	{
		pOutsideInfo->pProcessInfo = pData;
	}
	else if (s32Flag == _Hash_Traversal_Thread)
	{
		UnProcessInfo *pThreadInfo = pData;
		if (pOutsideInfo->stThreadInfo.u32StatisticTimes == pThreadInfo->stThreadInfo.u32StatisticTimes)
		{
			pOutsideInfo->pProcessInfo->stMainProcess.u32StatisticTimes = pOutsideInfo->stThreadInfo.u32StatisticTimes;
		}
		else
		{
			PRINT("the thread(%s) is out\n", pCursor->c8Key);
			return -1;	/* the thread had been killed */
		}
	}
	else if (s32Flag == _Hash_Traversal_Process_Out)
	{
		if (pOutsideInfo->pProcessInfo->stMainProcess.u32StatisticTimes !=
				pOutsideInfo->stThreadInfo.u32StatisticTimes)
		{
			PRINT("the process(%s) is out\n", pCursor->c8Key);
			return -1;	/* the process had been killed */

		}
	}

	return 0;
}


static int32_t MCSCallBack(uint32_t u32CmdNum, uint32_t u32CmdCnt, uint32_t u32CmdSize,
        const char *pCmdData, void *pContext)
{
	StProcessStatisticInfo *pOutsideInfo = (StProcessStatisticInfo *)pContext;

	if (u32CmdNum == _Process_Statistic_Add)
	{
		return ProcessStatisticInsertAProcess(&(pOutsideInfo->pHandle), pCmdData);
	}
	else if (u32CmdNum == _Process_Statistic_Delete)
	{
		return ProcessStatisticDeleteAProcess(pOutsideInfo->pHandle, pCmdData);
	}
	return MY_ERR(_Err_CmdType);
}
int ProcessStatisticTest(bool *pIsExit)
{
	int32_t i;
	StProcessStatisticInfo stInfo;
	int32_t s32Err;
	int32_t s32StatisticSocket;

	PRINT("PROCESS_STATISTIC_BASE_PID id %d\n", PROCESS_STATISTIC_BASE_PID);
	memset(&stInfo, 0, sizeof(StProcessStatisticInfo));

	stInfo.pHandle = ProcessStatisticInit(&s32Err);
	if (stInfo.pHandle == NULL)
	{
		PRINT("ProcessStatisticInit error: 0x%08x\n", s32Err);
		return -1;
	}

	s32StatisticSocket = ServerListen(PROCESS_STATISTIC_SOCKET);
	if (s32StatisticSocket < 0)
	{

		PRINT("ServerListen error: 0x%08x\n", s32StatisticSocket);
		s32Err = s32StatisticSocket;
		goto end;
	}


	for (i = 0; i < 6; i++)
	{
		uint16_t u16Index = ProcessStatisticInsertAProcess(&(stInfo.pHandle), pProcessName[i]);
		if (u16Index == HASH_INVALID_INDEX)
		{
			PRINT("ProcessStatisticInsertAProcess error: 0x%08x\n", s32Err);
		}
	}


	while (!(*pIsExit))
	{
		fd_set stSet;
		struct timeval stTimeout;

		int32_t s32Client = -1;
		stTimeout.tv_sec = 2;
		stTimeout.tv_usec = 0;
		FD_ZERO(&stSet);
		FD_SET(s32StatisticSocket, &stSet);

		if (select(s32StatisticSocket + 1, &stSet, NULL, NULL, &stTimeout) <= 0)
		{
			goto statistic;
		}
		if (!FD_ISSET(s32StatisticSocket, &stSet))
		{
			goto statistic;
		}
		s32Client = ServerAccept(s32StatisticSocket);
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
			pMCSStream = MCSSyncReceive(s32Client, true, 1000, &u32Size, &s32Err);
			if (pMCSStream == NULL)
			{
				PRINT("MCSSyncReceive error 0x%08x\n", s32Err);
			}
			else
			{
				MCSResolve((const char *)pMCSStream, u32Size, MCSCallBack, &stInfo);
			}
			MCSSyncFree(pMCSStream);
			close(s32Client);
			continue;
		}
statistic:
		CpuInfo(&(stInfo.stSYSInfo.stCPU));
		MemInfo(&(stInfo.stSYSInfo.stMem));
		TraversalDir("/proc/", false, TraversalDirCallback, &stInfo);

		ProcessStatisticTraversal(stInfo.pHandle, TraversalCB, &stInfo);

		stInfo.stThreadInfo.u32StatisticTimes++;
	}

	ProcessStatisticPrint(stInfo.pHandle);

	ServerRemove(s32StatisticSocket, PROCESS_STATISTIC_SOCKET);
end:
	HashTombDestroy(stInfo.pHandle);

	return 0;

}
int main()
{
	SignalRegister();
	return ProcessStatisticTest(&g_boIsExit);
}

#elif defined GETOPT_TEST
#include "common_define.h"
#include <common.h>

int main (int argc, char *argv[])
{
	int32_t s32Char;
	while ((s32Char = getopt(argc, argv, "vund")) != -1)
	{
		switch (s32Char)
		{
			case 'v':
			{
				PRINT("program version is: %s\n", PROGRAM_VERSION);
				exit(0);
			}
			case 'u':
			{
				PRINT("I will run update part\n");
				break;
			}
			case 'n':
			{
				PRINT("I will run normal part\n");
				break;
			}
			case 'd':
			{
				PRINT("I will daemon\n");
				break;
			}
			default:
				break;
		}
	}

	return 0;
}


#elif defined STRING_DISTANCE_TEST
#include "common_define.h"
#include <common.h>

uint32_t MinValue(uint32_t a, uint32_t b, uint32_t c)
{
	uint32_t u32Tmp = a;
	if (u32Tmp > b)
	{
		u32Tmp = b;
	}
	if (u32Tmp > c)
	{
		u32Tmp = c;
	}
	return u32Tmp;
}
/*
 * A loop method using dynamic programming.
 * Calculate from bottom to top.
 */
uint32_t CalculateStringDistance(const char *pStrA, const char *pStrB)
{
	uint32_t u32LenA = strlen(pStrA);
	uint32_t u32LenB = strlen(pStrB);
	uint32_t c[u32LenA + 1][u32LenB + 1]; // Record the distance of all begin points of each string

	int32_t i, j;
	PRINT("a: %u, b %u\n", u32LenA, u32LenB);
	// i: begin point of strA
	// j: begin point of strB
	for (i = 0; i < u32LenA; i++)
		c[i][u32LenB] = u32LenA - i;
	for (j = 0; j < u32LenB; j++)
		c[u32LenA][j] = u32LenB - j;
	c[u32LenA][u32LenB] = 0;
	PRINT("finish initial\n");

	for (i = u32LenA - 1; i >= 0; i--)
	{
		for (j = u32LenB - 1; j >= 0; j--)
		{
			if (pStrB[j] == pStrA[i])
				c[i][j] = c[i + 1][j + 1];
			else
				c[i][j] = MinValue(c[i][j + 1], c[i + 1][j], c[i + 1][j + 1]) + 1;
		}
	}
	PRINT("finish Calculate\n");
	return c[0][0];
}
int main()
{
	printf("Distance: %u\n", CalculateStringDistance("3GH", "HG3_MainProc_1.0.0"));

	return 0;
}

#elif defined IPC_MSG_TEST
#include "common_define.h"
#include <common.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MSG_TEST		"msg_test.msg"


typedef struct _tagStMsg
{
	long s32Type; /* positive message type */
	char c8Msg[512]; /* message data, of length nbytes */
}StMsg;

bool g_boIsExit = false;

static void ProcessStop(int32_t s32Signal)
{
    PRINT("Signal is %d\n", s32Signal);
    g_boIsExit = true;
}

void *ThreadTest(void *pArg)
{
	key_t keyMsg = MakeASpecialKey(MSG_TEST);
	if (keyMsg < 0)
	{
		PRINT("MakeASpecialKey error\n");
		exit(-1);
	}
	int32_t s32Msg = msgget(keyMsg, IPC_CREAT | 0600);
	if (s32Msg == -1)
	{
		PRINT("msgget error: %s\n", strerror(errno));
		exit(-1);
	}
	PRINT("thread in\n");
	StMsg stMsg;
	int32_t s32RecvSize = 64;
	while(!g_boIsExit)
	{
		stMsg.s32Type = 0;
		int s32Recv = msgrcv(s32Msg, &stMsg, s32RecvSize, 5, 0);
		if (s32Recv < 0)
		{
			PRINT("msgrcv error: %s(%d)\n", strerror(errno), errno);
			if (errno == E2BIG)
			{
				s32RecvSize <<= 1;
				continue;
			}
		}
		else
		{
			PRINT("get a msg(type is: %ld, and size is: %d): %s\n", stMsg.s32Type, s32Recv, stMsg.c8Msg);
			if (strcmp(stMsg.c8Msg, "exit") == 0)
			{
				PRINT("get a exit msg\n");
				continue;
			}
		}
	}
	PRINT("exit thread\n");

	return NULL;
}


int main(int argc, char *argv[])
{
	key_t keyMsg = MakeASpecialKey(MSG_TEST);
	if (keyMsg < 0)
	{
		PRINT("MakeASpecialKey error\n");
		exit(-1);
	}
	int32_t s32Msg = msgget(keyMsg, IPC_CREAT | 0600);
	if (s32Msg == -1)
	{
		PRINT("msgget error: %s\n", strerror(errno));
		exit(-1);
	}
	StMsg stMsg;
	if (argc > 1)
	{
		if (argc > 2)
		{
			stMsg.s32Type = atoi(argv[2]);
		}
		else
		{
			stMsg.s32Type = 1;
		}
		while(1)
		{
			stMsg.c8Msg[0] = 0;
			PRINT("enter some msg\n");
			scanf("%s", stMsg.c8Msg);
			int s32Err = msgsnd(s32Msg, &stMsg, strlen(stMsg.c8Msg) + 1, IPC_NOWAIT);
			if (s32Err != 0)
			{
				PRINT("msgsnd error: %s(%d)\n", strerror(errno), errno);
			}
			PRINT("send a msg\n");
		}

	}
	else
	{
		int32_t s32RecvSize = 64;
		signal(SIGINT, ProcessStop);
		signal(SIGTERM, ProcessStop);
		pthread_t s32Handle = NULL;
		MakeThread(ThreadTest, NULL, false, &s32Handle, false);

		while(!g_boIsExit)
		{
			stMsg.s32Type = 0;
			int s32Recv = msgrcv(s32Msg, &stMsg, s32RecvSize, 3, 0);
			if (s32Recv < 0)
			{
				PRINT("msgrcv error: %s(%d)\n", strerror(errno), errno);
				if (errno == E2BIG)
				{
					s32RecvSize <<= 1;
					continue;
				}
			}
			else
			{
				PRINT("get a msg(type is: %ld, and size is: %d): %s\n", stMsg.s32Type, s32Recv, stMsg.c8Msg);
				stMsg.s32Type = 5;
				msgsnd(s32Msg, &stMsg, strlen(stMsg.c8Msg) + 1, 0);
				if (strcmp(stMsg.c8Msg, "exit") == 0)
				{
					PRINT("get a exit msg\n");
					continue;
				}
			}
		}
		PRINT("exit program\n");
		pthread_join(s32Handle, NULL);
	}
	return 0;
}

#elif defined UNIX_SOCKET_TEST

#define UNIX_TEST		"/tmp/unix_test.socket"
int main(int argc, char *argv[])
{
	SignalRegister();
	/* client */
	if (argc > 1)
	{
		int32_t s32Client = -1;
		s32Client = ClientConnect(UNIX_TEST);
		if(s32Client < 0)
		{
			PRINT("ClientConnect error: 0x%08x\n", s32Client);
			exit(-1);
		}
		int32_t s32Err = 0;
		const uint8_t u8Buf[] =
		{
			0xF5, 0x05, 0x00, 0x14, 0x1A, 0x2B, 0x3C, 0x4D,
			0x5E, 0x64, 0x00, 0xB0, 0x05, 0x00, 0x55, 0xF3,
			0xC1, 0x00, 0x01, 0x01, 0x08, 0x02, 0x14, 0x00
		};
		s32Err = MCSSyncSendData(s32Client, 0, sizeof(u8Buf), u8Buf);
		if (s32Err != 0)
		{
			PRINT("MCSSyncSendData error: 0x%08x\n", s32Err);
		}
		close(s32Client);

	}
	else /* server */
	{
		int32_t s32Server = ServerListen(UNIX_TEST);

		if (s32Server < 0)
		{
			PRINT("ServerListen error: 0x%08x\n", s32Server);
			exit(-1);
		}
		while (!g_boIsExit)
		{
			fd_set stSet;
			struct timeval stTimeout;
			int32_t s32Client = -1;

			stTimeout.tv_sec = 1;
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

			if(s32Client < 0)
			{
				PRINT("ServerAccept error: 0x%08x\n", s32Client);
				ServerRemove(s32Server, UNIX_TEST);
				exit(-1);
			}
			/*
			 * PRINT("Press enter key to get msg\n");
			 * getchar();
			 */
			uint32_t u32Size;
			int32_t s32Err;
			char *pTmp = MCSSyncReceive(s32Client, false, 1000, &u32Size, &s32Err);
			if (pTmp == NULL)
			{
				PRINT("recv msg error: 0x%08x\n", s32Err);
			}
			else
			{
				//PRINT("get a msg: %s\n", pTmp);
				uint32_t i = 0;
				for (i = 0; i < u32Size; i++)
				{
					if ((i & 0x07) == 0)
					{
						printf("\n");
					}
					printf("0x%02hhX, ", pTmp[i]);
				}
				printf("\n");
				MCSFree(pTmp);
			}
		}
		ServerRemove(s32Server, UNIX_TEST);
	}
	return 0;
}

#elif defined UPDATE_TEST
#include "common_define.h"
#include <common.h>
#include "json/json.h"

#ifndef PRINT
#define PRINT(x, ...) printf("[%s:%d]: "x, __FILE__, __LINE__, ##__VA_ARGS__)
#endif


#if 1

#if HAS_CROSS
const char c_c8LanName[] = "eth2.2"; /* <TODO> */
const char c_c8WifiName[] = "wlp2s0"; /* <TODO> */
#else
const char c_c8LanName[] = "p8p1";
const char c_c8WifiName[] = "wlp2s0";
#endif

/* ./test_upgrade 192.168.1.22 443
 * ./test_upgrade 192.168.1.22 8091*/
int main(int argc, char *argv[])
{
	StCloudDomain stStat = {{_Cloud_IsOnline}};
	StIPV4Addr stIPV4Addr[4];
	int32_t s32LanAddr = 0;
	json_object *pObj = NULL;
	if (argc != 3)
	{
		PRINT("usage: %s <server> <port>\n", argv[0]);
		return -1;
	}
	SSLInit();
	{
	uint32_t u32Cnt = 4;
	uint32_t i;
	while (1)
	{
		/* list the network */
		u32Cnt = 4;
		GetIPV4Addr(stIPV4Addr, &u32Cnt);
		for (i = 0; i < u32Cnt; i++)
		{
			if (strcmp(stIPV4Addr[i].c8Name, c_c8LanName) == 0)
			{
				s32LanAddr = i; /* well, the lan is connected */
				break;
			}
			else
			{
				PRINT("name is: %s\n", stIPV4Addr[i].c8Name);
			}
		}
		if (i != u32Cnt)
		{
			break;
		}
		sleep(1);
	}
	}
	memcpy(stStat.stStat.c8ClientIPV4, stIPV4Addr[s32LanAddr].c8IPAddr, 16);
	memcpy(stStat.c8Domain, argv[1], strlen(argv[1]));
	stStat.s32Port = atoi(argv[2]);
	json_object *ClouldGetLastVersion(StCloudDomain *pStat, int32_t *pErr);
	if ((pObj = ClouldGetLastVersion(&stStat, NULL)) == NULL)
	{
		PRINT("Get last version error!\n");
	}
	else
	{
		StMovingFileInfo *pInfo = NULL;
		PRINT("\nreturn:\n%s\n", json_object_to_json_string_ext(pObj, JSON_C_TO_STRING_PRETTY));

		int32_t GetUpdateFile(StCloudStat *pCloudStat, json_object *pFileList, StMovingFileInfo **p2Info);
		GetUpdateFile(&(stStat.stStat), pObj, &pInfo);
		UpdateFileCopyToRun(&pInfo);
		MovingFileInfoRelease(pInfo);
		json_object_put(pObj);
	}

	SSLDestory();
	return 0;
}

#endif

#if 0
int main()
{
	StMovingFileInfo *pInfo = NULL;
	UpdateFileCopyToRun(&pInfo);
	MovingFileInfoRelease(pInfo);
	return 0;
}

#endif

#elif defined SEM_TEST
#include "common_define.h"
#include <common.h>

int main(int argc, const char *argv[])
{
	int32_t s32Err = 0;
	int32_t s32SemId = 0;
	key_t keySem = 0;

	keySem = MakeASpecialKey("semtest");

	if (-1 == keySem)
	{
		PRINT("MakeASpecialKey error: %s\n", strerror(errno));
		s32Err = MY_ERR(_Err_SYS + errno);
		return s32Err;
	}
	PRINT("the key is 0x%08x\n", keySem);


	if (argc < 2)
	{
		s32SemId = semget(keySem, 1, IPC_CREAT | IPC_EXCL | SEM_MODE);

		if (s32SemId >= 0)
		{
			PRINT("press enter key to set sem\n");
			getchar();
			if (semctl(s32SemId, 0, SETVAL, 1) == -1)
			{
				PRINT("semget error: %s\n", strerror(errno));
				semctl(s32SemId, 0, IPC_RMID);
				s32Err = MY_ERR(_Err_SYS + errno);
				return s32Err;
			}
		}
		else
		{
			return -1;
		}

	}
	else
	{
		PRINT("I will get it\n");
		s32SemId = semget(keySem, 0, SEM_MODE);
		if (s32SemId < 0)
		{
			PRINT("semget error: %s\n", strerror(errno));
			s32Err = MY_ERR(_Err_SYS + errno);
			return s32Err;
		}
		else
		{
			struct sembuf stSem;

			stSem.sem_num = 0;
			stSem.sem_op = -1;
			stSem.sem_flg = SEM_UNDO;
			if (semop(s32SemId, &stSem, 1) == -1)
			{
				PRINT("Failed to lock: %s \n", strerror(errno));
				return MY_ERR(_Err_SYS + errno);
			}
			PRINT("Lock OK\n");

			stSem.sem_num = 0;
			stSem.sem_op = 1;
			stSem.sem_flg = SEM_UNDO;
			if (semop(s32SemId, &stSem, 1) == -1)
			{
				PRINT("Failed to unlock: %s \n", strerror(errno));
				return MY_ERR(_Err_SYS + errno);
			}
			PRINT("Unlock OK\n");

		}
	}

	if (argc < 2)
	{
		PRINT("press enter key to remove sem\n");
		getchar();
		semctl(s32SemId, 0, IPC_RMID);
	}

	return 0;
}


#elif defined  CLOUD_STATE_TEST

int main(int argc, const char *argv[])
{
	int32_t s32Handle = 0;
	int32_t s32Err;

	s32Handle = CloudInit(&s32Err);
	if (s32Handle == 0)
	{
		PRINT("CloudInit error: 0x%08x\n", s32Err);
		return -1;
	}
	if (argc > 1)
	{
		char c8Domain[64] = {0};
		int32_t s32Err = CloudGetDomainFromRegion(s32Handle, _Region_UDP,
				"CN" , c8Domain, sizeof(c8Domain));
		if (s32Err == 0)
		{
			StCloudDomain stStat = {{0}};
			GetDomainPortFromString(c8Domain, stStat.c8Domain, 64, &(stStat.s32Port));
			PRINT("CN----%s:%d\n", stStat.c8Domain, stStat.s32Port);
		}
		else
		{
			PRINT("Cannot find CN\n");
		}
		PRINT("CloudIsOnline: %s\n", CloudIsOnline(s32Handle) ? "true" : "false");
	}
	else
	{
		CloudSaveDomainViaRegion(s32Handle, _Region_HTTPS, "CN", "192.168.1.100:443");
		CloudSaveDomainViaRegion(s32Handle, _Region_UDP, "CN", "192.168.1.100:6000");
	}

	PRINT("press enter key to exit\n");
	getchar();
	if (argc > 1)
	{
		CloudDestroy(s32Handle);
	}
	else
	{
		CloudTombDestroy(s32Handle);
	}

	return 0;
}


#elif defined GET_IPV4_ADDR_TEST
int main(int argc, const char *argv[])
{
	StIPV4Addr stIPV4Addr[4];
	uint32_t u32Cnt = 4;
	int32_t s32Err;
	uint32_t i;

	/* list the network */
	s32Err = GetIPV4Addr(stIPV4Addr, &u32Cnt);
	if (s32Err < 0)
	{
		PRINT("GetIPV4Addr error: 0x%08x\n", s32Err);
	}
	else
	{

		for (i = 0; i < u32Cnt ; i++)
		{
			char *pMac = stIPV4Addr[i].c8MacAddr;
			PRINT("name: %s, IPV4: %s, MAC: %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
					stIPV4Addr[i].c8Name, stIPV4Addr[i].c8IPAddr,
					pMac[0], pMac[1], pMac[2], pMac[3], pMac[4], pMac[5]);
		}
	}
	i = 0;
	s32Err = GetInterfaceIPV4Addr(ETH_WIFI_NAME, stIPV4Addr + i);
	if (s32Err < 0)
	{
		PRINT("GetInterfaceIPV4Addr error: 0x%08x\n", s32Err);
	}
	else
	{
		char *pMac = stIPV4Addr[i].c8MacAddr;
		PRINT("name: %s, IPV4: %s, MAC: %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n",
				stIPV4Addr[i].c8Name, stIPV4Addr[i].c8IPAddr,
				pMac[0], pMac[1], pMac[2], pMac[3], pMac[4], pMac[5]);
	}
	s32Err = GetNetLinkStatus(ETH_LAN_NAME);
	PRINT("GetNetLinkStatus : 0x%08x\n", s32Err);
	return 0;
}

#elif defined UDP_STATISTICS_TEST

#if 1
typedef struct _tagStBufSort
{
	time_t s32time;
	char *pBuf;
}StBufSort;

int32_t compare(const void *pLeft, const void *pRight)
{
	return ((const StBufSort *)pLeft)->s32time - ((const StBufSort *)pRight)->s32time;
}
int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("usage: %s <file name>\n", argv[0]);
		return 0;
	}

	FILE *pFile = fopen(argv[1], "r");
	if (pFile == NULL)
	{
		printf("cannot open file\n");
		return 0;
	}
	char c8Buf[1024];

	uint32_t u32Line = 1024;
	StBufSort *pSort = malloc(sizeof(StBufSort) * u32Line);
	if (pSort == NULL)
	{
		printf("error memory\n");
		goto end;
	}

	int32_t s32Cnt = 0;

	while (1)
	{
		char *pReturn = fgets(c8Buf, 1024, pFile);
		if (pReturn == NULL)
		{
			break;
		}
		pReturn = strchr(c8Buf, '\t');
		if (pReturn == NULL)
		{
			printf("string error\n");
			goto end;
		}
		struct tm stTime = {0};
		sscanf(pReturn + 1, "%d/%d/%d %d:%d:%d",
				&(stTime.tm_year), &(stTime.tm_mon), &(stTime.tm_mday),
				&(stTime.tm_hour), &(stTime.tm_min), &(stTime.tm_sec));
		stTime.tm_year -= 1900;
		pSort[s32Cnt].pBuf = NULL;
		pSort[s32Cnt].s32time = mktime(&stTime);
		pSort[s32Cnt].pBuf = malloc(strlen(c8Buf) + 4);
		if (pSort[s32Cnt].pBuf == NULL)
		{
			goto end1;
		}
		strcpy(pSort[s32Cnt].pBuf, c8Buf);
		s32Cnt++;
		if (s32Cnt >= u32Line)
		{
			u32Line *= 2;
			StBufSort *pSort1 = realloc(pSort, sizeof(StBufSort) * u32Line);
			if (pSort1 == NULL)
			{
				printf("error memory\n");
				goto end1;
			}
			pSort = pSort1;
		}
	}
	int32_t i;
	qsort(pSort, s32Cnt, sizeof(StBufSort), compare);
	for(i = 0; i < s32Cnt; i++)
	{
		printf("%s", pSort[i].pBuf);
	}

	uint32_t u32NumOrg = 0;
	uint32_t u32Total = 0;
	uint32_t u32Loss = 0;
	bool boIsBegin = true;
	for(i = 0; i < s32Cnt; i++)
	{
		char *pReturn = strstr(pSort[i].pBuf, "41 4E 47 57");
		if (pReturn == NULL)
		{
			printf("string error\n");
			goto end;
		}
		uint8_t u8HByte = 0, u8LByte = 0;
		sscanf(pReturn + 60, "%hhx %hhx", &u8LByte, &u8HByte);
		uint32_t u32Num = u8HByte;
		u32Num *= 256;
		u32Num += u8LByte;

		if (boIsBegin)
		{
			u32NumOrg = u32Num;
			boIsBegin = false;
			u32Total = 1;
		}
		else
		{
			uint u32NumDiff = u32Num - u32NumOrg;
			if (u32Num < u32NumOrg)
			{
				u32NumDiff = 65536 - u32NumOrg + u32Num;
				uint8_t u8Time = 0;
				sscanf(pReturn + 39, "%hhx", &u8Time);
				if (u8Time == 0)
				{
					/* restart */
					u32Total += u32Num;
					if (u32Num != 0)
					{
						printf("loss: %s", pSort[i].pBuf);
						printf("lost: %u\n\n", u32Num);
						u32Loss += u32Num;
					}
				}
				else
				{
					u32Total += u32NumDiff;
					if (u32NumDiff != 1)
					{
						printf("loss: %s", pSort[i].pBuf);
						printf("lost: %u\n\n", u32NumDiff);
						u32Loss += u32NumDiff;
					}
				}
			}
			else
			{
				u32Total += u32NumDiff;
				if (u32NumDiff != 1)
				{
					printf("loss: %s", pSort[i].pBuf);
					printf("lost: %u\n\n", u32NumDiff);
					u32Loss += u32NumDiff;
				}
			}
			u32NumOrg = u32Num;
		}
	}
	double d64Loss = u32Loss;
	double u64Total = u32Total;
	printf("\n\ntotal: %u, loss: %u ---- (%f%%)\n", u32Total, u32Loss, d64Loss * 100.0 / u64Total);

end1:

	for(i = 0; i < s32Cnt; i++)
	{
		free (pSort[i].pBuf);
	}
	free (pSort);
end:
	fclose(pFile);

	return 0;
}
#endif

#if 0
#include <time.h>

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("usage: %s <file name>\n", argv[0]);
		return 0;
	}

	FILE *pFile = fopen(argv[1], "r");
	if (pFile == NULL)
	{
		printf("cannot open file\n");
		return 0;
	}
	char c8Buf[1024];
	time_t s32TimeOrg = 0;
	uint32_t u32NumOrg = 0;
	uint32_t u32Total = 0;
	uint32_t u32Base = 0;
	uint32_t u32Loss = 0;
	bool boIsBegin = true;
	while (1)
	{
		char *pReturn = fgets(c8Buf, 1024, pFile);
		if (pReturn == NULL)
		{
			break;
		}
		pReturn = strchr(c8Buf, '\t');
		if (pReturn == NULL)
		{
			printf("string error\n");
			goto end;
		}
		struct tm stTime = {0};
		sscanf(pReturn + 1, "%d/%d/%d %d:%d:%d",
				&(stTime.tm_year), &(stTime.tm_mon), &(stTime.tm_mday),
				&(stTime.tm_hour), &(stTime.tm_min), &(stTime.tm_sec));

		pReturn = strstr(pReturn, "41 4E 47 57");
		if (pReturn == NULL)
		{
			printf("string error\n");
			goto end;
		}
		uint8_t u8HByte = 0, u8LByte = 0;
		sscanf(pReturn + 60, "%hhx %hhx", &u8LByte, &u8HByte);
		uint32_t u32Num = u8HByte;
		u32Num *= 256;
		u32Num += u8LByte;
		//printf("number: %u\n", u32Num);
		time_t s32Time = mktime(&stTime);
		if (boIsBegin)
		{
			u32Base = u32Num;
			u32NumOrg = u32Num;
			s32TimeOrg = s32Time;
			boIsBegin = false;
			u32Total = 1;
		}
		else
		{
			uint u32NumDiff = u32Num - u32NumOrg;
			if (u32Num < u32NumOrg)
			{
				u32NumDiff = 65536 - u32NumOrg + u32Num;
				uint8_t u8Time = 0;
				sscanf(pReturn + 39, "%hhx", &u8Time);
				if (u8Time == 0)
				{
					/* restart */
					u32Total += u32Num;
					u32Loss += u32Num;
				}
				else
				{
					u32Total += u32NumDiff;
					if (u32NumDiff != 1)
					{
						printf("loss: %s", c8Buf);
						printf("lost: %u\n\n", u32NumDiff);
						u32Loss += u32NumDiff;
					}
				}
			}
			else
			{
				u32Total += u32NumDiff;
				if (u32NumDiff != 1)
				{
					printf("loss: %s", c8Buf);
					printf("lost: %u\n\n", u32NumDiff);
					u32Loss += u32NumDiff;
				}
			}
			u32NumOrg = u32Num;
		}
	}
end:
	fclose(pFile);

	printf("total: %u, loss: %u\n", u32Total, u32Loss);

	return 0;
}
#endif

#if 0
#include <common.h>
typedef union _tagUnIpAddr
{
	uint32_t u32IPAddr;
	uint8_t u8IPAddr[4];
}UnIpAddr;
typedef struct _tagStFileOut
{
	FILE *pFile;
	UnIpAddr unIpAddr;
	char c8IPAddr[32];
}StFileOut;

StFileOut stFileOutArr[32] = { {NULL, } };
uint32_t u32FileCnt = 0;

uint32_t u32FileInCnt = 0;
char c8FileIn[128][256] = {{0}};

int32_t TraversalDirCallback(const char *pCurPath, struct dirent *pInfo, void *pContext)
{
	snprintf(c8FileIn[u32FileInCnt++], 255, "%s/%s", pCurPath, pInfo->d_name);
	return 0;
}

int32_t compare(const void *pLeft, const void *pRight)
{
	return strcmp((const char *)pLeft, (const char *)pRight);
}


int main(int argc, char *argv[])
{
	uint32_t i, j;
	if (argc != 3)
	{
		printf("usage: %s <input dir> <output dir>\n", argv[0]);
		return 0;
	}

	TraversalDir(argv[1], false, TraversalDirCallback, argv[2]);

	qsort(c8FileIn, u32FileInCnt, 256, compare);



	for (j = 0; j < u32FileInCnt; j++)
	{
		FILE *pFile = fopen(c8FileIn[j], "r");
		char c8Buf[1024];
		printf("open a new file: %s\n", c8FileIn[j]);
		while (1)
		{
			UnIpAddr unIpAddr = {0};
			char *pReturn = fgets(c8Buf, 1024, pFile);
			if (pReturn == NULL)
			{
				break;
			}
			pReturn = c8Buf + strlen(c8Buf) - 5;
			sscanf(pReturn, "%02hhx", &(unIpAddr.u8IPAddr[0]));
			//printf("char: %s\n%c\n", c8Buf, unIpAddr.u8IPAddr[0]);
			for (i = 0; i < u32FileCnt; i++)
			{
				if (unIpAddr.u32IPAddr == stFileOutArr[i].unIpAddr.u32IPAddr)
				{
					char *pTmp = strchr(c8Buf, '\t');
					pTmp = strchr(pTmp + 1, '\t');
					fprintf(stFileOutArr[i].pFile, "%s", pTmp +1);
					fflush(stFileOutArr[i].pFile);
					break;
				}
			}
			if (i == u32FileCnt)
			{
				char c8Name[256], *pTmp;
				sprintf(c8Name, "%s%c", argv[2], unIpAddr.u8IPAddr[0]);
				printf("new file: %s\n", c8Name);
				stFileOutArr[i].pFile = fopen(c8Name, "w+");
				stFileOutArr[i].unIpAddr = unIpAddr;
				pTmp = strchr(c8Buf, '\t');
				pTmp = strchr(pTmp + 1, '\t');
				fprintf(stFileOutArr[i].pFile, "%s", pTmp + 1);
				fflush(stFileOutArr[i].pFile);
				u32FileCnt++;
			}
		}
		fclose(pFile);
	}
#if 1
	for (i = 0; i < u32FileCnt; i++)
	{
		fclose(stFileOutArr[i].pFile);
	}
#endif
	return 0;
}
#endif

#elif defined NET_SSTAT_TEST

#include <sys/types.h>
#include <sys/socket.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/ioctl.h>
//#include <net/if.h>
#include <linux/if.h>
#include <string.h>

#include <common.h>

#define BUFLEN 32768

int main(int argc, char *argv[])
{
	int fd, retval;
	char *buf;
	struct sockaddr_nl addr;
	struct nlmsghdr *nh;
	int len = BUFLEN;
	struct ifinfomsg *ifinfo;
	struct rtattr *attr;

	buf = malloc(BUFLEN);
	fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &len, sizeof(len));
	memset(&addr, 0, sizeof(addr));
	addr.nl_family = AF_NETLINK;
	addr.nl_groups = RTNLGRP_LINK;
	bind(fd, (struct sockaddr*) &addr, sizeof(addr));
	while ((retval = read(fd, buf, BUFLEN)) > 0)
	{
		for (nh = (struct nlmsghdr *) buf; NLMSG_OK(nh, retval); nh =
				NLMSG_NEXT(nh, retval))
		{
			if (nh->nlmsg_type == NLMSG_DONE)
				break;
			else if (nh->nlmsg_type == NLMSG_ERROR)
				return 0;
			else if (nh->nlmsg_type != RTM_NEWLINK)
				continue;
			ifinfo = NLMSG_DATA(nh);
			printf("%u: %s", ifinfo->ifi_index,
					(ifinfo->ifi_flags & IFF_LOWER_UP) ? "up" : "down");
			attr = (struct rtattr*) (((char*) nh) + NLMSG_SPACE(sizeof(*ifinfo)));
			len = nh->nlmsg_len - NLMSG_SPACE(sizeof(*ifinfo));
			for (; RTA_OK(attr, len); attr = RTA_NEXT(attr, len))
			{
				if (attr->rta_type == IFLA_IFNAME)
					printf(" %s", (char*) RTA_DATA(attr));
				else if (attr->rta_type == IFLA_MASTER)
					printf(" [%u]", *(__u32 *) RTA_DATA(attr));
			}
			printf("\n");
		}
		//if (ifinfo->ifi_flags & IFF_LOWER_UP)
		{
			StIPV4Addr stIPV4Addr[4];
			uint32_t u32Cnt = 4;

			/* list the network */
			GetIPV4Addr(stIPV4Addr, &u32Cnt);
		}
	}

	return 0;
}

#elif defined IDPS_TEST
#include "common_define.h"
#include <common.h>

int main()
{
	StPtlIDPS stIDPS;
	char c8GatewaySN[32] = {0};
	GetSNOfGateway(c8GatewaySN, 31);
	printf("c8GatewaySN: %s\n", c8GatewaySN);

	int32_t GetIDPSStruct(StPtlIDPS *pIDPS);
	GetIDPSStruct(&stIDPS);

	return 0;
}


#elif defined GET_REGION_TEST
#include "common_define.h"
#include <common.h>

#ifndef PRINT
#define PRINT(x, ...) printf("[%s:%d]: "x, __FILE__, __LINE__, ##__VA_ARGS__)
#endif

/* ./test_upgrade 203.195.202.228 443 */
int main(int argc, char *argv[])
{
	StCloudDomain stStat = {{_Cloud_IsOnline}};
	StIPV4Addr stIPV4Addr[4];
	int32_t s32LanAddr = 0;
	char c8Region[16];

	StRegionMapping *pMap = NULL;
	uint32_t u32Cnt = 0;
	if (argc != 3)
	{
		PRINT("usage: %s <server> <port>\n", argv[0]);
		return -1;
	}
	SSLInit();
	{
	uint32_t u32Cnt = 4;
	uint32_t i;
	while (1)
	{
		/* list the network */
		GetIPV4Addr(stIPV4Addr, &u32Cnt);
		for (i = 0; i < u32Cnt; i++)
		{
			if (strcmp(stIPV4Addr[i].c8Name, "lo") != 0)
			{
				s32LanAddr = i; /* well, the lan is connected */
				PRINT("get network: %s, and I will bind it\n", stIPV4Addr[i].c8Name);
				break;
			}
			else
			{
				PRINT("name is: %s\n", stIPV4Addr[i].c8Name);
			}
		}
		if (i != u32Cnt)
		{
			break;
		}
		sleep(1);
	}
	}
	memcpy(stStat.stStat.c8ClientIPV4, stIPV4Addr[s32LanAddr].c8IPAddr, 16);
	memcpy(stStat.c8Domain, argv[1], strlen(argv[1]));
	stStat.s32Port = atoi(argv[2]);
	{
	int32_t i;
	for (i = 0; i < 1; i++)
	{
#if 1
		if (CloudGetSelfRegion(&stStat, c8Region, 16) == 0)
		{
			PRINT("gateway's region is: %s\n", c8Region);
		}
		PRINT("*************************************************\n\n\n");
#endif
		if (CloudGetRegionMapping(&stStat, &pMap, &u32Cnt) == 0)
		{
			uint32_t i = 0;
			for (i = 0; i < u32Cnt; i++)
			{
				PRINT("region: %s\n", pMap[i].c8Region);
				PRINT("cloud: %s\n", pMap[i].c8Cloud);
				PRINT("heartbeat: %s\n\n", pMap[i].c8Heartbeat);
			}
			free(pMap);
		}
	}
	}



	SSLDestory();
	return 0;
}

#elif defined COMMON_TEST
int main()
{
	char c8Str[8];
	c8Str[8 - 1] = 0;
	strncpy(c8Str, "01234567879", 8 - 1);
	printf("%s\n", c8Str);
	return 0;
}

#elif defined JSON_TEST
#include <common.h>

int main()
{
	void JSONTest();
	JSONTest();
	return 0;
}

#elif defined STAT_TEST
#include <common.h>
int main(int argc, char *argv[])
{
	struct stat stStat;
	if (argc != 2)
	{
		printf("usage: %s file name\n", argv[0]);
		return -1;
	}

	if( stat(argv[1], &stStat) != 0)
	{
		printf("stat error: %s\n", strerror(errno));
	}
	else
	{
		printf("stat ok\n");
	}

	return 0;
}

#elif defined RUN_A_PROCESS_TEST
#include "common_define.h"
#include <common.h>

#if 1
const char *pArgv[] =
{
	"-b",
	"-i",
	"eth2",
	"-s",
	"/sbin/udhcpc.sh",
	NULL
};

int main()
{
	int32_t RunAProcess(const char *pName, const char *pArgv[]);
	int32_t s32PID = RunAProcess("/sbin/udhcpc", pArgv);
	PRINT("child pid is: %d\n", s32PID);
	return 0;
}


#else


const char *pArgv[] =
{
	"a",
	"b",
	"c",
	NULL
};

int main(int argc, char *argv[])
{
	PRINT("PID: %d\n", getpid());
	if (argc < 2)
	{
		int32_t RunAProcess(const char *pName, const char *pArgv[]);
		RunAProcess("/home/lyndon/workspace/hg3/update_process/test_upgrade", pArgv);
	}
	else
	{
		int i;
		for(i = 0; i < argc; i++)
		{
			PRINT("arg(%d): %s\n", i, argv[i]);
		}
	}
	getchar();
	return 0;
}
#endif
#elif defined UPDATE_COPY_FILE_TEST
#include "common_define.h"
#include <common.h>
int main()
{
	StUpdateFileInfo *pInfo = NULL;
	UpdateFileCopyToRun(&pInfo);
	UpdateFileInfoRelease(pInfo);
	return 0;
}



#elif defined CRC32_FILE_TEST

int main(int argc, char *argv[])
{
	uint32_t u32CRC32 = 0;
	int32_t i;
	char c8Buf[16];
	if (argc != 2)
	{
		printf("usage: %s file name\n", argv[0]);
		return -1;
	}

	CRC32File(argv[1], &u32CRC32);
	PRINT("the CRC32 of file(%s) is: 0x%08X\n", argv[1], u32CRC32);
	srand(time(NULL));
	for (i = 0; i < 12; i++)
	{
		c8Buf[i] = rand() % 26 + 'A';
	}
	c8Buf[i] = 0;

	PRINT("string will be checked: %s\n", c8Buf);

	PRINT("CRC16 of this string is: 0x%04X, CRC32 of this string is: 0x%08X\n",
			CRC16((void *)c8Buf, 12), CRC32Buf((void *)c8Buf, 12));

	return 0;
}

#elif defined CRC32_TABLE_TEST
#include <common.h>

int main()
{
	FILE *pFile = fopen("CRC32_table", "wb+");
	int32_t i, j;
	uint32_t u32CRC32;
	fprintf(pFile, "static uint32_t s_u32CRC32Table[] = {\n");

	for (i = 0; i < 256; i++)
	{
		u32CRC32 = i;
		for (j = 0; j < 8; j++)
		{
			if ((u32CRC32 & 0x01) != 0)
			{
				u32CRC32 = (u32CRC32 >> 1) ^ 0xEDB88320;
			}
			else
			{
				u32CRC32 >>= 1;
			}
		}
		if ((i & 0x07) == 0)
		{
			fprintf(pFile, "\t");
		}
		fprintf(pFile, "0x%08X, ", u32CRC32);
		if ((i & 0x07) == 0x07)
		{
			fprintf(pFile, "\n");
		}
	}

	fprintf(pFile, "};\n");
	fclose(pFile);
	return 0;
}


#elif defined KEEPALIVE_TEST
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/time.h>

#include "common_define.h"
#include <common.h>

#ifndef PRINT
#define PRINT(x, ...) printf("[%s:%d]: "x, __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#define GW_TEST_SN			"0123456789000001"

const char c_c8Domain[] = "www.jiuan.com:8000";


#if HAS_CROSS
const char c_c8LanName[] = "eth2.2"; /* <TODO> */
const char c_c8WifiName[] = "wlp2s0"; /* <TODO> */
#else
const char c_c8LanName[] = "p8p1";
const char c_c8WifiName[] = "wlp2s0";
#endif
#define MAX_REPEAT_CNT			5

const uint16_t c_u16SendTime[MAX_REPEAT_CNT] =
{
	1000, 2000, 4000, 8000, 15000
};

/* ./test_upgrade 203.195.202.228 6000 eth2 */
int main(int argc, char *argv[])
{
	int32_t s32Socket = -1;

	UnUDPMsg unUDPMsg;
	uint16_t u16QueueNum = 0;

	uint32_t u32RepeatCnt = 0;
	uint32_t u32TimeNeedToSleep = 0;/*CLOUD_KEEPALIVE_TIME * 1000;*/
	bool boIsGetMessage = false;

	struct sockaddr stBindAddr = {0};
	struct sockaddr_in stServerAddr = {0};

	char c8GateWaySN[16] = GW_TEST_SN;

	StIPV4Addr stIPV4Addr[4];
	int32_t s32LanAddr = -1;

	struct sockaddr_in *pTmpAddr = (void *)(&stBindAddr);

	uint64_t u64SendTime = TimeGetTime();

	StUDPKeepalive *pUDPKA = UDPKAInit();

	if (pUDPKA == NULL)
	{
		//PrintLog("error memory\n");
		PRINT("error memory\n");
		goto end;
	}


	if (argc != 3)
	{
		PRINT("usage: %s <server> <port>\n", argv[0]);
		goto end;
	}

#if HAS_CROSS
	{
		FILE *pFileSN = popen("nvram_get 2860 GateWaySn", "r");
		if (pFileSN != NULL)
		{
			char c8Buf[32];
			fread(c8GateWaySN, sizeof(c8GateWaySN), 1, pFileSN);
			memcpy(c8Buf, c8GateWaySN, sizeof(c8GateWaySN));
			c8Buf[sizeof(c8GateWaySN)] = 0;
			PRINT("GateWay's SN is: %s\n", c8Buf);
			fclose(pFileSN);
		}
		else
		{
			PRINT("popen error: %s", strerror(errno));
		}
	}

#endif

	{
	uint32_t u32Cnt = 4;
	uint32_t i;
	while (1)
	{
		/* list the network */
		GetIPV4Addr(stIPV4Addr, &u32Cnt);
		for (i = 0; i < u32Cnt; i++)
		{
			//if (strcmp(stIPV4Addr[i].c8Name, argv[3]) == 0)
			if (strcmp(stIPV4Addr[i].c8Name, "lo") != 0)
			{
				s32LanAddr = i; /* well, the lan is connected */
				PRINT("network name is: %s, and I will bind it.\n", stIPV4Addr[i].c8Name);
				break;
			}
			else
			{
				PRINT("name is: %s\n", stIPV4Addr[i].c8Name);
			}
		}
		if (i != u32Cnt)
		{
			break;
		}
		sleep(1);
	}
	}
	s32Socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (s32Socket < 0)
	{
		//PrintLog("socket error: %s\n", strerror(errno));
		PRINT("socket error: %s\n", strerror(errno));
	}
	pTmpAddr->sin_family = AF_INET;
	pTmpAddr->sin_port = htons(0);
	if (inet_aton(stIPV4Addr[s32LanAddr].c8IPAddr, &(pTmpAddr->sin_addr)) == 0)
	{
		PRINT("local IP Address Error!\n");
		goto end;
	}

	if (bind(s32Socket, &stBindAddr, sizeof(struct sockaddr)))
	{
		PRINT("bind error: %s\n", strerror(errno));
		goto end;
	}

	/* 设置套接字选项,接收和发送超时时间 */
	{
	struct timeval stTimeout;
    stTimeout.tv_sec  = 1;
    stTimeout.tv_usec = 0;
    if(setsockopt(s32Socket, SOL_SOCKET, SO_RCVTIMEO, &stTimeout, sizeof(struct timeval)) < 0)
	{
		PRINT("setsockopt error: %s\n", strerror(errno));
		goto end;
	}

    if(setsockopt(s32Socket, SOL_SOCKET, SO_SNDTIMEO, &stTimeout, sizeof(struct timeval)) < 0)
    {
		PRINT("setsockopt error: %s\n", strerror(errno));
		goto end;
    }
	}

	stServerAddr.sin_family = AF_INET;
	stServerAddr.sin_port = htons(atoi(argv[2]));
	if (GetHostIPV4Addr(argv[1], NULL, &(stServerAddr.sin_addr)) != 0)
	{
		goto end;
	}

	while (1)
	{
#if 1
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
#endif
		{
			/* we need read some info from the server */
			struct sockaddr_in stRecvAddr = {0};
			int32_t s32RecvLen = 0;
			{
			/* I don't think we should check the return value */
			socklen_t s32Len = sizeof(struct sockaddr_in);
			s32RecvLen = recvfrom(s32Socket, &unUDPMsg, sizeof(UnUDPMsg), 0, (struct sockaddr *)(&stRecvAddr), &s32Len);
			if (s32RecvLen <= 0 || (s32Len != sizeof (struct sockaddr_in)))
			{
				PRINT("error message\n");
				goto next;
			}
			}
			if(memcmp(&stServerAddr, &stRecvAddr, sizeof(struct sockaddr)) == 0)
			{
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

					UDPKAAddAReceivedTime(pUDPKA, pData->u16QueueNum,u64GetTime, unUDPMsg.stHead.u64TimeStamp);

					PRINT("get echo: %d\n", pData->u16QueueNum);
					PRINT("server %s:%d, and the length is: %d\n",
							inet_ntoa(stRecvAddr.sin_addr), htons(stRecvAddr.sin_port),
							s32RecvLen);
#if defined _DEBUG
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
					boIsGetMessage = true;
					u32TimeNeedToSleep = c_u16SendTime[u32RepeatCnt - 1];
					u32TimeNeedToSleep = u32TimeNeedToSleep * 2 - 1000;
					{
						uint32_t u32Tmp = TimeGetTime() - u64SendTime;
						u32Tmp = c_u16SendTime[u32RepeatCnt - 1] - u32Tmp;
						u32TimeNeedToSleep = 30 * 1000 - (u32TimeNeedToSleep - u32Tmp);
						PRINT("after recv a message, I need to sleep %u(ms) to send next message\n", u32TimeNeedToSleep);
					}
					u32RepeatCnt = 0;
					u16QueueNum++;
					break;
				}
				case 3 ... 4:		/* config message */
				{
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
					break;
				}
				default:
					break;
				}
			}
			else
			{
				PRINT("Unknown server %s:%d(%d), and the length is: %d\n",
						inet_ntoa(stRecvAddr.sin_addr), htons(stRecvAddr.sin_port), stRecvAddr.sin_family,
						s32RecvLen);
				if (0)
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
				goto next;
			}
		}
next:
		if ((TimeGetTime() - u64SendTime) < u32TimeNeedToSleep)
		{
			int64_t s64TimeDiff = 0;
			if (UDPKAGetTimeDiff(pUDPKA, &s64TimeDiff) == 0)
			{
				PRINT("\n\ntime diff is: %lld\n", s64TimeDiff);
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
			continue;
		}

		if (u32RepeatCnt >= MAX_REPEAT_CNT)
		{
			u32RepeatCnt = 0;
			if (!boIsGetMessage)
			{
				u16QueueNum++;
			}
		}
		u32TimeNeedToSleep = c_u16SendTime[u32RepeatCnt];
		u32RepeatCnt++;
		PRINT("time that need to sleep is: %u\n", u32TimeNeedToSleep);

		/* I don't think we should check the return value */
		memset(&unUDPMsg, 0, sizeof(UnUDPMsg));
		memcpy(unUDPMsg.stHead.c8HeadName, UDP_MSG_HEAD_NAME, UDP_MSG_HEAD_NAME_LENGTH);
		u64SendTime = unUDPMsg.stHead.u64TimeStamp = TimeGetTime();
		unUDPMsg.stHead.u16Cmd = 1;
		unUDPMsg.stHead.u16CmdLength = sizeof(StUDPHeartBeat);
		{
			StUDPHeartBeat *pData = (void *)(&(unUDPMsg.pData));
			pData->u16QueueNum = u16QueueNum;
			memcpy(pData->c8SN, c8GateWaySN, sizeof(pData->c8SN));
		}
		sendto(s32Socket, &unUDPMsg, sizeof(StUDPMsgHead) + sizeof(StUDPHeartBeat), MSG_NOSIGNAL,
				(struct sockaddr *)(&stServerAddr), sizeof(struct sockaddr));

		UDPKAAddASendTime(pUDPKA, u16QueueNum, unUDPMsg.stHead.u64TimeStamp);

		PRINT("send a heart beat: %d at %lld\n", u16QueueNum, unUDPMsg.stHead.u64TimeStamp);
#if defined _DEBUG
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
		//u16QueueNum++;
		boIsGetMessage = false;
	}
end:
	UDPKADestroy(pUDPKA);
	if (s32Socket >= 0)
	{
		close(s32Socket);
	}
	return 0;
}

#elif defined UPDKA_TEST
#include "common_define.h"
int main()
{
	StUDPKeepalive *pUDP = UDPKAInit();
	int32_t i = 0;

	for (i = 0; i < 65536; i += 2)
	{
		UDPKAAddASendTime(pUDP, i >> 1, i);
		UDPKAAddAReceivedTime(pUDP, i >> 1, i, i);
	}
	UDPKADestroy(pUDP);

	return 0;
}

#elif defined GET_HOST_IP_TEST

int main(int argc, const char *argv[])
{
	char c8IPV4[IPV4_ADDR_LENGTH];
	int32_t s32Err;
	if (argc < 2)
	{
		PRINT("usage: ./test_upgrade host\n");
		return 0;
	}

	s32Err = GetHostIPV4AddrTimeout(argv[1], 5000, c8IPV4, NULL);
	if (s32Err < 0)
	{
		PRINT("GetHostIPV4AddrTimeout error: 0x%08x\n", s32Err);
	}
	else
	{
		PRINT("the IPV4 of host(%s) is %s\n", argv[1], c8IPV4);
	}

	s32Err = GetHostIPV4Addr(argv[1], c8IPV4, NULL);
	if (s32Err < 0)
	{
		PRINT("GetHostIPV4Addr error: 0x%08x\n", s32Err);
	}
	else
	{
		PRINT("the IPV4 of host(%s) is %s\n", argv[1], c8IPV4);
	}
	return 0;
}


#elif defined DB_TEST


#define		FILE_MODE	(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define		TEST_CNT	500
int main(void)
{
	void *db;
	int32_t i;
	char c8Buf[TEST_CNT][32], c8Data[TEST_CNT][64];
	uint64_t u64Time;
	srand((int)time(NULL));
	if ((db = DBOpen("/tmp/db", O_RDWR | O_CREAT | O_TRUNC, FILE_MODE)) == NULL)
	{
		PRINT("db_open error\n");
		exit(1);
	}

	for (i = 0; i < TEST_CNT; i++)
	{
		int32_t s32Cnt = (rand() % 20) + 5, j;
		sprintf(c8Buf[i], "SN%05d%05d", rand() % 100000, rand() % 100000);
		for(j = 0; j < s32Cnt; j++)
		{
			c8Data[i][j] = (rand() % 10) + '0';
		}
		c8Data[i][j] = 0;
		if (DBStore(db, c8Buf[i], c8Data[i], DB_STORE) == -1)
		{
			PRINT("db_store error for %s, %s\n", c8Buf[i], c8Data[i]);
			exit(1);
		}
	}
	u64Time = TimeGetTime();
	for (i = 0; i < TEST_CNT; i++)
	{
		if (DBFetch(db, c8Buf[i]) == NULL)
		{
			PRINT("db_fetch error for %d----%s\n", i, c8Buf[i]);
			exit(1);
		}
	}
	u64Time = TimeGetTime() - u64Time;
	PRINT("find %d entry using time: %lldms\n", TEST_CNT, u64Time);

	for (i = 0; i < TEST_CNT; i++)
	{
		char *pTmp = DBFetch(db, c8Buf[i]);
		if(strcmp(pTmp, c8Data[i]) != 0)
		{
			PRINT("SN: %s is not same(%s)(%s)\n", c8Buf[i], pTmp, c8Data[i]);
		}
	}


	DBClose(db);
	exit(0);
}

#elif defined MMAP_TEST
#include <common.h>

#define TEST_CNT	4096
#if 1 /* mmap memory file */
#include <sys/mman.h>
#include <unistd.h>
int main()
{
	char c8Buf[4096] = {0};
	int32_t i, s32Cnt;
	FILE *pFile;
	int32_t s32Fd;
	void *pMapAddr = NULL;
	uint64_t u64Tmp;
	uint64_t u64Time;

	for (i = 0; i < 4095; i++)
	{
		c8Buf[i] = rand() % 94 + ' ';
	}
	c8Buf[i] = 0;

	pFile = tmpfile();
	s32Fd = fileno(pFile);

	ftruncate(s32Fd, 4096);

	pMapAddr = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, s32Fd, 0);

	u64Time = TimeGetTime();
	s32Cnt = 0;
	while (s32Cnt < TEST_CNT)
	{
		memcpy(pMapAddr, c8Buf, 4096);
		memcpy(c8Buf, pMapAddr, 4096);
		s32Cnt++;
	}
	u64Time = TimeGetTime() - u64Time;
	printf("mmap: %d, time using %lldms\n", TEST_CNT, u64Time);
	munmap(pMapAddr, 4096);
	fclose(pFile);
	u64Tmp = (uint64_t)4096 * 2 * TEST_CNT * 1000 * 100;
	u64Tmp /= u64Time;
	u64Tmp /= 1024;
	u64Tmp /= 1024;
	printf("copy speed %lld.%02lldM/s\n", u64Tmp / 100, u64Tmp % 100);

	return 0;
}
#else
int main()
{
	char c8Buf[4096] = {0};
	int32_t i, s32Cnt;
	void *pMapAddr = NULL;
	uint64_t u64Tmp;
	uint64_t u64Time;

	for (i = 0; i < 4095; i++)
	{
		c8Buf[i] = rand() % 94 + ' ';
	}
	pMapAddr = malloc(4096);
	u64Time = TimeGetTime();
	s32Cnt = 0;
	while (s32Cnt < TEST_CNT)
	{
		memcpy(pMapAddr, c8Buf, 4096);
		memcpy(c8Buf, pMapAddr, 4096);
		s32Cnt++;
	}
	u64Time = TimeGetTime() - u64Time;
	printf("memory: %d, time using %lldms\n", TEST_CNT, u64Time);
	u64Tmp = (uint64_t)4096 * 2 * TEST_CNT * 1000 * 100;
	u64Tmp /= u64Time;
	u64Tmp /= 1024;
	u64Tmp /= 1024;
	printf("copy speed %lld.%02lldM/s\n", u64Tmp / 100, u64Tmp % 100);
	free(pMapAddr);
	return 0;
}
#endif


#elif defined SEND_FD_TEST
#include <common.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#define TEST_SOCKET		"/tmp/test.sock"
#define TEST_FD			0

int main(int argc, char *argv[])
{
	char c8Buf[4096] = {0};
	int32_t i;
	for (i = 0; i < 4095; i++)
	{
		c8Buf[i] = rand() % 94 + ' ';
	}
	c8Buf[i] = 0;
	if (argc > 1)	/* client */
	{
		int32_t s32Client;
#if TEST_FD
		int32_t s32FD;
		FILE *pFile = NULL;
#endif
		int32_t s32Cnt = 0;
		uint64_t u64TimeEnd;
		uint64_t u64Time = TimeGetTime();
		printf("client in %d\n", getpid());
#if TEST_FD
		pFile = tmpfile();
		fwrite(c8Buf, 4096, 1, pFile);
#endif
		while (s32Cnt < 4096)
		{
#if TEST_FD


			s32FD = fileno(pFile);
			s32Client = ClientConnect(TEST_SOCKET);
			SendFd(s32Client, s32FD);

#else
			s32Client = ClientConnect(TEST_SOCKET);
			MCSSyncSendData(s32Client, 1000, 4096, c8Buf);
			close(s32Client);
#endif
			close(s32Client);
			s32Cnt++;
		}
#if 0
		fclose(pFile);
#endif
		u64TimeEnd = TimeGetTime();
		printf("time using %lldms\n", u64TimeEnd - u64Time);
	}
	else			/* server */
	{
		int32_t s32Server = ServerListen(TEST_SOCKET);
		int32_t s32Client;
#if TEST_FD
		int32_t s32FD;
#endif
		printf("server in %d\n", getpid());
		while (1)
		{
			s32Client = ServerAccept(s32Server);

#if TEST_FD
			s32FD = ReceiveFd(s32Client);

			//printf("get fd %d\n", s32FD);
			//printf("press enter key to continue\n");
			//getchar();
			//lseek(s32FD, 0, SEEK_SET);
			//c8Buf[0] = 0;
			read(s32FD, c8Buf, 4096);
			//printf("get message: %s\n", c8Buf);
			close(s32FD);
#else
			uint32_t u32Size;
			void *pBuf = MCSSyncReceive(s32Client,false, 1000, &u32Size, NULL);
			MCSSyncFree(pBuf);
#endif
			close(s32Client);
		}
		printf("press enter key to exit\n");
		getchar();
		ServerRemove(s32Server, TEST_SOCKET);
	}

	return 0;
}


#elif defined AUTH_TEST
#include "common_define.h"
#include <common.h>
#ifndef PRINT
#define PRINT(x, ...) printf("[%s:%d]: "x, __FILE__, __LINE__, ##__VA_ARGS__)
#endif


#if HAS_CROSS
const char c_c8LanName[] = "eth2.2"; /* <TODO> */
const char c_c8WifiName[] = "wlp2s0"; /* <TODO> */
#else
const char c_c8LanName[] = "p8p1";
const char c_c8WifiName[] = "wlp2s0";
#endif

/* ./test_upgrade 203.195.202.228 443 */
int main(int argc, char *argv[])
{
	StCloudDomain stStat = {{_Cloud_IsOnline}};
	StIPV4Addr stIPV4Addr[4];
	int32_t s32LanAddr = 0;

#if 1
	char c8ID[PRODUCT_ID_CNT];
	char c8Key[XXTEA_KEY_CNT_CHAR];
	{
	int32_t i;
	char c8Buf[4] = {0};
	const char *pID = "31343036313830303030303030303234";
	const char *pKey = "2fe78b0d913d727c55b40f483b525d2d";
	printf("ID: \n");
	for (i = 0; i < 16; i++)
	{
		uint32_t u32Tmp = i * 2;
		c8Buf[0] = pID[u32Tmp];
		c8Buf[1] = pID[u32Tmp + 1];
		sscanf(c8Buf, "%02hhX", c8ID + i);
		printf("%02hhX", c8ID[i]);
	}
	printf("\n");

	printf("Key: \n");
	for (i = 0; i < 16; i++)
	{
		uint32_t u32Tmp = i * 2;
		c8Buf[0] = pKey[u32Tmp];
		c8Buf[1] = pKey[u32Tmp + 1];
		sscanf(c8Buf, "%02hhX", c8Key + i);
		printf("%02hhX", c8Key[i]);
	}
	printf("\n");

	}

#else
#if 1
	const char c8ID[PRODUCT_ID_CNT] = {"0123456789012345"};
	const char c8Key[XXTEA_KEY_CNT_CHAR] =
	{
		0xC5, 0x14, 0x68, 0x31, 0xB3, 0x9F, 0xF0, 0xCC,
		0x23, 0xA5, 0x9C, 0x30, 0x19, 0x6D, 0x38, 0x73
	};
#else
	const char c8ID[PRODUCT_ID_CNT] =
	{
		0x31, 0x34, 0x30, 0x36, 0x31, 0x38, 0x30, 0x30,
		0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x32, 0x30
	};
	const char c8Key[XXTEA_KEY_CNT_CHAR] =
	{
		0x6e, 0x2e, 0xcb, 0x96, 0x30, 0xb2, 0x98, 0x64,
		0x8c, 0xf9, 0x42, 0x12, 0x50, 0x95, 0x83, 0x36
	};

#endif
#endif
	if (argc != 3)
	{
		PRINT("usage: %s <server> <port>\n", argv[0]);
		return -1;
	}
	SSLInit();
	{
	uint32_t u32Cnt = 4;
	uint32_t i;
	while (1)
	{
		/* list the network */
		GetIPV4Addr(stIPV4Addr, &u32Cnt);
		for (i = 0; i < u32Cnt; i++)
		{
			if (strcmp(stIPV4Addr[i].c8Name, c_c8LanName) == 0)
			{
				s32LanAddr = i; /* well, the lan is connected */
				break;
			}
			else
			{
				PRINT("name is: %s\n", stIPV4Addr[i].c8Name);
			}
		}
		if (i != u32Cnt)
		{
			break;
		}
		sleep(1);
	}
	}
	memcpy(stStat.stStat.c8ClientIPV4, stIPV4Addr[s32LanAddr].c8IPAddr, 16);
	memcpy(stStat.c8Domain, argv[1], strlen(argv[1]));
	stStat.s32Port = atoi(argv[2]);
	if (CloudAuthentication(&stStat, true, c8ID, c8Key) == 0)
	{
		PRINT("Auth OK!\n");
	}
	CloudKeepAlive(&stStat, c8ID);

	SSLDestory();
	return 0;
}


#elif defined XXTEA_TEST

int main()
{
#if 1
	int32_t s32Key[4] =
	{
		0x01234567, 0x89ABCDEF, 0xFEDCBA98, 0x76543210
	};
	char c8ID[16];
	int32_t i, j;
	srand(time(NULL));
	for (j = 0; j < 1; j++)
	{
		printf("org ID is: ");
		for (i = 0; i < 16; i++)
		{
			c8ID[i] = (rand() % 10) + '0';
			printf("%c", c8ID[i]);
		}
		printf("\ncoding is: ");
		btea((int32_t *)c8ID, 4, s32Key);
		for (i = 0; i < 16; i++)
		{
			printf("%02hhx ", c8ID[i]);
		}
		btea((int32_t *)c8ID, -4, s32Key);
		printf("\ndecoding is: ");
		for (i = 0; i < 16; i++)
		{
			printf("%c", c8ID[i]);
		}
		printf("\n\n\n");
	}
#else


	int32_t i;
	char c8RServer[36] = {"899AD0DE672482234961E4B1D90FEE6B"}, c8R[16];
	char c8KeyAscii[36] = {"6e2ecb9630b298648cf9421250958336"}, c8Key[16];
#if 0
	const char c8Key[16] =
	{
		0xC5, 0x14, 0x68, 0x31,
		0xB3, 0x9F, 0xF0, 0xCC,
		0x23, 0xA5, 0x9C, 0x30,
		0x19, 0x6D, 0x38, 0x73
	};
	const char c8Key[16] =
	{
		0x31, 0x68, 0x14, 0xC5,
		0xCC, 0xF0, 0x9F, 0xB3,
		0x30, 0x9C, 0xA5, 0x23,
		0x73, 0x38, 0x6D, 0x19,
	};
#endif
	char c8Buf[4] = {0};
	printf("R: \n");
	for (i = 0; i < 16; i++)
	{
		uint32_t u32Tmp = i * 2;
		c8Buf[0] = c8RServer[u32Tmp];
		c8Buf[1] = c8RServer[u32Tmp + 1];
		sscanf(c8Buf, "%02hhX", c8R + i);
		printf("%02hhX", c8R[i]);
	}
	printf("\n");

	printf("Key: \n");
	for (i = 0; i < 16; i++)
	{
		uint32_t u32Tmp = i * 2;
		c8Buf[0] = c8KeyAscii[u32Tmp];
		c8Buf[1] = c8KeyAscii[u32Tmp + 1];
		sscanf(c8Buf, "%02hhX", c8Key + i);
		printf("%02hhX", c8Key[i]);
	}
	printf("\n");

	printf("Code:\n");
	btea((int32_t *)c8R, 4, (int32_t *)c8Key);
	for (i = 0; i < 16; i++)
	{
		printf("%02hhX", c8R[i]);
	}
	printf("\n");
#endif
	return 0;
}



#elif defined HTTPS_TEST
//ebsnew.boc.cn/boc15/login.html
//pbnj.ebank.cmbchina.com/CmbBank_GenShell/UI/GenShellPC/Login/Login.aspx
//ebank.spdb.com.cn/per/gb/otplogin.jsp
/*
 * 1、api1--网关认证询问测试地址：
http://203.195.202.228:8008/API/auth.ashx

2、post内容：
content={"Sc":"001cfe2fe7044aa691d4e6eff9bfb56c",
"Sv":"56435ce5601f40c59b1db14405578f60","QueueNum":123,
"IDPS":{"Ptl":"com.jiuan.BPV20","SN":"00000001","FVer":"1.0.2",
"HVer":"1.0.1","MFR":"iHealth","Model":"BP3 11070","Name":"BP Monitor"},"Command":"F5"}

因为服务器redis服务还没有更新，所以目前返回结果为：
{"Result":3,"QueueNum":123,"ResultMessage":"5000","TS":1402907636419,"ReturnValue":"通用缓存异常"}
更新之后，正确结果:{"Result":1,"QueueNum":123,"ResultMessage":"1000","TS":1402906148304,"ReturnValue":"F0"}

如果仅仅为了测试通道发送和接收，目前已经具备条件，可以测试！
 *
 */
#if 0
#define HTTPS_SERVER_DOMAIN				"boc.cn"
#define HTTPS_SERVER_PORT				443

#define HTTPS_SERVER_SECOND_DOMAIN		"ebsnew"
#define HTTPS_SERVER_FILE				"boc15/login.html"

#define HTTPS_SEND_BODY					NULL
#define HTTPS_BODY_LENGTH				(-1)
#else
#define HTTPS_SERVER_DOMAIN				"203.195.202.228"
#define HTTPS_SERVER_PORT				8018

#define HTTPS_SERVER_SECOND_DOMAIN		NULL
#define HTTPS_SERVER_FILE				"gateway/product_data.ashx"

#define HTTPS_SEND_BODY					"content={ \"Sc\": \"001cfe2fe7044aa691d4e6eff9bfb56c\", \"Sv\": \"a379eee043dd4521b5376613d4fea16c\","\
        "\"QueueNum\": 123, \"IDPS\": { \"Ptl\": \"com.jiuan.HGV010\", \"Name\": \"19\", \"FVer\": \"1.1.0\", \"HVer\": \"1.1.0\", "\
        "\"MFR\": \"iHealth\", \"Model\": \"G1-100\", \"SN\": \"012345678900000B\" }, \"ProdData\": { \"Sc\": \"001cfe2fe7044aa691d4e6eff9bfb56c\","\
        "\"Sv\": \"a379eee043dd4521b5376613d4fea16c\", \"IDPS\": { \"Ptl\": \"com.jiuan.BPW10\", \"Name\": \"iHealth\", \"FVer\": \"0.0.1\", "\
        "\"HVer\": \"0.0.1\", \"MFR\": \"iHealth\", \"Model\": \"BP7 11070\", \"SN\": \"1A2B3C4D516100000000000000000000\" }, \"UserData\": [ { \"UserID\": 3, \"Data\": \"iAwXERgATDRD\n\" }, "\
        "\{ \"UserID\": 3, \"Data\": \"iAwXERsARDBH\n\" }, { \"UserID\": 3, \"Data\": \"CAwXERwAQSc8\n\" } ] } }"
#define HTTPS_BODY_LENGTH				(-1)
#endif

#define HTTP_SERVER_DOMAIN				"yinyueshiting.baidu.com"
#define HTTP_SERVER_PORT				80
#define HTTP_SERVER_SECOND_DOMAIN		NULL
#define HTTP_SERVER_FILE				"data2/music/134367888/12588566710800128.mp3?xcode=bbdac2782066ff097c17f8d0a8f1d47260cf242dce2c1cbc"
#define HTTP_SEND_BODY					NULL
#define HTTP_BODY_LENGTH				0


#if 0
#define SERVER_DOMAIN			"203.195.202.228"
#if 1
#define SERVER_PORT				443
#else
#define SERVER_PORT				8008
#endif
#define SERVER_SECOND_DOMAIN	NULL
#define SERVER_FILE				"API/auth.ashx"

#define SEND_BODY				"content={\"Sc\":\"001cfe2fe7044aa691d4e6eff9bfb56c\","\
								"\"Sv\":\"56435ce5601f40c59b1db14405578f60\",\"QueueNum\":123,"\
								"\"IDPS\":{\"Ptl\":\"com.jiuan.BPV20\",\"SN\":\"00000001\",\"FVer\":\"1.0.2\","\
								"\"HVer\":\"1.0.1\",\"MFR\":\"iHealth\",\"Model\":\"BP3 11070\","\
								"\"Name\":\"BP Monitor\"},\"Command\":\"F5\"}"
#define BODY_LENGTH				(sizeof(SEND_BODY) - 1)

#endif
#include <sys/types.h>
#include <sys/wait.h>
static int32_t g_s32Cnt = 0;
void *UsingThread(void *pArg)
{
	StCloudDomain stStat;// = { {_Cloud_IsOnline, LOACAL_IP}, SERVER_DOMAIN, SERVER_PORT};
	StSendInfo  stInfo; //= { true, SERVER_SECOND_DOMAIN, SERVER_FILE, SEND_BODY, BODY_LENGTH};
	StMMap stMMap = {NULL,};
	int32_t s32Err;
	int32_t argc = (int32_t)pArg;
	StIPV4Addr stAddr[8];
	uint32_t i, u32AddrSize = 8;

	//while (!g_boIsExit)
	{
		s32Err = GetIPV4Addr(stAddr, &u32AddrSize);
		if (s32Err != 0)
		{
			PRINT("GetIPV4Addr error: 0x%08x\n", s32Err);
			return NULL;
		}

		stStat.stStat.emStat = _Cloud_IsOnline;
		if (argc > 1)
		{
			strcpy(stStat.c8Domain, HTTPS_SERVER_DOMAIN);
			stStat.s32Port = HTTPS_SERVER_PORT;

			stInfo.boIsGet = true;
			stInfo.pSecondDomain = HTTPS_SERVER_SECOND_DOMAIN;
			stInfo.pFile = HTTPS_SERVER_FILE;
			stInfo.pSendBody = HTTPS_SEND_BODY;
			stInfo.s32BodySize = HTTPS_BODY_LENGTH;

		}
		else
		{
			strcpy(stStat.c8Domain, HTTP_SERVER_DOMAIN);
			stStat.s32Port = HTTP_SERVER_PORT;

			stInfo.boIsGet = true;
			stInfo.pSecondDomain = HTTP_SERVER_SECOND_DOMAIN;
			stInfo.pFile = HTTP_SERVER_FILE;
			stInfo.pSendBody = HTTP_SEND_BODY;
			stInfo.s32BodySize = HTTP_BODY_LENGTH;
		}

		for (i = 0; i < u32AddrSize; i++)
		{
			if (strcmp(stAddr[i].c8Name, ETH_LAN_NAME) != 0)
			{
				continue;
			}
			printf("network: %s, IP address: %s begin to try\n", stAddr[i].c8Name, stAddr[i].c8IPAddr);
			strncpy(stStat.stStat.c8ClientIPV4, stAddr[i].c8IPAddr, 16);

			if (argc > 1)
			{
				s32Err = CloudSendAndGetReturn(&stStat, &stInfo, &stMMap);
			}
			else
			{
				s32Err = CloudSendAndGetReturnNoSSL(&stStat, &stInfo, &stMMap);
			}
			if (s32Err == 0)
			{
#if 0
				FILE *pFile;
				uint32_t u32BodyLength = 0, u32HeaderLength = 0;
				char *pTmp;
				char c8Name[64];

				if (argc > 1)
				{
					pFile = fopen("HTTPS_test_body.file", "wb+");
				}
				else
				{
					pFile = fopen("HTTP_test_body.file", "wb+");
				}

				pTmp = strstr((const char *)(stMMap.pMap), "\r\n\r\n");
				pTmp += 4;
				u32HeaderLength = ((uint32_t)pTmp - (uint32_t)stMMap.pMap);

				pTmp = strstr((const char *)(stMMap.pMap), "Content-Length: ");
				if (pTmp == NULL)
				{
					u32BodyLength = stMMap.u32MapSize - u32HeaderLength;
				}
				else
				{
					sscanf(pTmp, "Content-Length: %u", &u32BodyLength);
				}

				pTmp = strstr((const char *)(pTmp), "filename=");
				if (pTmp == NULL)
				{
					sprintf(c8Name, "UnkownName");
					pTmp = c8Name;
				}
				else
				{
					sscanf(pTmp, "%s", c8Name);
					pTmp = strchr(c8Name, '\"');
					pTmp += 1;
					c8Name[strlen(c8Name) - 1] = 0;
				}
				PRINT("file name is: %s\n", pTmp);
				PRINT("file length is: %uB\n", u32BodyLength);
				pTmp = stMMap.pMap;
				fwrite(pTmp + u32HeaderLength, u32BodyLength, 1, pFile);
				fclose(pFile);
#endif
				CloudMapRelease(&stMMap);
				g_s32Cnt++;
			}
		}
	}

	return NULL;

}

int main(int argc, const char *argv[])
{
#if 1
	const int32_t s32TestNumber = 80;
	pthread_t s32Tid[s32TestNumber];
	int32_t i;
	SignalRegister();

	if (argc > 1)
	{
		SSLInit();
	}
	PRINT("PID is: %d, and press enter key to continue\n", getpid());
	getchar();

#if 1
	while (!g_boIsExit)
	{
		for (i = 0; i < s32TestNumber; i++)
		{
			MakeThread(UsingThread, (void *)argc, false, s32Tid + i, false);
		}
		for (i = 0; i < s32TestNumber; i++)
		{
			pthread_join(s32Tid[i], NULL);
		}
		PRINT("next test cycle\n");
		sleep(10);
	}
#else
	for (i = 0; i < s32TestNumber; i++)
	{
		UsingThread((void *)argc);
	}
#endif

#else
	StCloudDomain stStat;// = { {_Cloud_IsOnline, LOACAL_IP}, SERVER_DOMAIN, SERVER_PORT};
	StSendInfo  stInfo; //= { true, SERVER_SECOND_DOMAIN, SERVER_FILE, SEND_BODY, BODY_LENGTH};
	StMMap stMMap = {NULL,};
	int32_t s32Err;

	StIPV4Addr stAddr[8];
	uint32_t i, u32AddrSize = 8;

	SignalRegister();
	while (!g_boIsExit)
	{
		s32Err = GetIPV4Addr(stAddr, &u32AddrSize);
		if (s32Err != 0)
		{
			PRINT("GetIPV4Addr error: 0x%08x\n", s32Err);
			return -1;
		}

		stStat.stStat.emStat = _Cloud_IsOnline;
		if (argc > 1)
		{
			SSLInit();
			strcpy(stStat.c8Domain, HTTPS_SERVER_DOMAIN);
			stStat.s32Port = HTTPS_SERVER_PORT;

			stInfo.boIsGet = true;
			stInfo.pSecondDomain = HTTPS_SERVER_SECOND_DOMAIN;
			stInfo.pFile = HTTPS_SERVER_FILE;
			stInfo.pSendBody = HTTPS_SEND_BODY;
			stInfo.s32BodySize = HTTPS_BODY_LENGTH;

		}
		else
		{
			strcpy(stStat.c8Domain, HTTP_SERVER_DOMAIN);
			stStat.s32Port = HTTP_SERVER_PORT;

			stInfo.boIsGet = true;
			stInfo.pSecondDomain = HTTP_SERVER_SECOND_DOMAIN;
			stInfo.pFile = HTTP_SERVER_FILE;
			stInfo.pSendBody = HTTP_SEND_BODY;
			stInfo.s32BodySize = HTTP_BODY_LENGTH;
		}

		for (i = 0; i < u32AddrSize; i++)
		{
			if (strcmp(stAddr[i].c8Name, ETH_LAN_NAME) != 0)
			{
				continue;
			}
			printf("network: %s, IP address: %s begin to try\n", stAddr[i].c8Name, stAddr[i].c8IPAddr);
			strncpy(stStat.stStat.c8ClientIPV4, stAddr[i].c8IPAddr, 16);

			if (argc > 1)
			{
				s32Err = CloudSendAndGetReturn(&stStat, &stInfo, &stMMap);
			}
			else
			{
				s32Err = CloudSendAndGetReturnNoSSL(&stStat, &stInfo, &stMMap);
			}
			if (s32Err == 0)
			{
				FILE *pFile;
				uint32_t u32BodyLength = 0, u32HeaderLength = 0;
				char *pTmp;
				char c8Name[64];

				if (argc > 1)
				{
					pFile = fopen("HTTPS_test_body.file", "wb+");
				}
				else
				{
					pFile = fopen("HTTP_test_body.file", "wb+");
				}

				pTmp = strstr((const char *)(stMMap.pMap), "\r\n\r\n");
				pTmp += 4;
				u32HeaderLength = ((uint32_t)pTmp - (uint32_t)stMMap.pMap);

				pTmp = strstr((const char *)(stMMap.pMap), "Content-Length: ");
				if (pTmp == NULL)
				{
					u32BodyLength = stMMap.u32MapSize - u32HeaderLength;
				}
				else
				{
					sscanf(pTmp, "Content-Length: %u", &u32BodyLength);
				}

				pTmp = strstr((const char *)(pTmp), "filename=");
				if (pTmp == NULL)
				{
					sprintf(c8Name, "UnkownName");
					pTmp = c8Name;
				}
				else
				{
					sscanf(pTmp, "%s", c8Name);
					pTmp = strchr(c8Name, '\"');
					pTmp += 1;
					c8Name[strlen(c8Name) - 1] = 0;
				}
				PRINT("file name is: %s\n", pTmp);
				PRINT("file length is: %uB\n", u32BodyLength);
				pTmp = stMMap.pMap;
				fwrite(pTmp + u32HeaderLength, u32BodyLength, 1, pFile);
				fclose(pFile);

				CloudMapRelease(&stMMap);
				getchar();
			}
		}
		usleep(1000);
	}

#endif
	getchar();
	if (argc > 1)
	{
		SSLDestory();
	}
	getchar();
	PRINT("g_s32Cnt: %d\n", g_s32Cnt);
	return 0;
}
#elif defined HTTPS_POLL_TEST

int main()
{
	void CSGRNBPollTest1(void);
	SignalRegister();
	CSGRNBPollTest1();
	return 0;
}


#elif defined TMPFILE_TEST
#include <common.h>
#include <sys/types.h>
#include <unistd.h>
int main()
{
	FILE *pFile = tmpfile();
	char c8Buf[16] = {0};

	fwrite("test ok", sizeof("test ok"), 1, pFile);
	fflush(pFile);
	printf("PID is %d, fileno is %d\n", getpid(), fileno(pFile));
	printf("press enter key to continue\n");
	getchar();

	fseek(pFile, 0, SEEK_SET);
	fread(c8Buf, 16, 1, pFile);

	printf("file: %s", c8Buf);

	printf("press enter key to continue\n");
	getchar();

	fclose(pFile);
	return 0;
}



#elif defined PROCESS_LIST_TEST
#include <common.h>

#include <signal.h>
#include <unistd.h>


bool g_boIsExit = false;

static void ProcessStop(int32_t s32Signal)
{
    printf("Signal is %d\n", s32Signal);
    g_boIsExit = true;
}


int main(int argc, char *argv[])
{
	int32_t s32Handle, s32Err;

	signal(SIGINT, ProcessStop);
	signal(SIGTERM, ProcessStop);

	s32Handle = ProcessListInit(&s32Err);
	if (s32Handle == 0)
	{
		printf("ProcessListInit error 0x%08x\n", s32Err);
		PrintLog("ProcessListInit error 0x%08x\n", s32Err);
	}
	while (!g_boIsExit)
	{
		ProcessListUpdate(s32Handle);
		sleep(1);
	}
	ProcessListDestroy(s32Handle);
	return 0;
}



#elif defined TRAVERSAL_DIR_TEST

int32_t Callback(const char *pCurPath, struct dirent *pInfo, void *pContext)
{
	if (pCurPath[strlen(pCurPath) - 1] != '/')
	{
		printf("%s/%s\n", pCurPath, pInfo->d_name);
	}
	else
	{
		printf("%s%s\n", pCurPath, pInfo->d_name);
	}
	return 0;
}

int main(int argc, char *argv[])
{
	bool boIsRecursion = false;
	if (argc < 2)
	{
		printf("usage: %s directory\n", argv[0]);
	}
	else if (argc > 2)
	{
		boIsRecursion = true;
	}

	TraversalDir(argv[1], boIsRecursion, Callback, NULL);
	return 0;
}


#elif defined LOG_FILE_TEST
#include "common_define.h"
#include <common.h>
int main(int argc, char *argv[])
{
	int i;
	int32_t s32Day = 15;
	int32_t s32Handle = LogFileCtrlInit(NULL, NULL);
	(void)i;
	(void)s32Day;
#if 1
#if 1
#if 0
	for (i = 0; i < 5; i++)
	{
		int32_t j;
		for (j = 0; j < 24; j++)
		{
			int32_t k;
			for (k = 0; k < 60; k++)
			{
				int32_t m;
				for (m = 0; m < 6; m++)
				{
					LogFileCtrlDelAOldestBlockLog(s32Handle);
				}
			}

		}
	}
#endif
	for (i = 0; i < 146; i++)
	{
		LogFileCtrlDelAOldestBlockLog(s32Handle);
	}

#else
	for (i = 0; i < 10; i++)
	{
		int32_t j;
		for (j = 0; j < 24; j++)
		{
			int32_t k;
			for (k = 0; k < 60; k++)
			{
				int32_t m;
				for (m = 0; m < 6; m++)
				{
					/* 10s, a message */
					char c8Buf[128];
					c8Buf[0] = 0;
					sprintf(c8Buf, LOG_DATE_FORMAT"%s(%d)\n",
						2014, 5, i + s32Day,
						j, k, m * 10, rand() % 1000,
						"Test OOOOOOOOOOOOOOOOOOOOOOOOOOOOK!",
						m);
					LogFileCtrlWriteLog(s32Handle, c8Buf, -1);
				}
			}

		}
	}
#endif
#endif

	LogFileCtrlDestroy(s32Handle);

	PRINT("ptess enter key to exit\n");
	getchar();	return 0;
}


#elif defined USAGE_TEST
#include <common_define.h>
#include <common.h>
int main(int argc, char *argv[])
{
	int32_t s32Pid = 0;
	StSYSInfo stSYSInfo = {{0}, {0}};
	StProcessInfo stProcessInfo = {0};
	if (argc == 2)
	{
		s32Pid = atoi(argv[1]);
	}

	if (s32Pid == 0)
	{
		s32Pid = getpid();
	}


	while (1)
	{
		StProcessStat stProcessStat = {0};
		int32_t s32Err;
		CpuInfo(&(stSYSInfo.stCPU));
		MemInfo(&(stSYSInfo.stMem));


		s32Err = GetProcessStat(s32Pid, &stProcessStat);
		if (s32Err == 0)
		{
			uint64_t u64Tmp2 = stProcessInfo.u32SysTime + stProcessInfo.u32UserTime;
			uint64_t u64Tmp;
			stProcessInfo.u32SysTime = stProcessStat.u32Stime;
			stProcessInfo.u32UserTime = stProcessStat.u32Utime;
			u64Tmp = stProcessInfo.u32SysTime + stProcessInfo.u32UserTime;

			u64Tmp = (u64Tmp - u64Tmp2) * 10000 /
					(stSYSInfo.stCPU.u64Total - stSYSInfo.stCPU.u64PrevTotal);
			u64Tmp *= sysconf(_SC_NPROCESSORS_ONLN);
			stProcessInfo.u16CPUUsage = u64Tmp;
			u64Tmp2 = stProcessInfo.u16CPUAverageUsage;
			u64Tmp = u64Tmp2 * (100 - AVERAGE_WEIGHT) + u64Tmp * AVERAGE_WEIGHT;
			u64Tmp /= 100;
			stProcessInfo.u16CPUAverageUsage = u64Tmp;

			u64Tmp = stProcessInfo.u32RSS = stProcessStat.s32Rss;
			u64Tmp = ((u64Tmp / 1024 ) * 10000) / stSYSInfo.stMem.u32MemTotal;
			stProcessInfo.u16MemUsage = u64Tmp;

			u64Tmp2 = stProcessInfo.u16MemAverageUsage;
			u64Tmp = u64Tmp2 * (100 - AVERAGE_WEIGHT) + u64Tmp * AVERAGE_WEIGHT;
			u64Tmp /= 100;
			stProcessInfo.u16MemAverageUsage = u64Tmp;

			printf("Process CPU: %d.%02d, %d.%02d\n",
					stProcessInfo.u16CPUUsage / 100, stProcessInfo.u16CPUUsage % 100,
					stProcessInfo.u16CPUAverageUsage / 100, stProcessInfo.u16CPUAverageUsage % 100);

			printf("Process Mem: %d.%02d, %d.%02d\n",
					stProcessInfo.u16MemUsage / 100, stProcessInfo.u16MemUsage % 100,
					stProcessInfo.u16MemAverageUsage / 100, stProcessInfo.u16MemAverageUsage % 100);
		}
		sleep(1);
	}

	return 0;
}


#elif defined LOG_SERVER_TEST

void *ThreadLogServerTest(void *pArg)
{
	FILE *pLogFile;
	int32_t s32LogSocket = -1;


	pLogFile = fopen(LOG_FILE, "wb+");

	if (pLogFile == NULL)
	{
		PRINT("fopen %s error: %s\n", LOG_FILE, strerror(errno));
		return NULL;
	}
	s32LogSocket = ServerListen(LOG_SOCKET);
	if (s32LogSocket < 0)
	{

		PRINT("ServerListen error: 0x%08x\n", s32LogSocket);
		fclose(pLogFile);
		return NULL;
	}

	while (!g_boIsExit)
	{
        fd_set stSet;
        struct timeval stTimeout;

        int32_t s32Client = -1;
        stTimeout.tv_sec = 2;
        stTimeout.tv_usec = 0;
        FD_ZERO(&stSet);
        FD_SET(s32LogSocket, &stSet);

        if (select(s32LogSocket + 1, &stSet, NULL, NULL, &stTimeout) <= 0)
        {
            continue;
        }
        s32Client = ServerAccept(s32LogSocket);
        if (s32Client < 0)
        {
        	PRINT("ServerAccept error: 0x%08x\n", s32Client);
        	break;
        }
        else
        {
            uint32_t u32Size = 0;
            void *pMCSStream;
            pMCSStream = MCSSyncReceive(s32Client, false, 1000, &u32Size, NULL);
            if (pMCSStream != NULL)
            {
             	fwrite(pMCSStream, u32Size, 1, pLogFile);
                fflush(pLogFile);
                MCSSyncFree(pMCSStream);
            }
            close(s32Client);
        }
	}
	ServerRemove(s32LogSocket, LOG_SOCKET);
	fclose(pLogFile);
	return NULL;
}

int main(int argc, char *argv[])
{
	pthread_t s32LogServerPid = 0;
	int32_t s32Err, s32Cnt;

	system("mkdir -p "LOG_DIR);
	SignalRegister();

	s32Err = MakeThread(ThreadLogServerTest, NULL, false, &s32LogServerPid, false);

	if (s32Err != 0)
	{
		PRINT("MakeThread error: 0x%08x\n", s32Err);
		return -1;
	}
	s32Cnt = 0;
	while (!g_boIsExit)
	{
		s32Err = PrintLog("Log test %d\n", s32Cnt++);
		PRINT("PrintLog error code: 0x%08x\n", s32Err);
		sleep(1);
	}

	pthread_join(s32LogServerPid, NULL);

	return 0;
}


#elif defined SLO_TEST

static int32_t Compare(void *pLeft, void *pAdditionData, void *pRight)
{
	if (((StProcessInfo *)pLeft)->u32Pid == ((StProcessInfo *)pRight)->u32Pid)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

static int32_t CompareError(void *pLeft, void *pAdditionData, void *pRight)
{
	PRINT("the id is: %d\n", ((StProcessInfo *)pLeft)->u32Pid);
	if (((StProcessInfo *)pLeft)->u32Pid == ((StProcessInfo *)pRight)->u32Pid)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

static int32_t ComparePrint(void *pLeft, void *pAdditionData, void *pRight)
{
	PRINT("the id is: %d\n", ((StProcessInfo *)pLeft)->u32Pid);
	return 0;
}

static int32_t OperateWrite(void *pAdditionData, void *pContext)
{
	PRINT("before write, value is: %d\n", ((int32_t *)pAdditionData)[0]);
	((int32_t *)pAdditionData)[0] = 12345;
	return 0;
}

static int32_t OperateRead(void *pAdditionData, void *pContext)
{
	PRINT("value is: %d\n", ((int32_t *)pAdditionData)[0]);
	return 0;
}
int32_t OperateData(void *pData, void *pContext)
{
	((StProcessInfo *)pData)->u32Pid = 7890;
	return 0;
}

int main(int argc, char *argv[])
{
	StProcessInfo stInfo = { getpid(), "TestProcess", 0 };
	int32_t s32Handle, s32BaseNumber;
	int32_t i;
	int32_t s32Err = 0;

	if (argc != 2)
	{
		PRINT("usage: %s [basenumber]\n", argv[0]);
		s32BaseNumber = 10;
	}
	else
	{
		s32BaseNumber = atoi(argv[1]);
	}

	system("mkdir -p /tmp/workdir");

	s32Handle = SLOInit("ProcessList", 256, sizeof(StProcessInfo), 256, NULL);

	for (i = 0; i < 6; i++)
	{
		int32_t s32Index = 0;
		stInfo.u32Pid = s32BaseNumber + i;
		s32Index = SLOInsertAnEntity(s32Handle, &stInfo, NULL);
		PRINT("the entity index is: 0x%08x\n", s32Index);
	}
	PRINT("after insert\n");
	SLOTraversal(s32Handle, NULL, ComparePrint);

	SLODeleteAnEntityUsingIndexDirectly(s32Handle, 1);
	PRINT("after directly delete\n");
	SLOTraversal(s32Handle, NULL, ComparePrint);

	stInfo.u32Pid = 3 + s32BaseNumber;
	SLODeleteAnEntity(s32Handle, &stInfo, Compare);
	PRINT("after Compare delete\n");
	SLOTraversal(s32Handle, NULL, ComparePrint);


	stInfo.u32Pid = 0 + s32BaseNumber;
	for (i = 0; i < 7; i++)
	{
		PRINT("Update %dth\n", i);
		SLOUpdateAnEntity(s32Handle, &stInfo, Compare);
		SLOTraversal(s32Handle, NULL, ComparePrint);
	}

	stInfo.u32Pid = 2 + s32BaseNumber;
	PRINT("Traversal error\n");
	SLOTraversal(s32Handle, &stInfo, CompareError);

	PRINT("Traversal normally\n");
	SLOTraversal(s32Handle, NULL, ComparePrint);


	stInfo.u32Pid = 1000;
	SLOSetEntityUsingIndexDirectly(s32Handle, 0, &stInfo);
	PRINT("After change, traversal normally\n");
	SLOTraversal(s32Handle, NULL, ComparePrint);

	stInfo.u32Pid = 0;
	SLOGetEntityUsingIndexDirectly(s32Handle, 0, &stInfo);
	PRINT("Get index 0, id is: %d\n", stInfo.u32Pid);

	stInfo.u32Pid = 0;
	s32Err = SLOGetEntityUsingIndexDirectly(s32Handle, 1, &stInfo);
	PRINT("Get index 1, id is: %d, error is: 0x%08x\n", stInfo.u32Pid, s32Err);

	SLOOperateAdditionData(s32Handle, NULL, OperateWrite);
	SLOOperateAdditionData(s32Handle, NULL, OperateRead);

	PRINT("After SLOOperateAdditionData, traversal normally\n");
	SLOTraversal(s32Handle, NULL, ComparePrint);

	SLOOperateEntityUsingIndexDirectly(s32Handle, 0, NULL, OperateData);
	PRINT("After Operate, traversal normally\n");
	SLOTraversal(s32Handle, NULL, ComparePrint);


	printf("Hello World!\nPress enter key to exit!\n");
	getchar();

	SLOTombDestroy(s32Handle);

	return 0;
}
#elif defined SLO_TEST_1
#include <common.h>

typedef struct _tagStConfigInfo
{
	char     ConfigFlag;                  //是否有配置
	char     SerialNum[16];               //设备或者模块的SN

	char     TimeZone;                    //时区
	char     DaySTime;                    //夏令时
	uint32_t ConfigLen;                   //配置长度
	uint64_t ConfigTS;                    //配置TS
	uint8_t  ConfigData[512];             //配置数据
}StConfigInfo;


int32_t PrintCB(void *pLeft, void *pAdditionData, void *pRight)
{
	printf("ConfigFlag: %hhd\n", ((StConfigInfo *)pLeft)->ConfigFlag);
	return 0;
}
int main()
{
	int32_t s32Handle;
	//int32_t s32Err = 0;

	StConfigInfo stInfo = {0};
	s32Handle = SLOInit("ProcessList", 8, sizeof(StConfigInfo), 0, NULL);
	stInfo.ConfigFlag = 1;
	SLOInsertAnEntity(s32Handle, &stInfo, NULL);
	stInfo.ConfigFlag = 2;
	SLOInsertAnEntity(s32Handle, &stInfo, NULL);
	stInfo.ConfigFlag = 3;
	SLOInsertAnEntity(s32Handle, &stInfo, NULL);

	SLOTraversal(s32Handle, NULL, PrintCB);
	SLOTombDestroy(s32Handle);
	return 0;
}


#elif defined LOCK_TEST
#include "common_define.h"
#include <common.h>
int main(void)
{
	int32_t s32Handle = 0, s32Cnt = 0;
	s32Handle = LockOpen(NULL);
	while(s32Cnt < 20)
	{
		PRINT("the %dth test\n", s32Cnt++);
		LockLock(s32Handle);
		sleep(2);
		LockUnlock(s32Handle);
	}
	LockClose(s32Handle);

	puts("!!!Hello World!!!"); /* prints !!!Hello World!!! */
	return 0;
}
#else
int main()
{
	int32_t s32FD = -1;
	int32_t s32FileLength = 0;
	s32FD = open(PROJECT_NAME, O_RDONLY);
	s32FileLength = lseek(s32FD, 0, SEEK_END);
	printf("test_update file length is: %d(0x%08X)\n", s32FileLength, s32FileLength);
	printf("0x%02hhX, 0x%02hhX, 0x%02hhX, 0x%02hhX\n",
			(s32FileLength >> 24) & 0xFF,
			(s32FileLength >> 16) & 0xFF,
			(s32FileLength >> 8) & 0xFF,
			(s32FileLength >> 0) & 0xFF);
	{
		uint8_t *pTmp = (uint8_t *)(&s32FileLength);
		printf("0x%02hhX, 0x%02hhX, 0x%02hhX, 0x%02hhX\n",
				pTmp[0], pTmp[1], pTmp[2], pTmp[3]);
	}
	close(s32FD);
	printf("Hello World!\n");

	return 0;
}

#endif

