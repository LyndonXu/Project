/****************************************************************************
 * Copyright(c), 2001-2060, ************************** 版权所有
 ****************************************************************************
 * 文件名称             : stat.c
 * 版本                 : 0.0.1
 * 作者                 : 许龙杰
 * 创建日期             : 2014年04月08日
 * 描述                 :
 ****************************************************************************/

#define NOT_USING_PRINT

#include <common.h>
#include "common_define.h"


const char *c_pMemInfoChar[] =
{
	"MemTotal",
	"MemFree",
	"Buffers",
	"Cached",
};

/*
 * 函数名      : MemInfo
 * 功能        : 得到当前内存使用情况
 * 参数        : pMemInfo[in/out] (StMemInfo): 详见定义
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t MemInfo(StMemInfo *pMemInfo)
{
	FILE *pFileMemInfo = NULL;

	if (pMemInfo == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}

	pFileMemInfo = fopen("/proc/meminfo", "r");
	if (pFileMemInfo == NULL)
	{
		PRINT("file open /proc/meminfo error %s\n", strerror(errno));
		return MY_ERR(_Err_SYS + errno);
	}

	{
		int32_t i = 0;
		uint32_t *pTmp = (uint32_t *)(pMemInfo);
		char c8Char[64];

		for (i = 0; i < MEM_INFO_CNT; i++)
		{
			sprintf(c8Char, "%s: %%u kB\n", c_pMemInfoChar[i]);
			fscanf(pFileMemInfo, c8Char, pTmp + i);
			//PRINT("%s: %u kB\n", c_pMemInfoChar[i], pTmp[i]);
		}
		PRINT("MemUsed: %u kB\n", pMemInfo->u32MemTotal - pMemInfo->u32MemFree);
	}

	fclose(pFileMemInfo);
	{
		pMemInfo->u32Usage = (pMemInfo->u32MemTotal - pMemInfo->u32MemFree -
							pMemInfo->u32Buffers - pMemInfo->u32Cached) * 10000 / pMemInfo->u32MemTotal;
		pMemInfo->u32AverageUsage = pMemInfo->u32AverageUsage * (100 - AVERAGE_WEIGHT) +
									pMemInfo->u32Usage * AVERAGE_WEIGHT;
		pMemInfo->u32AverageUsage /= 100;

	}
	PRINT("MEM: %d.%02d, %d.%02d\n",
			pMemInfo->u32Usage / 100, pMemInfo->u32Usage % 100,
			pMemInfo->u32AverageUsage / 100, pMemInfo->u32AverageUsage % 100);

	return 0;
}


/*
 * 函数名      : CpuInfo
 * 功能        : 得到当前CPU使用情况
 * 参数        : pJiffies[in/out] (StJiffies): 详见定义
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t CpuInfo(StJiffies *pJiffies)
{
	FILE *pFileStat = NULL;
	uint64_t u64Tmp;
	uint64_t u64Total;
	if (pJiffies == NULL)
	{
		PRINT("Argument error\n");
		return MY_ERR(_Err_InvalidParam);
	}
	pFileStat = fopen("/proc/stat", "r");
	if (pFileStat == NULL)
	{
		PRINT("file open /proc/stat error %s\n", strerror(errno));
		return MY_ERR(_Err_SYS + errno);
	}
	u64Tmp = pJiffies->u64Idle;
	u64Total = pJiffies->u64PrevTotal = pJiffies->u64Total;

	fscanf(pFileStat, "cpu  %llu %llu %llu %llu %llu %llu %llu %llu",
			&pJiffies->u64Usr, &pJiffies->u64Nice,
			&pJiffies->u64Sys, &pJiffies->u64Idle,
			&pJiffies->u64IOWait, &pJiffies->u64Irq,
			&pJiffies->u64SoftIrq, &pJiffies->u64Streal);
	fclose(pFileStat);

	{
		int32_t i;
		uint64_t *pTmp = (uint64_t *)pJiffies;
		pJiffies->u64Total = 0;
		for (i = 0; i < CPU_INFO_CNT; i++)
		{
			pJiffies->u64Total += pTmp[i];
		}
	}

	u64Tmp = pJiffies->u64Idle - u64Tmp;
	u64Total = pJiffies->u64Total - u64Total;
	u64Tmp = (u64Total - u64Tmp) * 10000 / u64Total;

	pJiffies->u32Usage = u64Tmp;
	pJiffies->u32AverageUsage = pJiffies->u32AverageUsage * (100 - AVERAGE_WEIGHT) +
								pJiffies->u32Usage * AVERAGE_WEIGHT;
	pJiffies->u32AverageUsage /= 100;

	PRINT("CPU: %d.%02d, %d.%02d\n",
			pJiffies->u32Usage / 100, pJiffies->u32Usage % 100,
			pJiffies->u32AverageUsage / 100, pJiffies->u32AverageUsage % 100);

	return 0;
}

/*
 * 函数名      : GetStatFileInfo
 * 功能        : 得到stat文件的信息
 * 参数        : pStatFile [in] (const char *): stat文件的全路径
 * 			     pProcessStat[in/out] (StProcessStat): 详见定义
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t GetStatFileInfo(const char *pStatFile, StProcessStat *pProcessStat)
{
	FILE *pFileProcessStat = NULL;
	uint32_t u32Size = 0;

	if((pStatFile == NULL) || (pProcessStat == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}

	pFileProcessStat = fopen(pStatFile, "r");
	if (pFileProcessStat == NULL)
	{
		PRINT("file open %s error %s\n", pStatFile, strerror(errno));
		return MY_ERR(_Err_SYS + errno);
	}

	fscanf(pFileProcessStat,
			"%d %*s "
			"%c "
			"%*d %*d %*d %*d %*d "
			"%*u "
			"%*u %*u %*u %*u %u %u "
			"%*d %*d %*d %*d %*d %*d "
			"%*u "
			"%u "
			"%d",
			&pProcessStat->s32Pid, /*pProcessStat->c8Name,*/
			&pProcessStat->c8State,
			/*
			 * &pProcessStat->s32Ppid,&pProcessStat->s32Pgrp,&pProcessStat->s32Session,&pProcessStat->s32Tty_nr,&pProcessStat->s32Tpgid, //5
			 * &pProcessStat->u32Flags,//1
			 * &pProcessStat->u32Minflt,&pProcessStat->u32Cminflt,&pProcessStat->u32Majflt,&pProcessStat->u32Cmajflt,&pProcessStat->u32Utime,&pProcessStat->u32Stime,//6
			 */
			&pProcessStat->u32Utime,&pProcessStat->u32Stime,
			/*
			 * &pProcessStat->s32Cutime,&pProcessStat->s32Cstime,&pProcessStat->s32Priority,&pProcessStat->s32Nice,&pProcessStat->s32Threads,&pProcessStat->s32Iterealvalue, //6
			 * &pProcessStat->u64Starttime,
			 */
			&pProcessStat->u32Vsize,
			&pProcessStat->s32Rss);
	fclose(pFileProcessStat);
	u32Size = sysconf(_SC_PAGESIZE);
	pProcessStat->s32Rss *= u32Size;
	PRINT("%d %c utime: %u stime:%u vsize: %u rss %d\n",
			pProcessStat->s32Pid, pProcessStat->c8State,
			pProcessStat->u32Utime, pProcessStat->u32Stime,
			pProcessStat->u32Vsize, pProcessStat->s32Rss);
	return 0;
}

