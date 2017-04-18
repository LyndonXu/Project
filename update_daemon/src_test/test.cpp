/*
 * test.cpp
 *
 *  Created on: 2017年4月5日
 *      Author: lyndon
 */

#include <unistd.h>
#include "update_daemon.h"
//#define SERCHER_TEST
#define UPDATE_TEST

#if defined UPDATE_TEST
#include <sys/un.h>
#include <sys/socket.h>

const UnBteaKey c_unUpdateKey[_Update_Mode_Reserved] =
{
	{.c8Key = "update mode exe"},
	{.c8Key = "mode gui config"},
	{.c8Key = "mode driver****"},
	{.c8Key = "mode thirdpaty*"},
};
int32_t GetVersion(int32_t s32Socket, EmUpdateMode emUpdateMode)
{
	StUpdateMode stMode;
	int32_t s32Err = 0;
	uint8_t *pMCS;
	uint32_t u32RecvSize = 0;
	stMode.emMode = emUpdateMode;
	for (int32_t i = 0; i < 16; i++)
	{
		stMode.u8RandCode[i] = stMode.u8Rand[i] = rand();
	}
	btea((int32_t *)stMode.u8RandCode, 4,(int32_t *)(c_unUpdateKey[emUpdateMode].s32Key));

	s32Err = MCSSyncSend(s32Socket, 1000, _TCP_Cmd_Update_GetVersion, sizeof(StUpdateMode), &stMode);
	if (s32Err != 0)
	{
		PRINT("MCSSyncSend error: %08x\n", s32Err);
		return s32Err;
	}

	pMCS = (uint8_t *)MCSSyncReceive(s32Socket, false, 1000, &u32RecvSize, &s32Err);
	if (pMCS == NULL)
	{
		PRINT("MCSSyncReceive error: %08x\n", s32Err);
		return s32Err;
	}

	do
	{
		uint32_t u32Cmd, u32Cnt, u32Length;
		uint8_t *pData = pMCS;
		/* command */
		LittleAndBigEndianTransfer((char *)(&u32Cmd), (const char *)pData, sizeof(uint32_t));
		pData += sizeof(uint32_t);
		/* count */
		LittleAndBigEndianTransfer((char *)(&u32Cnt), (const char *)pData, sizeof(uint32_t));
		pData += sizeof(uint32_t);
		/* length */
		LittleAndBigEndianTransfer((char *)(&u32Length), (const char *)pData, sizeof(uint32_t));
		pData += sizeof(uint32_t);
		PRINT("cmd %08x, cnt: %d, length %d\n", u32Cmd, u32Cnt, u32Length);

		if (u32Cmd == (_MCS_Cmd_Echo | _TCP_Cmd_Update_GetVersion))
		{
			StUpdateVersion *pVersion = (StUpdateVersion *)pData;
			for (uint32_t i = 0; i < u32Cnt; i++, pVersion++)
			{
				PRINT("\nname: %s, version: %d\n", pVersion->c8Name, pVersion->s32Version);
			}
		}
	} while (0);

	MCSSyncFree(pMCS);

	return s32Err;
}

int32_t UpdateSeleteMode(int32_t s32Socket, EmUpdateMode emUpdateMode)
{
	StUpdateMode stMode;
	int32_t s32Err = 0;
	uint8_t *pMCS;
	uint32_t u32RecvSize = 0;
	stMode.emMode = emUpdateMode;
	for (int32_t i = 0; i < 16; i++)
	{
		stMode.u8RandCode[i] = stMode.u8Rand[i] = rand();
	}
	btea((int32_t *)stMode.u8RandCode, 4,(int32_t *)(c_unUpdateKey[emUpdateMode].s32Key));

	s32Err = MCSSyncSend(s32Socket, 1000, _TCP_Cmd_Update_Mode, sizeof(StUpdateMode), &stMode);
	if (s32Err != 0)
	{
		PRINT("MCSSyncSend error: %08x\n", s32Err);
		return s32Err;
	}

	pMCS = (uint8_t *)MCSSyncReceive(s32Socket, false, 1000, &u32RecvSize, &s32Err);
	if (pMCS == NULL)
	{
		PRINT("MCSSyncReceive error: %08x\n", s32Err);
		return s32Err;
	}

	do
	{
		uint32_t u32Cmd, u32Cnt, u32Length;
		uint8_t *pData = pMCS;
		/* command */
		LittleAndBigEndianTransfer((char *)(&u32Cmd), (const char *)pData, sizeof(uint32_t));
		pData += sizeof(uint32_t);
		/* count */
		LittleAndBigEndianTransfer((char *)(&u32Cnt), (const char *)pData, sizeof(uint32_t));
		pData += sizeof(uint32_t);
		/* length */
		LittleAndBigEndianTransfer((char *)(&u32Length), (const char *)pData, sizeof(uint32_t));
		pData += sizeof(uint32_t);
		PRINT("cmd %08x, cnt: %d, length %d\n", u32Cmd, u32Cnt, u32Length);

		if (u32Cmd != (_MCS_Cmd_Echo | _TCP_Cmd_Update_Mode))
		{
			s32Err = u32Cmd;
		}
	} while (0);
	MCSSyncFree(pMCS);
	return s32Err;
}

