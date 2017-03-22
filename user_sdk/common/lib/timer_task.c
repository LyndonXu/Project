/****************************************************************************
 * Copyright(c), 2001-2060, ******************************* 版权所有
 ****************************************************************************
 * 文件名称             : timer_task.c
 * 版本                 : 0.0.1
 * 作者                 : 许龙杰
 * 创建日期             : 2015年1月23日
 * 描述                 : 
 ****************************************************************************/

#include <common.h>
#include "common_define.h"

/* 将任务连接到HASH表中 */
static int32_t TimerTaskLinkInner(StTimeTaskInfo *pHeadArrar[TIME_TASK_WHEEL_SIZE], uint64_t u64CurTime,
		StTimeTaskInfo *pTask)
{
	int32_t s32Spoke;

	pTask->u64TimeMatch = u64CurTime + pTask->stInfo.u32Cycle;
	s32Spoke = pTask->u64TimeMatch % TIME_TASK_WHEEL_SIZE;	/* 取模HASH */
	PRINT("s32Spoke: %d, matching time is: %lld\n", s32Spoke, pTask->u64TimeMatch);

	if (pHeadArrar[s32Spoke] == NULL)
	{
		pTask->pNext = pTask->pPrev = NULL;
	}
	else
	{
		pTask->pPrev = NULL;
		pHeadArrar[s32Spoke]->pPrev = pTask;
		pTask->pNext = pHeadArrar[s32Spoke];
	}
	pHeadArrar[s32Spoke] = pTask;
	return 0;

}