/*
 * 函数名      : GetProcessStat
 * 功能        : 得到当前某一个进程信息
 * 参数        : s32Pid [in] (pid_t): 进程PID
 * 			     pProcessStat[in/out] (StProcessStat): 详见定义
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t GetProcessStat(pid_t s32Pid, StProcessStat *pProcessStat)
{
#if 1
	char c8Buf[_POSIX_PATH_MAX];
	if((s32Pid < 0) || (pProcessStat == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}

	sprintf(c8Buf, "/proc/%d/stat", s32Pid);
	return GetStatFileInfo(c8Buf, pProcessStat);
#else
	FILE *pFileProcessStat = NULL;
	char c8Buf[_POSIX_PATH_MAX];
#if 0
	int32_t s32Size = 0;
#endif
	uint32_t u32Size = 0;
	sprintf(c8Buf, "/proc/%d/stat", s32Pid);
	pFileProcessStat = fopen(c8Buf, "r");
	if (pFileProcessStat == NULL)
	{
		PRINT("file open /proc/%d/stat error %s", s32Pid, strerror(errno));
		return MY_ERR(_Err_SYS + errno);
	}

	fscanf(pFileProcessStat,
			"%d %*s "
			"%c "
			"%*d %*d %*d %*d %*d "
			"%*u "
			"%*u %*u %*u %*u %u %u "
			"%*d %*d %*d %*d %*d %*d "
			"%*u "
			"%u "
			"%d",
			&pProcessStat->s32Pid, /*pProcessStat->c8Name,*/
			&pProcessStat->c8State,
			/*
			 * &pProcessStat->s32Ppid,&pProcessStat->s32Pgrp,&pProcessStat->s32Session,&pProcessStat->s32Tty_nr,&pProcessStat->s32Tpgid, //5
			 * &pProcessStat->u32Flags,//1
			 * &pProcessStat->u32Minflt,&pProcessStat->u32Cminflt,&pProcessStat->u32Majflt,&pProcessStat->u32Cmajflt,&pProcessStat->u32Utime,&pProcessStat->u32Stime,//6
			 */
			&pProcessStat->u32Utime,&pProcessStat->u32Stime,
			/*
			 * &pProcessStat->s32Cutime,&pProcessStat->s32Cstime,&pProcessStat->s32Priority,&pProcessStat->s32Nice,&pProcessStat->s32Threads,&pProcessStat->s32Iterealvalue, //6
			 * &pProcessStat->u64Starttime,
			 */
			&pProcessStat->u32Vsize,
			&pProcessStat->s32Rss);
	u32Size = sysconf(_SC_PAGESIZE);
	pProcessStat->s32Rss *= u32Size;
	PRINT("%d %c utime: %u stime:%u vsize: %u rss %d\n",
			pProcessStat->s32Pid, pProcessStat->c8State,
			pProcessStat->u32Utime, pProcessStat->u32Stime,
			pProcessStat->u32Vsize, pProcessStat->s32Rss);
	return 0;