int32_t UpdateSendFileName(int32_t s32Socket, const char *pFileName)
{
	int32_t s32Err = 0;
	uint8_t *pMCS;
	uint32_t u32RecvSize = 0;
	s32Err = MCSSyncSend(s32Socket, 1000, _TCP_Cmd_Update_Name, strlen(pFileName) + 1, pFileName);
	if (s32Err != 0)
	{
		PRINT("MCSSyncSend error: %08x\n", s32Err);
		return s32Err;
	}

	pMCS = (uint8_t *)MCSSyncReceive(s32Socket, false, 1000, &u32RecvSize, &s32Err);
	if (pMCS == NULL)
	{
		PRINT("MCSSyncReceive error: %08x\n", s32Err);
		return s32Err;
	}

	do
	{
		uint32_t u32Cmd, u32Cnt, u32Length;
		uint8_t *pData = pMCS;
		/* command */
		LittleAndBigEndianTransfer((char *)(&u32Cmd), (const char *)pData, sizeof(uint32_t));
		pData += sizeof(uint32_t);
		/* count */
		LittleAndBigEndianTransfer((char *)(&u32Cnt), (const char *)pData, sizeof(uint32_t));
		pData += sizeof(uint32_t);
		/* length */
		LittleAndBigEndianTransfer((char *)(&u32Length), (const char *)pData, sizeof(uint32_t));
		pData += sizeof(uint32_t);
		PRINT("cmd %08x, cnt: %d, length %d\n", u32Cmd, u32Cnt, u32Length);

		if (u32Cmd != (_MCS_Cmd_Echo | _TCP_Cmd_Update_Name))
		{
			s32Err = u32Cmd;
		}
	} while (0);
	MCSSyncFree(pMCS);
	return s32Err;
}

int32_t UpdateSendFile(int32_t s32Socket, const char *pFileName)
{
	int32_t s32Err = 0;
	uint8_t *pMCS;
	uint32_t u32RecvSize = 0;
	s32Err = MCSSyncSendFile(s32Socket, 100 * 1000, _TCP_Cmd_Update_File, pFileName);
	if (s32Err != 0)
	{
		PRINT("MCSSyncSend error: %08x\n", s32Err);
		return s32Err;
	}

	pMCS = (uint8_t *)MCSSyncReceive(s32Socket, false, 1000, &u32RecvSize, &s32Err);
	if (pMCS == NULL)
	{
		PRINT("MCSSyncReceive error: %08x\n", s32Err);
		return s32Err;
	}

	do
	{
		uint32_t u32Cmd, u32Cnt, u32Length;
		uint8_t *pData = pMCS;
		/* command */
		LittleAndBigEndianTransfer((char *)(&u32Cmd), (const char *)pData, sizeof(uint32_t));
		pData += sizeof(uint32_t);
		/* count */
		LittleAndBigEndianTransfer((char *)(&u32Cnt), (const char *)pData, sizeof(uint32_t));
		pData += sizeof(uint32_t);
		/* length */
		LittleAndBigEndianTransfer((char *)(&u32Length), (const char *)pData, sizeof(uint32_t));
		pData += sizeof(uint32_t);
		PRINT("cmd %08x, cnt: %d, length %d\n", u32Cmd, u32Cnt, u32Length);

		if (u32Cmd != (_MCS_Cmd_Echo | _TCP_Cmd_Update_File))
		{
			s32Err = u32Cmd;
		}
	} while (0);
	MCSSyncFree(pMCS);
	return s32Err;
}


int main(int argc, char * const argv[])
{
	int32_t s32Socket = -1;
	int32_t s32Len, s32Err = 0;
	struct sockaddr_in stAddr;
	const char *pAddr = "127.0.0.1";
	const char *pFileName = "test_version.tar";
	const char *pFileRealName = "/home/ubuntu/workspace/Project/update_daemon/test_version.tar";

	EmUpdateMode emUpdateMode = _Update_Mode_Driver;

	if ((s32Socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		return MY_ERR(_Err_SYS + errno);
	}
	memset(&stAddr, 0, sizeof(stAddr));
	stAddr.sin_family = AF_INET;
	stAddr.sin_port = htons(TCP_UPDATE_SERVER_PORT);
	stAddr.sin_addr.s_addr = inet_addr(pAddr);
	s32Len = sizeof(stAddr);

	if (connect(s32Socket, (struct sockaddr*) (&stAddr), s32Len) < 0)
	{
		PRINT("error: %s\n", strerror(errno));
		s32Err = MY_ERR(_Err_SYS + errno);

		close(s32Socket);
		return s32Err;
	}

	do
	{
		s32Err = GetVersion(s32Socket, emUpdateMode);
		if (s32Err != 0)
		{
			PRINT("error: %08x\n", s32Err);
			break;
		}
		s32Err = UpdateSeleteMode(s32Socket, emUpdateMode);
		if (s32Err != 0)
		{
			PRINT("error: %08x\n", s32Err);
			break;
		}

		s32Err = UpdateSendFileName(s32Socket, pFileName);
		if (s32Err != 0)
		{
			PRINT("error: %08x\n", s32Err);
			break;
		}

		s32Err = UpdateSendFile(s32Socket, pFileRealName);
		if (s32Err != 0)
		{
			PRINT("error: %08x\n", s32Err);
			break;
		}
	} while (0);



	getchar();

	close(s32Socket);
	return s32Err;
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
