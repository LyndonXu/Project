/*
 * tcp.cpp
 *
 *  Created on: 2017年4月15日
 *      Author: lyndon
 */

#include "update_daemon.h"

const UnBteaKey c_unUpdateKey[_Update_Mode_Reserved] =
{
	{.c8Key = "update mode exe"},
	{.c8Key = "mode gui config"},
	{.c8Key = "mode driver****"},
	{.c8Key = "mode thirdpaty*"},
};

const char *c_pUpdateDir[_Update_Mode_Reserved] =
{
	EXE_DIR,
	GUI_CONFIG_DIR,
	DRIVER_DIR,
	THIRD_PARTY_DIR,
};

int32_t WriteFileCB(void *pData, uint32_t u32Len, void *pContext)
{
	return fwrite(pData, 1, u32Len, (FILE *)pContext);
}

void *ThreadTCPUpdate(void *pArg)
{
	int32_t s32Socket = -1;
	int32_t s32Err = 0;

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
	    stAddr.sin_port = htons(TCP_UPDATE_SERVER_PORT);

	    if(bind(s32Socket,(struct sockaddr *)&(stAddr), sizeof(struct sockaddr_in)) == -1)
	    {
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
			EmUpdateMode emUpdateMode = _Update_Mode_Reserved;
			char c8FileName[_POSIX_PATH_MAX] = "/tmp/";
			FILE *pFile = NULL;
			uint32_t u32State = _TCP_Cmd_Update_Mode;
			uint32_t u32ErrCnt = 0;
			bool boCanGetOut = false;
			bool boNeedReboot = false;
			int32_t s32Client = ServerAccept(s32Socket);
			if (s32Client < 0)
			{
				continue;
			}
			while ((!g_boIsExit) && (u32ErrCnt < 5) && (!boCanGetOut))
			{
				if (u32State <= _TCP_Cmd_Update_Name)
				{
					uint32_t u32Cmd, u32Cnt, u32Length;
					uint8_t *pData;

					uint32_t u32RecvSize = 0;
					uint8_t *pMCS = (uint8_t *)MCSSyncReceiveWithLimit(s32Client, true, 1024,
							5000, &u32RecvSize, &s32Err);
					if (pMCS == NULL)
					{
						break;
					}

					u32ErrCnt++;

					pData = pMCS + 16; /* mcs header */
					/* command */
					LittleAndBigEndianTransfer((char *)(&u32Cmd), (const char *)pData, sizeof(uint32_t));
					pData += sizeof(uint32_t);
					/* count */
					LittleAndBigEndianTransfer((char *)(&u32Cnt), (const char *)pData, sizeof(uint32_t));
					pData += sizeof(uint32_t);
					/* length */
					LittleAndBigEndianTransfer((char *)(&u32Length), (const char *)pData, sizeof(uint32_t));
					pData += sizeof(uint32_t);

					if (u32Cmd == _TCP_Cmd_Update_GetVersion)
					{
						/* <TODO> */
						MCSSyncSend(s32Client, 2000, _MCS_Cmd_Echo | _TCP_Cmd_Update_GetVersion,
								0, NULL);
						break;
					}
					else if (u32Cmd != u32State)
					{
						MCSSyncSend(s32Client, 2000, MY_ERR(_Err_CmdType), sizeof(uint32_t), &u32State);
						continue;
					}

					if (u32State == _TCP_Cmd_Update_Mode)
					{
						if (u32Cnt == 1 && u32Length == sizeof(StUpdateMode))
						{
							StUpdateMode *pMode = (StUpdateMode *)pData;
							if (pMode->emMode >= _Update_Mode_Reserved)
							{
								MCSSyncSend(s32Client, 2000, MY_ERR(_Err_InvalidParam), 0, NULL);
								continue;
							}
							btea((int32_t *)pMode->u8Rand, 4,
									(int32_t *)(c_unUpdateKey[pMode->emMode].s32Key));
							if (memcmp(pMode->u8Rand, pMode->u8RandCode, 16) != 0)
							{
								MCSSyncSend(s32Client, 2000, MY_ERR(_Err_InvalidParam), 0, NULL);
								continue;
							}

							emUpdateMode = pMode->emMode;
							u32State = _TCP_Cmd_Update_Name;
							u32ErrCnt = 0;
							PRINT("begin get the file name\n");
						}
						else
						{
							MCSSyncSend(s32Client, 2000, MY_ERR(_Err_CmdLen), 0, NULL);
							continue;
						}
						break;
					}
					else
					{
						if (u32Cnt == 1 && u32Length < 196)
						{
							uint32_t u32StrLen = strlen(c8FileName);
							memcpy(c8FileName + u32StrLen, pData, u32Length);
							u32StrLen += u32Length;
							c8FileName[u32StrLen] = 0;
							PRINT("open file %s\n", c8FileName);
							pFile = fopen(c8FileName, "wb+");
							if (pFile == NULL)
							{
								MCSSyncSend(s32Client, 2000, MY_ERR(_Err_InvalidParam), 0, NULL);
								continue;
							}
						}
						else
						{
							MCSSyncSend(s32Client, 2000, MY_ERR(_Err_CmdLen), 0, NULL);
							continue;
						}

						if (pFile == NULL)
						{
							continue;
						}

						u32State = _TCP_Cmd_Update_File;
						u32ErrCnt = 0;
					}
					MCSSyncFree(pMCS);

				}
				else if (u32State == _TCP_Cmd_Update_File)
				{
					/* receive the file */
					PRINT("begin receive the file\n");

					s32Err = MCSSyncReceiveCmdWithCB(s32Socket, _TCP_Cmd_Update_File, 5000,
							WriteFileCB, pFile);

					if (s32Err != 0)
					{
						continue;
					}

					u32ErrCnt = 0;

					fclose(pFile);
					pFile = NULL;
					do
					{
						char c8Buf[512];
						PRINT("begin uncompress the file into %s\n", c_pUpdateDir[emUpdateMode]);
						sprintf(c8Buf, "mkdir -p %s", c_pUpdateDir[emUpdateMode]);
						system(c8Buf);

						sprintf(c8Buf, "tar -xf %s -C %s", c8FileName, c_pUpdateDir[emUpdateMode]);
						system(c8Buf);

						/* link, check, write configure and delete oldest version */

					} while (0);
				}
				else
				{
					u32State = _TCP_Cmd_Update_Mode;
				}

			}
			close(s32Client);

			if (pFile != NULL)
			{
				fclose(pFile);
			}

			if (u32State == _TCP_Cmd_Update_Name ||
					u32State ==_TCP_Cmd_Update_File)
			{
				char c8Buf[256];
				sprintf(c8Buf, "rm -rf %s", c8FileName);
				system(c8Buf);
			}

			if (boNeedReboot)
			{
				system("reboot");
			}

		}
	}

	if (s32Socket >= 0)
	{
		close(s32Socket);
	}
	return NULL;
}