#endif
#if 0
	s32Size = fread(c8Buf, 1024, 1, pFileProcessStat);
	fclose(pFileProcessStat);
	if (s32Size < 0)
	{
		printf("file read /proc/%d/stat error %s", s32Pid, strerror(errno));
		return MY_ERR(_Err_SYS + errno);
	}
	c8Buf[s32Size - 1] = 0;
	{
		char *p1, *p2;

		sscanf(c8Buf, "%d ", &pProcessStat->s32Pid);

		p1 = strchr(c8Buf, '(');
		p1++;
		p2 = strchr(p1, ')');
		u32Size = p2 - p1;
		memcpy(pProcessStat->c8Name, p1, u32Size);
		pProcessStat->c8Name[u32Size] = 0;
		sscanf(p2 + 2,
			"%c "
			"%d %d %d %d %d "
			"%u "
			"%u %u %u %u %u %u "
			"%d %d %d %d %d %d "
			"%llu "
			"%u "
			"%d",
			&pProcessStat->c8State, //1
			&pProcessStat->s32Ppid,&pProcessStat->s32Pgrp,&pProcessStat->s32Session,&pProcessStat->s32Tty_nr,&pProcessStat->s32Tpgid, //5
			&pProcessStat->u32Flags,//1
			&pProcessStat->u32Minflt,&pProcessStat->u32Cminflt,&pProcessStat->u32Majflt,&pProcessStat->u32Cmajflt,&pProcessStat->u32Utime,&pProcessStat->u32Stime,//6
			&pProcessStat->s32Cutime,&pProcessStat->s32Cstime,&pProcessStat->s32Priority,&pProcessStat->s32Nice,&pProcessStat->s32Threads,&pProcessStat->s32Iterealvalue, //6
			&pProcessStat->u64Starttime,//1
			&pProcessStat->u32Vsize,//1
			&pProcessStat->s32Rss); //1

	}
	u32Size = sysconf(_SC_PAGESIZE);
	pProcessStat->s32Rss *= u32Size;
	PRINT("%d (%s) %c utime: %u stime:%u vsize: %u rss %d\n",
			pProcessStat->s32Pid, pProcessStat->c8Name, pProcessStat->c8State,
			pProcessStat->u32Utime, pProcessStat->u32Stime,
			pProcessStat->u32Vsize, pProcessStat->s32Rss);
	return 0;
