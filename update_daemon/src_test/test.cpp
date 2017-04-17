/*
 * test.cpp
 *
 *  Created on: 2017年4月5日
 *      Author: lyndon
 */

#include <unistd.h>
#include "update_daemon.h"
//#define SERCHER_TEST
//#define CHEKCK_VERSION

#if defined CHEKCK_VERSION
int main()
{
	int32_t CheckVersion(const char *pLink, const char *pDir, bool boAutoLink);
	return CheckVersion(NULL, NULL, false);
}
#elif defined SERCHER_TEST
char c8Buf[4096];

int main(int argc, char * const argv[])
{
	int32_t s32Socket = -1;
	int32_t s32Err = 0;
	uint8_t *pMCS;
	uint32_t u32Size = 0;
	uint32_t u32Cmd = -1, u32Length = -1;
	const char *pDataIn = NULL;
	bool boNeedEcho = false;
	int32_t s32Char;
	uint8_t *pData = NULL;

	while ((s32Char = getopt(argc, argv, "c:l:d:o")) != -1)
	{
		switch (s32Char)
		{
			case 'c':
			{
				sscanf(optarg, "%x", &u32Cmd);
				break;
			}
			case 'd':
			{
				pDataIn = optarg;
				break;
			}
			case 'l':
			{
				sscanf(optarg, "%d", &u32Length);
				break;
			}
			case 'o':
			{
				boNeedEcho = true;
				break;
			}
            default: /* '?' */
                fprintf(stderr, "Usage: %s [-c command number] [-l length] "
                		"[-d data] [-o need print echo]\n", argv[0]);
				return -1;
		}
	}


	if (u32Length == (uint32_t)(-1))
	{
		if (pDataIn == NULL)
		{
			u32Length = 0;
		}
		else
		{
			u32Length = strlen(pDataIn);
		}
	}

	if ((pDataIn != NULL && (int32_t)u32Length > 0))
	{
		pData = (uint8_t *)malloc(u32Length);
		if (pData == NULL)
		{
			return -1;
		}
	}


	switch (u32Cmd)
	{
		case _UDP_Cmd_GetEthInfo:
		{
			break;
		}
		default:
			u32Cmd = _UDP_Cmd_GetEthInfo;
			if (pData != NULL)
			{
				memcpy(pData, pDataIn, u32Length);
			}
			break;
	}

	s32Socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (s32Socket < 0)
	{
		PRINT("socket error: %s\n", strerror(errno));
		return -1;
	}


	{
		int32_t s32Enable = 1;
		s32Err = setsockopt(s32Socket, SOL_SOCKET, SO_BROADCAST, &s32Enable, sizeof(int32_t));
		if (s32Err < 0)
		{
			PRINT("setsockopt: %08x\n", errno);
			close(s32Socket);
			return -1;
		}
	}

	pMCS = (uint8_t *)MCSMakeAnArrayVarialbleCmd(u32Cmd, pData, 1, u32Length, &u32Size);
	if (pData != NULL)
	{
		free(pData);
	}
	if (pMCS != NULL)
	{
		struct sockaddr_in stAddr = {0};
		stAddr.sin_family = AF_INET;
		stAddr.sin_port = htons(UDP_SERVER_PORT);
		stAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); //inet_addr("192.168.247.135"); //htonl(INADDR_BROADCAST);//
		s32Err = sendto(s32Socket, pMCS, u32Size,
				MSG_NOSIGNAL, (struct sockaddr *)(&stAddr), sizeof(struct sockaddr));
		if (s32Err < 0)
		{
			PRINT("UDP send message error: %s\n", strerror(errno));
		}

		MCSFree(pMCS);
	}

	if (boNeedEcho)
	{
		struct sockaddr_in stAddr = {0};
		socklen_t s32Len = sizeof(struct sockaddr_in);
		int32_t s32RecvLen = recvfrom(s32Socket, c8Buf, 4096,
				0, (struct sockaddr *)(&stAddr), &s32Len);
		if (s32RecvLen > 0 && (s32Len == sizeof (struct sockaddr_in)))
		{
			for (int32_t i = 0; i < s32RecvLen; i++)
			{
				if ((i & 0x0F) == 0)
				{
					printf("\n");
				}
				printf("%02hhx ", c8Buf[i]);
			}
			printf("\n");

		}
	}

	close(s32Socket);

	return 0;

}

#else
int main()
{
	printf("test ok!\n");
	return 0;
}
#endif
