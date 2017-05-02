/*
 * inner.cpp
 *
 *  Created on: 2017年5月2日
 *      Author: ubuntu
 */

#include "cmd_communication.h"

void *ThreadUnixCmd(void *pArg)
{
	int32_t s32Server = ServerListen(UNIX_SOCKET_NAME);
	StThreadArg *pThreadArg = (StThreadArg *)pArg;
	CSockCtrl *pCtrl = pThreadArg->pCtrl;

	if (s32Server < 0)
	{
		PRINT("ServerListen: %s, error: %08x\n", UNIX_SOCKET_NAME, s32Server);
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
				const char *pMCS = (const char *)pMCSStream;
				const char *pMCSEnd = pMCS + u32Size;
				while (pMCS < pMCSEnd)
				{
					uint32_t u32Cmd, u32Count, u32ElementSize;
					uint32_t u32DataLength;
			        LittleAndBigEndianTransfer((char *)&u32Cmd, pMCS, sizeof(uint32_t));   /* 命令号 */
			        pMCS += sizeof(uint32_t);

			        LittleAndBigEndianTransfer((char *)&u32Count, pMCS, sizeof(uint32_t));   /* 此命令号命令的数量 */
			        pMCS += sizeof(uint32_t);

			        LittleAndBigEndianTransfer((char *)&u32ElementSize, pMCS, sizeof(uint32_t));   /* 每一个命令的大小 */
			        pMCS += sizeof(uint32_t);

			        u32DataLength = u32Count * u32ElementSize;
			        if (u32DataLength != 0)
			        {
						switch (u32Cmd)
						{
							case _Unix_Cmd_Com_Send_Data:
							{
								int32_t *pSock;
								int32_t s32Count = 0;
								pSock = pCtrl->GetSocket(s32Count);
								if ((s32Count == 0) || (pSock == NULL))
								{
									break;
								}
								else
								{
									CMallocAutoRelease CAutoSock(pSock);
									for (int32_t i = 0; i < s32Count; i++)
									{
										int32_t s32SendCnt = send(pSock[i], pMCS, u32DataLength, MSG_NOSIGNAL);
										if (s32SendCnt != (int32_t)u32DataLength)
										{
											PRINT("send cmd error(%d), %08x\n", pSock[i], s32SendCnt);
											pCtrl->DeleteASocket(pSock[i]);
										}
										else
										{
											PRINT("send(%d) some data %d\n",  pSock[i], s32SendCnt);
										}
									}
								}
								break;
							}
							default:
								break;
						}
			        }

			        pMCS += u32DataLength;

				}
				MCSSyncFree(pMCSStream);
			}
			else
			{
				PRINT("MCSSyncReceive error 0x%08x\n", s32Err);
			}
			close(s32Client);
		}
	}

	close(s32Server);

	return NULL;

}