static int32_t TimerTaskLink(StTimeTaskInfo *pHeadArrar[TIME_TASK_WHEEL_SIZE], uint64_t u64CurTime,
		StTimeTaskBaseInfo *pBaseTask)
{
	StTimeTaskInfo *pTask;
	if ((pHeadArrar == NULL) || (pBaseTask == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}
	pTask = malloc(sizeof(StTimeTaskInfo));
	if (pTask == NULL)
	{
		return MY_ERR(_Err_Mem);
	}
	pTask->stInfo = *pBaseTask;
	return TimerTaskLinkInner(pHeadArrar, u64CurTime, pTask);
}
/* 将任务从HASH表中删除 */
static int32_t TimerTaskUnlink(StTimeTaskInfo *pHeadArrar[TIME_TASK_WHEEL_SIZE], StTimeTaskInfo *pTask)
{
	int32_t s32Spoke;

	if ((pHeadArrar == NULL) || (pTask == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}

	s32Spoke = pTask->u64TimeMatch % TIME_TASK_WHEEL_SIZE;
	PRINT("s32Spoke: %d\n", s32Spoke);

	if (pHeadArrar[s32Spoke] == NULL)
	{
		return MY_ERR(_Err_Common);
	}

	if (pHeadArrar[s32Spoke] == pTask)
	{
		pHeadArrar[s32Spoke] = pTask->pNext;
		if (pHeadArrar[s32Spoke] != NULL)
		{
			pHeadArrar[s32Spoke]->pPrev = NULL;
		}
	}
	else
	{
		if (pTask->pPrev != NULL)
		{
			pTask->pPrev->pNext = pTask->pNext;
		}
		if (pTask->pNext != NULL)
		{
			pTask->pNext->pPrev = pTask->pPrev;
		}

	}
	return 0;
}

/* 定时任务 HASH 表轮询 */
static int32_t TimerTaskWheel(StTimeTaskInfo *pHeadArrar[TIME_TASK_WHEEL_SIZE], uint64_t u64CurTime)
{
	int32_t s32Spoke;
	StTimeTaskInfo *pTmp;
	if (pHeadArrar == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}

	s32Spoke = u64CurTime % TIME_TASK_WHEEL_SIZE;
	/*PRINT("s32Spoke: %d\n", s32Spoke);*/
	pTmp = pHeadArrar[s32Spoke];
	while (pTmp != NULL)
	{
		PRINT("s32Spoke: %d\n", s32Spoke);
		StTimeTaskInfo *pTmpNext = pTmp->pNext;

		if (u64CurTime == pTmp->u64TimeMatch)
		{
			int32_t s32Err = 0;
			if (pTmp->stInfo.pFunTask != NULL)
			{
				s32Err = pTmp->stInfo.pFunTask(pTmp->stInfo.pArg);
			}
			TimerTaskUnlink(pHeadArrar, pTmp);
			if ((s32Err == 0) && (pTmp->stInfo.emType == _Time_Task_Periodic))
			{
				TimerTaskLinkInner(pHeadArrar, u64CurTime, pTmp);
			}
			else
			{
				free(pTmp);
			}
		}
		pTmp = pTmpNext;
	}
	return 0;
}
/* 释放所有占用的资源 */
static int32_t TimerTaskReleaseAll(StTimeTaskInfo *pHeadArrar[TIME_TASK_WHEEL_SIZE])
{
	int32_t i;
	if (pHeadArrar == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	for (i = 0; i < TIME_TASK_WHEEL_SIZE; i++)
	{
		StTimeTaskInfo *pTmp;
		pTmp = pHeadArrar[i];
		while (pTmp != NULL)
		{
			StTimeTaskInfo *pTmpNext = pTmp->pNext;

			free(pTmp);
			pTmp = pTmpNext;
		}
	}
	return 0;
}

/* 定时任务线程 */
static void *ThreadTimeTask(void *pArg)
{
	StTimeTask *pTask = pArg;
	int s32Server = -1;
	uint64_t u64Time = 0;
	uint64_t u64Cnt = 0;

	StTimeTaskInfo *pHeadArrar[TIME_TASK_WHEEL_SIZE] = {NULL};

	PRINT("thread(%s) in, %s\n", __FUNCTION__, pTask->c8Str);
	s32Server = ServerListen(pTask->c8Str);
	if (s32Server < 0)
	{
		PRINT("ServerListen error: 0x%08x\n", s32Server);
		return NULL;
	}
	u64Time = TimeGetSetupTime();
	while(!pTask->boIsExit)
	{
		fd_set stSet;
		struct timeval stTimeout;
		uint64_t u64TimeTmp = TimeGetSetupTime() - u64Time;
		uint64_t u64TimeTmp1 = 0;
		int32_t s32Client = -1;

		u64TimeTmp1 = (u64Cnt + 1) * pTask->u32Cycle;
		if (u64TimeTmp >= u64TimeTmp1)				/* 当前时间大于应该的时间 */
		{
			TimerTaskWheel(pHeadArrar, u64Cnt);
			u64Cnt++;
			continue;
		}
		u64TimeTmp = u64TimeTmp1 - u64TimeTmp;		/* 减去程序消耗的时间, 以保证时间的准确与误差的不累计 */
		stTimeout.tv_sec = u64TimeTmp / 1000;
		stTimeout.tv_usec = (u64TimeTmp % 1000) * 1000;
		FD_ZERO(&stSet);
		FD_SET(s32Server, &stSet);

		if (select(s32Server + 1, &stSet, NULL, NULL, &stTimeout) <= 0)	/* 超时 */
		{
			TimerTaskWheel(pHeadArrar, u64Cnt);
			u64Cnt++;
			continue;
		}
		if (!FD_ISSET(s32Server, &stSet))				/* 不是自己的描述符 */
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
				TimerTaskLink(pHeadArrar, u64Cnt, (StTimeTaskBaseInfo *)pMCSStream);
				MCSSyncFree(pMCSStream);
			}
			else
			{
				PRINT("MCSSyncReceive error 0x%08x\n", s32Err);
			}
			close(s32Client);
		}
	}
	TimerTaskReleaseAll(pHeadArrar);
	ServerRemove(s32Server, pTask->c8Str);
	PRINT("thread(%s) out\n", __FUNCTION__);
	return NULL;
}

/*
 * 函数名      : TimeTaskInit
 * 功能        : 初始化定时任务
 * 参数        : pName [in]: (const char * 类型) 定时任务的名字
 *             : u32Cycle [in]: (uint32_t 类型) 周期的颗粒度(ms), 不建议太小, 也不建议太大尽量不要超过秒级
 *             : pErr [in/out]: (int32_t * 类型) 不为NULL时用于保存错误码
 * 返回值      : (int32_t类型) 非0表示成功, 否则错误
 * 作者        : 许龙杰
 */
int32_t TimeTaskInit(const char *pName, uint32_t u32Cycle, int32_t *pErr)
{
	StTimeTask *pTask = NULL;
	int32_t s32Err;

	if ((pName == NULL) || (u32Cycle == 0))
	{
		s32Err = MY_ERR(_Err_InvalidParam);
		goto end;
	}
	pTask = malloc(sizeof(StTimeTask));
	if (pTask == NULL)
	{
		s32Err = MY_ERR(_Err_Mem);
		goto end;
	}
	snprintf(pTask->c8Str, 63, "%s%s", WORK_DIR, pName);
	pTask->c8Str[63] = 0;
	pTask->u32Cycle = u32Cycle;
	pTask->boIsExit = false;
	s32Err = MakeThread(ThreadTimeTask, pTask, false, &(pTask->u32ThreadId), false);
	if (s32Err != 0)
	{
		free(pTask);
		pTask = NULL;
	}

end:
	if (pErr != NULL)
	{
		*pErr = s32Err;
	}
	return (int32_t)pTask;
}


/*
 * 函数名      : TimeTaskAddATask
 * 功能        : 增加一个定时任务
 * 参数        : s32Handle [in]: (int32_t 类型) TimeTaskInit的返回值
 *             : pFunTask [in]: (pFUN_TimeTask 类型) 任务的函数指针
 *             : pArg [in]: (void * 类型) 传递给回调函数的参数
 *             : u32Cycle [in]: (uint32_t 类型) 轮询的周期, 实际轮询的时间是该时间 * 颗粒度
 *             : emType [in]: (EmTimeTask 类型) 详见定义
 * 返回值      : (int32_t类型) 0表示成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t TimeTaskAddATask(int32_t s32Handle, pFUN_TimeTask pFunTask,
		void *pArg, uint32_t u32Cycle, EmTimeTask emType)
{
	StTimeTask *pTask = (StTimeTask *)s32Handle;
	if ((pTask == NULL) || (emType >= _Time_Task_Reserved))
	{
		return MY_ERR(_Err_InvalidParam);
	}
	else
	{
		StTimeTaskBaseInfo stInfo;
		int32_t s32Client;
		int32_t s32Err;

		s32Client = ClientConnect(pTask->c8Str);
		if (s32Client < 0)
		{
			return s32Client;
		}

		stInfo.emType = emType;
		stInfo.pArg = pArg;
		stInfo.pFunTask = pFunTask;
		stInfo.u32Cycle = u32Cycle;

		s32Err = MCSSyncSendData(s32Client, 1000, sizeof(StTimeTaskBaseInfo), &stInfo);
		close(s32Client);

		return s32Err;
	}
}

/*
 * 函数名      : TimeTaskDestory
 * 功能        : 销毁定时任务
 * 参数        : s32Handle [in]: (int32_t 类型) TimeTaskInit的返回值
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void TimeTaskDestory(int32_t s32Handle)
{
	StTimeTask *pTask = (StTimeTask *)s32Handle;
	if (pTask != NULL)
	{
		pTask->boIsExit = true;
		pthread_join(pTask->u32ThreadId, NULL);
		free(pTask);
	}
}


