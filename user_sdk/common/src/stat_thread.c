/****************************************************************************
 * Copyright(c), 2001-2060, ******************************* 版权所有
 ****************************************************************************
 * 文件名称             : stat_thread.c
 * 版本                 : 0.0.1
 * 作者                 : 许龙杰
 * 创建日期             : 2014年5月8日
 * 描述                 : 
 ****************************************************************************/
#include "../inc/common.h"
#include "common_define.h"




#if 1

typedef struct _tagStProcessStatisticInfo
{
	StSYSInfo stSYSInfo;				/* 当前的系统状态 */
	StProcessStat stThreadStat;			/* 进程/线程的当前状态 */
	StProcessInfo stThreadInfo;			/* 保存进程名字等一些信息 */
	StHashHandle *pHandle;				/* 哈希表的句柄 */
	UnProcessInfo *pProcessInfo;		/* 在统计结束后, 检查各个进程的状态时使用*/
}StProcessStatisticInfo;


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

#if defined _DEBUG
			do
			{
				if ((pOutsideInfo->stThreadInfo.u32StatisticTimes & 0x03) == 0)
				{
					StProcessInfo *pStatisticInfo = &(pThreadInfo->stThreadInfo);
					PRINT("Process(%s_%d) CPU: %d.%02d \tMem: (%d Byte)%d.%02d\n",
							pStatisticInfo->c8Name,
							pStatisticInfo->u32Pid,
							pStatisticInfo->u16CPUAverageUsage / 100, pStatisticInfo->u16CPUAverageUsage % 100,
							pStatisticInfo->u32RSS,
							pStatisticInfo->u16MemAverageUsage / 100, pStatisticInfo->u16MemAverageUsage % 100);
				}
			}while(0);

#endif
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

/* the callback of ProcessStatisticUpdateAProcess */
static int32_t UpdateAProcessCB(int32_t s32Flag, const StHashCursor *pCursor, void *pData, void *pContext)
{
	UnProcessInfo *pInfo = (UnProcessInfo *)pData;

	pInfo->stMainProcess.u64LatestUpdateTime = TimeGetTime();
	PRINT("process(%s) latest update time is: %lld\n", pCursor->c8Key, pInfo->stMainProcess.u64LatestUpdateTime);
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
	else if (u32CmdNum == _Process_Statistic_Update)
	{
		return ProcessStatisticUpdateAProcess(pOutsideInfo->pHandle, pCmdData, UpdateAProcessCB, pContext);
	}
	return MY_ERR(_Err_CmdType);
}


void *ThreadStat(void *pArg)
{
	StProcessStatisticInfo stInfo;
	int32_t s32Err;
	int32_t s32StatisticSocket;
	uint64_t u64Time, u64Tmp;

	PRINT("PROCESS_STATISTIC_BASE_PID id %d\n", PROCESS_STATISTIC_BASE_PID);
	memset(&stInfo, 0, sizeof(StProcessStatisticInfo));

	stInfo.pHandle = ProcessStatisticInit(&s32Err);
	if (stInfo.pHandle == NULL)
	{
		PrintLog("ProcessStatisticInit error: 0x%08x\n", s32Err);
		PRINT("ProcessStatisticInit error: 0x%08x\n", s32Err);
		return NULL;
	}

	s32StatisticSocket = ServerListen(PROCESS_STATISTIC_SOCKET);
	if (s32StatisticSocket < 0)
	{

		PRINT("ServerListen error: 0x%08x\n", s32StatisticSocket);
		s32Err = s32StatisticSocket;
		goto end;
	}


	do
	{
		uint16_t u16Index = ProcessStatisticInsertAProcess(&(stInfo.pHandle), PROJECT_NAME);
		if (u16Index == HASH_INVALID_INDEX)
		{
			PRINT("ProcessStatisticInsertAProcess error: 0x%08x\n", s32Err);
		}
	}while(0);

	u64Time = TimeGetSetupTime();
	while (!g_boIsExit)
	{
		fd_set stSet;
		struct timeval stTimeout;

		int32_t s32Client = -1;
		stTimeout.tv_sec = 1;
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
		}
statistic:
		u64Tmp = TimeGetSetupTime();
		if ((u64Tmp - u64Time) < PROCESS_STATISTIC_TIME)
		{
			continue;
		}
		u64Time = u64Tmp;
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

	return NULL;

}