#endif
}


/*
 * 函数名      : GetUpTime
 * 功能        : 得到系统的uptime
 * 参数        : pUpTime[out] (StUpTime): 详见定义
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t GetUpTime(StUpTime *pUpTime)
{
	FILE *pFile;
	if (pUpTime == NULL)
	{
		PRINT("Argument error\n");
		return -1;
	}
	pFile = fopen("/proc/uptime", "r");
	if (pFile == NULL)
	{
		PRINT("file open /proc/uptime error %s\n", strerror(errno));
		return -1;
	}

	fscanf(pFile, "%lf %lf", &(pUpTime->d64UpTime), &(pUpTime->d64IdleTime));
	fclose(pFile);
	return 0;
}

#if 0
int32_t GetProcessUsage(pid_t s32Pid, uint8_t *pCPU, uint8_t *pMEM)
{
	StProcessStat stProcessStat;
	StUpTime stUpTime;
	int32_t s32Err = 0;
	if((s32Pid < 0) || (pCPU == NULL)/* || (pMEM == NULL)*/)
	{
		PRINT("Argument error");
		return -1;
	}

	s32Err = GetProcessStat(s32Pid, &stProcessStat);
	if (s32Err != 0)
	{
		return -1;
	}
	s32Err = GetUpTime(&stUpTime);
	if (s32Err != 0)
	{
		return -1;
	}
	{
		int32_t s32CPUHZ = sysconf(_SC_CLK_TCK);
		uint64_t u64ProcessTime = stProcessStat.u32Utime+stProcessStat.u32Stime;
		uint64_t u64UpTime = stUpTime.d64UpTime * s32CPUHZ - stProcessStat.u64Starttime;
		uint64_t u64CPU = u64UpTime ? (u64ProcessTime * 100 / u64UpTime) : 0;
		*pCPU = u64CPU;
	}

	return 0;
}
#endif

#if 0
int32_t ProcessListCreate(int32_t *pErr)
{
	return SLOInit("ProcessList", 64, sizeof(StProcessInfo), 0, pErr);
}
void ProcessListRelease()
void ProcessListTombDestroy(int32_t s32Handle)
{
	SLOTombDestroy(s32Handle);
}

int32_t ProcessListUpdateState(int32_t s32Handle)
{


	return 0;
}
#endif

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

/*
 * 函数名      : ProcessListInit
 * 功能        : 初始化进程维护表在该进程空间的资源，并将该进程信息插入到表中
 * 参数        : pErr[in] (int32_t * 类型): 不为NULL时, 用于保存错误码
 * 返回值      : int32_t 型数据, 错误返回0, 否则返回控制句柄
 * 作者        : 许龙杰
 * 注          : 此函数不再使用
 */
int32_t ProcessListInit(int32_t *pErr)
{
	int32_t s32SLOHandle = 0;
	int32_t s32Err = 0;
	StProcessList *pList;
	StProcessInfo stInfo = {0};

	pList = malloc(sizeof(StProcessList));
	if (pList == NULL)
	{
		s32Err = MY_ERR(_Err_Mem);
		goto err1;
	}

	s32SLOHandle = SLOInit(PEOCESS_LIST_NAME, PEOCESS_LIST_CNT, sizeof(StProcessInfo), 0, &s32Err);
	if (s32SLOHandle == 0)
	{
		goto err2;
	}

	stInfo.u32Pid = getpid();
	GetProcessNameFromPID(stInfo.c8Name, sizeof(stInfo.c8Name), stInfo.u32Pid);
	s32Err = SLOInsertAnEntity(s32SLOHandle, &stInfo, Compare);
	if (s32Err < 0)
	{
		goto err3;
	}
	pList->s32SLOHandle = s32SLOHandle;
	pList->s32EntityIndex = s32Err;

	return (int32_t)pList;

err3:
	SLODestroy(s32SLOHandle);
err2:
	free(pList);
err1:
	if (pErr != NULL)
	{
		*pErr = s32Err;
	}
	return 0;
}

