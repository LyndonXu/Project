/****************************************************************************
 * Copyright(c), 2001-2060, ******************************* 版权所有
 ****************************************************************************
 * 文件名称             : log_thread.c
 * 版本                 : 0.0.1
 * 作者                 : 许龙杰
 * 创建日期             : 2014年5月7日
 * 描述                 : 
 ****************************************************************************/
#include "../inc/common.h"
#include "common_define.h"


#if 1
#if 0
static int32_t MCSCallBack(uint32_t u32CmdNum, uint32_t u32CmdCnt, uint32_t u32CmdSize,
        const char *pCmdData, void *pContext)
{
	int32_t s32LogHandle = (int32_t)pContext;

    if ((u32CmdNum == 0x00006001) || (u32CmdNum == 0x00006002) || (u32CmdNum == 0))
	{
    	LogFileCtrlWriteLog(s32LogHandle, pCmdData, u32CmdSize * u32CmdCnt);
	}
    else
	{
		PRINT("error command: 0x%08x\n", u32CmdNum);
		return -1;
	}
    return 0;
}
#endif
void *ThreadLogServer(void *pArg)
{
	int32_t s32LogHandle, s32Err = 0;
	int32_t s32LogSocket = -1;

	s32LogHandle = LogFileCtrlInit(LOG_FILE, &s32Err);

	if (s32LogHandle == 0)
	{
		PRINT("LogFileCtrlInit error: 0x%08x\n", s32Err);
		return NULL;
	}
	s32LogSocket = ServerListen(LOG_SOCKET);
	if (s32LogSocket < 0)
	{

		PRINT("ServerListen error: 0x%08x\n", s32LogSocket);
		LogFileCtrlDestroy(s32LogHandle);
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
		if (!FD_ISSET(s32LogSocket, &stSet))
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
			int32_t s32Err = 0;
			void *pMCSStream;
			pMCSStream = MCSSyncReceive(s32Client, false, 1000, &u32Size, &s32Err);
			if (pMCSStream != NULL)
			{
				LogFileCtrlWriteLog(s32LogHandle, pMCSStream, u32Size);
#if 0
				PRINT("msg: %s\n", (char *)pMCSStream);
				MCSResolve((const char *)pMCSStream, u32Size, MCSCallBack, (void *)s32LogHandle);
#endif
				MCSSyncFree(pMCSStream);
			}
			else
			{
				PRINT("MCSSyncReceive error 0x%08x\n", s32Err);
			}
			close(s32Client);
		}
	}
	ServerRemove(s32LogSocket, LOG_SOCKET);
	LogFileCtrlDestroy(s32LogHandle);
	return NULL;
}
#else
static int32_t MCSCallBack(uint32_t u32CmdNum, uint32_t u32CmdCnt, uint32_t u32CmdSize,
        const char *pCmdData, void *pContext)
{
    FILE *pLogFile = (FILE *)pContext;

    if ((u32CmdNum == 0x00006001) || (u32CmdNum == 0x00006002))
	{
		fwrite(pCmdData, u32CmdSize, u32CmdCnt, pLogFile);
	}
    else
	{
		fprintf(pLogFile, "error command: 0x%08x\n", u32CmdNum);
	}
    fflush(pLogFile);
    return 0;
}


void *ThreadLogServer(void *pArg)
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
            pMCSStream = MCSSyncReceive(s32Client, 1000, &u32Size, NULL);
            if (pMCSStream != NULL)
            {
                MCSResolve((const char *)pMCSStream, u32Size, MCSCallBack, pLogFile);
                MCSSyncFree(pMCSStream);
            }
            close(s32Client);
        }
	}
	ServerRemove(s32LogSocket, LOG_SOCKET);
	fclose(pLogFile);
	return NULL;
}
#endif