#endif



#if 0
static int32_t SLOTraversalUpdateInfoCallback(void *pLeft, void *pUnused, void *pRight)
{
	StProcessStat stProcessStat = {0};

	StProcessInfo *pProcessInfo = (StProcessInfo *)pLeft;
	StSYSInfo *pSYSInfo = (StSYSInfo *)pRight;
	int32_t s32Err;

	s32Err = GetProcessStat(pProcessInfo->u32Pid, &stProcessStat);
	if (s32Err != 0)
	{
		PrintLog("Process %s(%d) is exited for unknown reason\n");
		return s32Err;
	}
	if (s32Err == 0)
	{
		uint64_t u64Tmp2 = pProcessInfo->u32SysTime + pProcessInfo->u32UserTime;
		uint64_t u64Tmp;
		pProcessInfo->u32SysTime = stProcessStat.u32Stime;
		pProcessInfo->u32UserTime = stProcessStat.u32Utime;
		u64Tmp = pProcessInfo->u32SysTime + pProcessInfo->u32UserTime;

		u64Tmp = (u64Tmp - u64Tmp2) * 10000 /
				(pSYSInfo->stCPU.u64Total - pSYSInfo->stCPU.u64PrevTotal);
		u64Tmp *= sysconf(_SC_NPROCESSORS_ONLN);
		pProcessInfo->u16CPUUsage = u64Tmp;
		u64Tmp2 = pProcessInfo->u16CPUAverageUsage;
		u64Tmp = u64Tmp2 * (100 - AVERAGE_WEIGHT) + u64Tmp * AVERAGE_WEIGHT;
		u64Tmp /= 100;
		pProcessInfo->u16CPUAverageUsage = u64Tmp;

		u64Tmp = pProcessInfo->u32RSS = stProcessStat.s32Rss;
		u64Tmp = ((u64Tmp / 1024 ) * 10000) / pSYSInfo->stMem.u32MemTotal;
		pProcessInfo->u16MemUsage = u64Tmp;

		u64Tmp2 = pProcessInfo->u16MemAverageUsage;
		u64Tmp = u64Tmp2 * (100 - AVERAGE_WEIGHT) + u64Tmp * AVERAGE_WEIGHT;
		u64Tmp /= 100;
		pProcessInfo->u16MemAverageUsage = u64Tmp;

		PRINT("Process(%d) CPU: %d.%02d, %d.%02d \tMem: %d.%02d, %d.%02d\n",
				pProcessInfo->u32Pid,
				pProcessInfo->u16CPUUsage / 100, pProcessInfo->u16CPUUsage % 100,
				pProcessInfo->u16CPUAverageUsage / 100, pProcessInfo->u16CPUAverageUsage % 100,
				pProcessInfo->u16MemUsage / 100, pProcessInfo->u16MemUsage % 100,
				pProcessInfo->u16MemAverageUsage / 100, pProcessInfo->u16MemAverageUsage % 100);
	}
	return 0;
}


void *ThreadStat(void *pArg)
{
	int32_t s32SLOHandle, s32Err = 0;
#if 0
	int32_t s32Cnt = 0;
#endif
	StSYSInfo stSYSInfo = {{0}, {0}};
	s32SLOHandle = SLOInit(PEOCESS_LIST_NAME, PEOCESS_LIST_CNT, sizeof(StProcessInfo), 0, &s32Err);
	if (s32SLOHandle == 0)
	{
		PrintLog("SLOInit error 0x%08x", s32Err);
		PRINT("SLOInit error 0x%08x", s32Err);
	}
	while (!g_boIsExit)
	{
		CpuInfo(&(stSYSInfo.stCPU));
		MemInfo(&(stSYSInfo.stMem));

		s32Err = SLOTraversal(s32SLOHandle, &stSYSInfo, SLOTraversalUpdateInfoCallback);
#if 0
		s32Err = PrintLog("Log test %d\n", s32Cnt++);
		PRINT("PrintLog error code: 0x%08x\n", s32Err);
#endif
		sleep(1);
	}
#if defined _DEBUG
	SLOTombDestroy(s32SLOHandle);
#else
	SLODestroy(s32SLOHandle);
#endif
	return NULL;
}
#endif