/*
 * 函数名      : ProcessListUpdate
 * 功能        : 更新进程表，指明当前进程在仍在有效运行
 * 参数        : s32Handle[in] (int32_t): ProcessListInit的返回值
 * 返回值      : int32_t 型数据, 正数返回0, 否则返回错误码
 * 作者        : 许龙杰
 * 注          : 此函数不再使用
 */
int32_t ProcessListUpdate(int32_t s32Handle)
{
	StProcessList *pList = (StProcessList *)s32Handle;
	if (pList == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	return SLOUpdateAnEntityUsingIndexDirectly(pList->s32SLOHandle, pList->s32EntityIndex);
}


/*
 * 函数名      : ProcessListDestroy
 * 功能        : 从进程表中删除该进程信息，并销毁句柄在该进程空间的资源
 * 参数        : s32Handle[in] (int32_t): ProcessListInit的返回值
 * 返回值      : 无
 * 作者        : 许龙杰
 * 注          : 此函数不再使用
 */
void ProcessListDestroy(int32_t s32Handle)
{
	StProcessList *pList = (StProcessList *)s32Handle;
	if (pList == NULL)
	{
		return;
	}
#ifdef _DEBUG
	SLOTraversal(pList->s32SLOHandle, NULL, NULL);
#endif
	SLODeleteAnEntityUsingIndexDirectly(pList->s32SLOHandle, pList->s32EntityIndex);
#ifdef _DEBUG
	SLOTraversal(pList->s32SLOHandle, NULL, NULL);
#endif
	SLODestroy(pList->s32SLOHandle);
	free(pList);

}

/* called by ProcessStatisticNew, ProcessStatisticDelete an ProcessStatisticUpdate */
static int32_t ProcessStatisticOperate(int32_t s32Code)
{
	int32_t s32PID = getpid();
	int32_t s32Err = 0;
	char c8ProcessName[_POSIX_PATH_MAX];
	int32_t s32Client = -1;

	s32Err = GetProcessNameFromPID(c8ProcessName, MAX_LINE, s32PID);
	if (s32Err != 0)
	{
		return s32Err;
	}

	s32Client = ClientConnect(PROCESS_STATISTIC_SOCKET);
	if (s32Client < 0)
	{
		return s32Client;
	}

	s32Err = MCSSyncSend(s32Client, 1000, s32Code,
			strlen(c8ProcessName) + 1, c8ProcessName);
	close(s32Client);

	return s32Err;

}

/*
 * 函数名      : ProcessStatisticNew
 * 功能        : 告知统计线程要增加一个新的进程
 * 参数        : 无
 * 返回值      : int32_t 型数据, 正数返回0, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t ProcessStatisticNew(void)
{
	return ProcessStatisticOperate(_Process_Statistic_Add);
}


/*
 * 函数名      : ProcessStatisticDelete
 * 功能        : 告知统计线程要删除一个正在统计的进程
 * 参数        : 无
 * 返回值      : int32_t 型数据, 正数返回0, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t ProcessStatisticDelete(void)
{
	return ProcessStatisticOperate(_Process_Statistic_Delete);
}

/*
 * 函数名      : ProcessStatisticUpdate
 * 功能        : 告知统计线程要更新一个正在统计的进程
 * 参数        : 无
 * 返回值      : int32_t 型数据, 正数返回0, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t ProcessStatisticUpdate(void)
{
	return ProcessStatisticOperate(_Process_Statistic_Update);
}



