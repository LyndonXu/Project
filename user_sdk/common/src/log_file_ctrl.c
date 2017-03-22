/****************************************************************************
 * Copyright(c), 2001-2060, ******************************* 版权所有
 ****************************************************************************
 * 文件名称             : log_file_ctrl.c
 * 版本                 : 0.0.1
 * 作者                 : 许龙杰
 * 创建日期             : 2014年5月7日
 * 描述                 : 
 ****************************************************************************/
#include "common_define.h"

#define TIME_FLAG_CNT	(20 * 2 + 1 + 1)/* uint64_t-uint64_t\n */
#define FILE_LOG_CNT		8


/*
 * 函数名      : FileCtrlDestroy
 * 功能        : 销毁日志控制和相关资源与FileCtrlInit成对使用
 * 参数        : pFileCtrl [in] (StFileCtrl) FileCtrlInit返回的句柄
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void FileCtrlDestroy(StFileCtrl *pFileCtrl)
{
	StFileCtrlElement *pFileCtrlElement;
	if (pFileCtrl == NULL)
	{
		return;
	}

	if (pFileCtrl->u32BufUsingCnt > TIME_FLAG_CNT)
	{
		if (pFileCtrl->pLatestFile != NULL)
		{
			if (pFileCtrl->pLatestFile->pFile != NULL)
			{
				/* write the rest of log into the file */
				char c8TimeFlag[TIME_FLAG_CNT];
				c8TimeFlag[0] = 0;
				sprintf(c8TimeFlag, "% 20lld-% 20lld",
						pFileCtrl->u64BufStartTime, pFileCtrl->u64BufEndTime);
				memcpy(pFileCtrl->c8Buf, c8TimeFlag, strlen(c8TimeFlag));

				/* fill the buffer with space and write the buffer to the file */
				memset(pFileCtrl->c8Buf + pFileCtrl->u32BufUsingCnt, ' ',
						PAGE_SIZE - pFileCtrl->u32BufUsingCnt - 1);
				pFileCtrl->c8Buf[PAGE_SIZE - 1] = '\n';
				fseek(pFileCtrl->pLatestFile->pFile, 0, SEEK_END);
				fwrite(pFileCtrl->c8Buf, PAGE_SIZE, 1, pFileCtrl->pLatestFile->pFile);
				fflush(pFileCtrl->pLatestFile->pFile);
			}
		}
	}


	pFileCtrlElement = pFileCtrl->pFileCtrlElement;
	if (pFileCtrlElement != NULL)
	{
		uint32_t i = 0;
		for (i = 0; i < pFileCtrl->u32MaxCnt; i++)
		{
			if (pFileCtrlElement[i].pFile != NULL)
			{
				fclose(pFileCtrlElement[i].pFile);
			}
		}
		free(pFileCtrlElement);
	}
	free(pFileCtrl);
}

/*
 * 函数名      : FileCtrlInit
 * 功能        : 初始化日志控制和相关资源与FileCtrlDestroy成对使用
 * 参数        : pName [in] (const char *) 日志的名字
 * 			   : u32MaxCnt [in] (uint32_t) 最大缓冲天数
 * 			   : pErr [out] (uint32_t *) 不为NULL时保存错误码
 * 返回值      : 成功赶回非NULL指针
 * 作者        : 许龙杰
 */
StFileCtrl *FileCtrlInit(const char *pName, uint32_t u32MaxCnt, int32_t *pErr)
{
	StFileCtrl *pFileCtrl = NULL;
	StFileCtrlElement *pFileCtrlElement = NULL;
	int32_t s32Err = 0;
	char c8Name[PATH_MAX];


	pFileCtrl = (StFileCtrl *)calloc(1, sizeof(StFileCtrl));
	if (pFileCtrl == NULL)
	{
		s32Err = MY_ERR(_Err_Mem);
		goto err;
	}

	pFileCtrl->u32MaxCnt = u32MaxCnt;

	c8Name[0] = 0;
	if (pName == NULL)
	{
		pName = LOG_FILE;
	}
	sprintf(c8Name + strlen(c8Name), "%s", pName);

	pFileCtrl->pFileCtrlElement = pFileCtrlElement =
		(StFileCtrlElement *)calloc(u32MaxCnt, sizeof(StFileCtrl));

	if (pFileCtrlElement == NULL)
	{
		s32Err = MY_ERR(_Err_Mem);
		goto err;
	}
	{
		uint32_t i;
		uint32_t u32PageSize = PAGE_SIZE;
		bool boFind = false;

		for (i = 0; i < u32MaxCnt; i++)
		{
			char c8Buf[PATH_MAX];
			FILE *pFile;
			uint32_t u32Tmp;

			/* link the element */
			pFileCtrlElement[i].pNext = pFileCtrlElement + ((i + 1) % u32MaxCnt);
			pFileCtrlElement[i].pPrev = pFileCtrlElement + ((i - 1) % u32MaxCnt);

			/* open file */
			c8Buf[0] = 0;
			sprintf(c8Buf, "%s%d", c8Name, i);
#ifdef _DEBUG
			pFileCtrlElement[i].c8Name[0] = 0;
			strcat(pFileCtrlElement[i].c8Name, c8Buf);
#endif
			pFile = fopen(c8Buf, "ab+");
			if (pFile == NULL)
			{
				s32Err = MY_ERR(_Err_SYS + errno);
				goto err;
			}
			fclose(pFile);
			pFileCtrlElement[i].pFile = pFile = fopen(c8Buf, "rb+");
			if (pFile == NULL)
			{
				s32Err = MY_ERR(_Err_SYS + errno);
				goto err;
			}
			fseek(pFile, 0, SEEK_END);
			u32Tmp = pFileCtrlElement[i].u32FileSize = ftell(pFile);
			fseek(pFile, 0, SEEK_SET);
			if (u32Tmp == 0)
			{
				continue;
			}

			u32Tmp /= u32PageSize;
			/* get the valid text position */
			{
				int32_t s32Left = -1, s32Right = u32Tmp;
				int32_t s32Pos = 0;
				boFind = false;
				while (s32Left < s32Right)
				{
					s32Pos = (s32Left + s32Right) / 2;
					fseek(pFile, s32Pos * u32PageSize, SEEK_SET);
					fread(c8Buf, TIME_FLAG_CNT, 1, pFile);
					if (c8Buf[0] != 0)
					{
						if ((s32Left + 1) == s32Pos)
						{
							boFind = true;
							break;
						}
						s32Right = s32Pos;
					}
					else
					{
						if ((s32Pos + 1) == s32Right)
						{
							boFind = true;
							s32Pos = s32Right;
							break;
						}
						s32Left = s32Pos;
					}
				} /* while (s32Left < s32Right) */
				if (s32Pos == (int32_t)u32Tmp)
				{
					/* there is no valid text in the file */
					pFileCtrlElement[i].u32FileSize = 0;

					ftruncate(fileno(pFileCtrlElement[i].pFile), 0);
					rewind(pFileCtrlElement[i].pFile);

					continue;
				}
				pFileCtrlElement[i].u32ValidSize = (u32Tmp - (uint32_t)s32Pos) * u32PageSize;
				pFileCtrl->u32GlobalValidSize += pFileCtrlElement[i].u32ValidSize;
				fseek(pFile, s32Pos * u32PageSize, SEEK_SET);
				fread(c8Buf, TIME_FLAG_CNT, 1, pFile);
				sscanf(c8Buf, "%lld-%*d", &(pFileCtrlElement[i].u64StartTime));
				fseek(pFile, (u32Tmp - 1) * u32PageSize, SEEK_SET);
				fread(c8Buf, TIME_FLAG_CNT, 1, pFile);
				sscanf(c8Buf, "%*d-%lld", &(pFileCtrlElement[i].u64EndTime));
			}
		}/* for (i = 0; i < u32MaxCnt; i++) */


		/* find the oldest file and latest file */
		pFileCtrl->pLatestFile = pFileCtrl->pOldestFile = pFileCtrlElement;
		boFind = false;
		for (i = 0; i < u32MaxCnt; i++)
		{
			pFileCtrlElement = pFileCtrl->pLatestFile;
			if (pFileCtrlElement->u64EndTime == 0)
			{
				if (pFileCtrl->u64GlobalEndTime == 0)
				{
					pFileCtrl->pLatestFile = pFileCtrlElement->pNext;
				}
			}
			else
			{
				if (pFileCtrl->u64GlobalEndTime < pFileCtrlElement->u64EndTime)
				{
					boFind = true;
					pFileCtrl->u64GlobalEndTime = pFileCtrlElement->u64EndTime;
					pFileCtrl->pLatestFile = pFileCtrlElement->pNext;
				}
			}

			pFileCtrlElement = pFileCtrl->pOldestFile;
			if (pFileCtrlElement->u64StartTime == 0)
			{
				if (pFileCtrl->u64GlobalStartTime == 0)
				{
					pFileCtrl->pOldestFile = pFileCtrlElement->pPrev;
				}
			}
			else
			{
				if ((pFileCtrl->u64GlobalStartTime == 0) ||
					(pFileCtrl->u64GlobalStartTime > pFileCtrlElement->u64StartTime))
				{
					boFind = true;
					pFileCtrl->u64GlobalStartTime = pFileCtrlElement->u64StartTime;
					pFileCtrl->pOldestFile = pFileCtrlElement->pPrev;
				}
			}
		}/* for (i = 0; i < u32MaxCnt; i++) */
		if (boFind)
		{
			/* correct one file */
			pFileCtrl->pOldestFile = pFileCtrl->pOldestFile->pNext;
			pFileCtrl->pLatestFile = pFileCtrl->pLatestFile->pPrev;

			pFileCtrl->u32CurUsing = (uint32_t)pFileCtrl->pLatestFile - (uint32_t)pFileCtrl->pOldestFile;
			if ((int32_t)pFileCtrl->u32CurUsing < 0)
			{
				pFileCtrl->u32CurUsing =
					sizeof(StFileCtrlElement) * pFileCtrl->u32MaxCnt +
					pFileCtrl->u32CurUsing;
			}
			pFileCtrl->u32CurUsing /= sizeof(StFileCtrlElement);
			pFileCtrl->u32CurUsing += 1;
		}
		else
		{
			/*  */
			pFileCtrl->u32CurUsing = 0;
			pFileCtrl->pOldestFile = pFileCtrl->pFileCtrlElement;
			pFileCtrl->pLatestFile = pFileCtrl->pFileCtrlElement->pPrev;
		}
	}
	pFileCtrl->u32BufUsingCnt = TIME_FLAG_CNT;
	memset(pFileCtrl->c8Buf, ' ', TIME_FLAG_CNT - 1);
	pFileCtrl->c8Buf[TIME_FLAG_CNT - 1] = '\n';

#ifdef _DEBUG
	PRINT("start time %lld\n", pFileCtrl->u64GlobalStartTime);
	PRINT("end time %lld\n", pFileCtrl->u64GlobalEndTime);
	PRINT("oldest file %s\n", pFileCtrl->pOldestFile->c8Name);
	PRINT("latest file %s\n", pFileCtrl->pLatestFile->c8Name);
#endif
	if (pErr != NULL)
	{
		*pErr = 0;
	}
	return pFileCtrl;
err:
	FileCtrlDestroy(pFileCtrl);
	if (pErr != NULL)
	{
		*pErr = s32Err;
	}
	return NULL;
}

/*
 * 函数名      : TimeDiff
 * 功能        : 得到两个的时间差
 * 参数        : s32Left [in] (time_t) 时间
 * 			   : s32Right [in] (time_t) 时间
 * 返回值      : s32Right大于s32Left一天的话返回1，否则返回0
 * 作者        : 许龙杰
 */
int32_t TimeDiff(time_t s32Left, time_t s32Right)
{
	struct tm stLeft = *localtime(&s32Left);
	struct tm stRight = *localtime(&s32Right);

	if (stRight.tm_year > stLeft.tm_year)
	{
		return 1;
	}
	if (stRight.tm_mon > stLeft.tm_mon)
	{
		return 1;
	}
	if (stRight.tm_mday > stLeft.tm_mday)
	{
		return 1;
	}
	return 0;
}


/*
 * 函数名      : FileCtrlWriteLog
 * 功能        : 写入一条日志
 * 参数        : pCtrl [in] (StFileCtrl *) FileCtrlInit返回的句柄
 * 			   : pLog [in] (const char *): 日志内容, 字符串
 * 			   : s32Size [out] (int32_t): 正数表示内容的长度，负值，程序将会自动计算长度
 * 返回值      : int32_t 错误返回0，否则返回控制句柄
 * 作者        : 许龙杰
 */
int32_t FileCtrlWriteLog(StFileCtrl *pCtrl, const char *pLog, int32_t s32Size)
{

	if ((pCtrl == NULL) || (pLog == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}

	if (s32Size < 0)
	{
		s32Size = strlen(pLog);
	}
	{
		struct tm stLogTime;
		uint64_t u64Time = 0;
		uint32_t u32Time = 0;

		sscanf(pLog, LOG_DATE_FORMAT,
			&(stLogTime.tm_year), &(stLogTime.tm_mon), &(stLogTime.tm_mday),
			&(stLogTime.tm_hour), &(stLogTime.tm_min), &(stLogTime.tm_sec),
			&u32Time);

		stLogTime.tm_year -= 1900;
		stLogTime.tm_mon -= 1;
		{
			time_t s32Time = mktime(&stLogTime);
			if (s32Time == -1)
			{
				return MY_ERR(_Err_LOG_Time_FMT);
			}
			u64Time = s32Time;
		}
		u64Time *= 1000;
		u64Time += u32Time;	/* ms */


		if (pCtrl->u64BufStartTime == 0)
		{
			pCtrl->u64BufStartTime = u64Time;
		}

		if ((s32Size + pCtrl->u32BufUsingCnt) < PAGE_SIZE)
		{
			memcpy(pCtrl->c8Buf + pCtrl->u32BufUsingCnt, pLog, s32Size);
			pCtrl->u32BufUsingCnt += s32Size;
			if (pCtrl->c8Buf[pCtrl->u32BufUsingCnt - 1] != '\n')
			{
				pCtrl->c8Buf[pCtrl->u32BufUsingCnt - 1] = '\n';
				pCtrl->u32BufUsingCnt += 1;
			}
			pCtrl->u64BufEndTime = u64Time;
		}
		else
		{
			char c8TimeFlag[TIME_FLAG_CNT];
			c8TimeFlag[0] = 0;
			sprintf(c8TimeFlag, "% 20lld-% 20lld", pCtrl->u64BufStartTime, pCtrl->u64BufEndTime);
			memcpy(pCtrl->c8Buf, c8TimeFlag, strlen(c8TimeFlag));
			if (pCtrl->u64GlobalStartTime == 0)
			{
				pCtrl->u64GlobalStartTime = pCtrl->u64BufStartTime;
			}


			if (TimeDiff(pCtrl->u64GlobalEndTime / 1000,
				pCtrl->u64BufEndTime / 1000) == 1)/* new day */
			{
				if (pCtrl->u32CurUsing == pCtrl->u32MaxCnt)
				{
					pCtrl->u32GlobalValidSize -= pCtrl->pOldestFile->u32ValidSize;
					pCtrl->pOldestFile = pCtrl->pOldestFile->pNext;
					pCtrl->u64GlobalStartTime = pCtrl->pOldestFile->u64StartTime;
				}
				else
				{
					pCtrl->u32CurUsing++;
				}
				pCtrl->pLatestFile = pCtrl->pLatestFile->pNext;
				pCtrl->pLatestFile->u32FileSize =
					pCtrl->pLatestFile->u32ValidSize = 0;
				pCtrl->pLatestFile->u64StartTime = pCtrl->u64BufStartTime;

				/* truncate the file */
				ftruncate(fileno(pCtrl->pLatestFile->pFile), 0);
				rewind(pCtrl->pLatestFile->pFile);
			}
			/* fill the buffer with space and write the buffer to the file */
			memset(pCtrl->c8Buf + pCtrl->u32BufUsingCnt, ' ', PAGE_SIZE - pCtrl->u32BufUsingCnt - 1);
			pCtrl->c8Buf[PAGE_SIZE - 1] = '\n';
			fseek(pCtrl->pLatestFile->pFile, 0, SEEK_END);
			fwrite(pCtrl->c8Buf, PAGE_SIZE, 1, pCtrl->pLatestFile->pFile);
			fflush(pCtrl->pLatestFile->pFile);

			/* flush the flags */
			pCtrl->pLatestFile->u32FileSize += PAGE_SIZE;
			pCtrl->pLatestFile->u32ValidSize += PAGE_SIZE;
			pCtrl->pLatestFile->u64EndTime = pCtrl->u64BufEndTime;

			pCtrl->u32GlobalValidSize += PAGE_SIZE;
			pCtrl->u64GlobalEndTime = pCtrl->u64BufEndTime;

			/* initial the buf */
			pCtrl->u32BufUsingCnt = TIME_FLAG_CNT;
			memset(pCtrl->c8Buf, ' ', TIME_FLAG_CNT - 1);
			pCtrl->c8Buf[TIME_FLAG_CNT - 1] = '\n';
			pCtrl->u64BufStartTime = u64Time;

			/* write the buf */
			memcpy(pCtrl->c8Buf + pCtrl->u32BufUsingCnt, pLog, s32Size);
			pCtrl->u32BufUsingCnt += s32Size;
			if (pCtrl->c8Buf[pCtrl->u32BufUsingCnt - 1] != '\n')
			{
				pCtrl->c8Buf[pCtrl->u32BufUsingCnt - 1] = '\n';
				pCtrl->u32BufUsingCnt += 1;
			}
			pCtrl->u64BufEndTime = u64Time;
		}
	}
	return 0;
}


/*
 * 函数名      : FileCtrlGetAOldestBlockLog
 * 功能        : 读取一块最旧日志，成功后并使用过之后应该使用free函数释放*p2Log
 * 参数        : pCtrl [in] (StFileCtrl *) FileCtrlInit返回的句柄
 * 			   : p2Log [in] (const char **): 一块儿日志内容, 字符串
 * 			   : pSize [out] (int32_t *): 保存日志的长度
 * 返回值      : int32_t 成功返回0，否则返回错误码
 * 作者        : 许龙杰
 */
int32_t FileCtrlGetAOldestBlockLog(StFileCtrl *pCtrl, char **p2Log, uint32_t *pSize)
{
	int s32Err = 0;

	if ((pCtrl == NULL) || (p2Log == NULL) || (pSize == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}
	*p2Log = NULL;
	*pSize = 0;
	if (pCtrl->u32CurUsing == 0)
	{
		s32Err = MY_ERR(_Err_LOG_NoText);
		goto err;
	}
	{
		StFileCtrlElement *pElement = pCtrl->pOldestFile;
		if (pElement->u32ValidSize < PAGE_SIZE)
		{
			/* It should not be happened */
#ifdef _DEBUG
			PRINT("There is no valid data in the file %s\n", pElement->c8Name);
#else
			PRINT("There is no valid data in the file\n");
#endif
			pCtrl->pOldestFile = pElement->pNext;
			pCtrl->u32CurUsing--;

			pCtrl->u32GlobalValidSize -= pElement->u32ValidSize;
			pCtrl->u64GlobalStartTime = pCtrl->pOldestFile->u64StartTime;

			pElement->u64StartTime = pElement->u64EndTime = 0;
			pElement->u32FileSize = pElement->u32ValidSize = 0;

			/* truncate the file */
			ftruncate(fileno(pElement->pFile), 0);
			rewind(pElement->pFile);

			goto err;
		}
		else
		{
			*p2Log = malloc(PAGE_SIZE);
			if (*p2Log == NULL)
			{
				PRINT("Memory error\n");
				return -1;
			}
			*pSize = PAGE_SIZE;
			fseek(pElement->pFile, pElement->u32FileSize - pElement->u32ValidSize, SEEK_SET);
			fread(*p2Log, PAGE_SIZE, 1, pElement->pFile);
		}
	}
	return 0;
err:
	if ((*p2Log) != NULL)
	{
		free(*p2Log);
	}
	*pSize = 0;
	return s32Err;
}

/*
 * 函数名      : FileCtrlGetAOldestBlockLog
 * 功能        : 删除一块最旧日志
 * 参数        : pCtrl [in] (StFileCtrl *) FileCtrlInit返回的句柄
 * 返回值      : int32_t 成功返回0，否则返回错误码
 * 作者        : 许龙杰
 */
int32_t FileCtrlDelAOldestBlockLog(StFileCtrl *pCtrl)
{
	if (pCtrl == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}

	if (pCtrl->u32CurUsing == 0)
	{
		PRINT("There is no date in the ctrl\n");
		return MY_ERR(_Err_LOG_NoText);
	}
	{
		StFileCtrlElement *pElement = pCtrl->pOldestFile;
		if (pElement->u32ValidSize <= PAGE_SIZE)
		{
			pCtrl->pOldestFile = pElement->pNext;
			pCtrl->u32CurUsing--;

			pCtrl->u32GlobalValidSize -= pElement->u32ValidSize;
			pCtrl->u64GlobalStartTime = pCtrl->pOldestFile->u64StartTime;

			pElement->u64StartTime = pElement->u64EndTime = 0;
			pElement->u32FileSize = pElement->u32ValidSize = 0;

			/* truncate the file */
			ftruncate(fileno(pElement->pFile), 0);
			rewind(pElement->pFile);
		}
		else
		{
			char c8Buf[TIME_FLAG_CNT] = {0};
			uint32_t u32offset = pElement->u32FileSize - pElement->u32ValidSize;
			fseek(pElement->pFile, u32offset, SEEK_SET);
			fwrite(c8Buf, TIME_FLAG_CNT, 1, pElement->pFile);
			fflush(pElement->pFile);
			pElement->u32ValidSize -= PAGE_SIZE;
			u32offset += PAGE_SIZE;
			fseek(pElement->pFile, u32offset, SEEK_SET);
			fread(c8Buf, TIME_FLAG_CNT, 1, pElement->pFile);
			sscanf(c8Buf, "%lld-%*d", &(pElement->u64StartTime));


			pCtrl->u32GlobalValidSize -= PAGE_SIZE;
			pCtrl->u64GlobalStartTime = pElement->u64StartTime;
		}
	}
	return 0;
}



/*
 * 函数名      : LogFileCtrlInit
 * 功能        : 初始化日志文件控制，与函数LogFileCtrlDestroy成对使用
 * 参数        : pName [in] (const char *): 日志的名字
 * 			   : pErr [out] (int32_t *): 不为NULL时用于保护错误码
 * 返回值      : int32_t 错误返回0，否则返回控制句柄
 * 作者        : 许龙杰
 */
int32_t LogFileCtrlInit(const char *pName, int32_t *pErr)
{
	int32_t s32Err = 0;
	StLogFileCtrl *pCtrl;

	pCtrl = calloc(1, sizeof(StLogFileCtrl));

	if (pCtrl == NULL)
	{
		s32Err = MY_ERR(_Err_Mem);
		goto err;
	}

	{
		pthread_mutexattr_t stAttr;

		s32Err = pthread_mutexattr_init(&stAttr);
		if (s32Err != 0)
		{
			s32Err = MY_ERR(_Err_SYS + s32Err);
			goto err1;
		}
		s32Err = pthread_mutex_init(&(pCtrl->stMutex), &stAttr);
		pthread_mutexattr_destroy(&stAttr);
		if (s32Err != 0)
		{
			s32Err = MY_ERR(_Err_SYS + s32Err);
			goto err1;
		}
	}

	pCtrl->pFileCtrl = FileCtrlInit(pName, FILE_LOG_CNT, &s32Err);
	if (pCtrl->pFileCtrl == NULL)
	{
		goto err2;
	}

	if (pErr != NULL)
	{
		*pErr = 0;
	}
	return (int32_t)pCtrl;

err2:
	pthread_mutex_destroy(&(pCtrl->stMutex));
err1:
	free(pCtrl);
err:
	if (pErr != NULL)
	{
		*pErr = s32Err;
	}
	return 0;
}


/*
 * 函数名      : LogFileCtrlWriteLog
 * 功能        : 写入一条日志
 * 参数        : s32Handle [in] (int32_t) LogFileCtrlInit返回的句柄
 * 			   : pLog [in] (const char *): 日志内容, 字符串
 * 			   : s32Size [out] (int32_t): 正数表示内容的长度，负值，程序将会自动计算长度
 * 返回值      : int32_t 成功返回0，否则返回错误码
 * 作者        : 许龙杰
 */
int32_t LogFileCtrlWriteLog(int32_t s32Handle, const char *pLog, int32_t s32Size)
{
	int32_t s32Err = 0;
	StLogFileCtrl *pCtrl = (StLogFileCtrl *)s32Handle;

	if ((pCtrl == NULL) || (pLog == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}

	s32Err = pthread_mutex_lock(&(pCtrl->stMutex));
	if (s32Err != 0)
	{
		return MY_ERR(_Err_SYS + s32Err);
	}

	s32Err = FileCtrlWriteLog(pCtrl->pFileCtrl, pLog, s32Size);
	pthread_mutex_unlock(&(pCtrl->stMutex));

	return s32Err;
}

/*
 * 函数名      : LogFileCtrlGetAOldestBlockLog
 * 功能        : 读取一块最旧日志，成功后并使用过之后应该使用free函数释放*p2Log
 * 参数        : s32Handle [in] (int32_t) LogFileCtrlInit返回的句柄
 * 			   : p2Log [in] (const char **): 一块儿日志内容, 字符串
 * 			   : pSize [out] (int32_t *): 保存日志的长度
 * 返回值      : int32_t 成功返回0，否则返回错误码
 * 作者        : 许龙杰
 */
int32_t LogFileCtrlGetAOldestBlockLog(int32_t s32Handle, char **p2Log, uint32_t *pSize)
{
	int32_t s32Err = 0;
	StLogFileCtrl *pCtrl = (StLogFileCtrl *)s32Handle;

	if ((pCtrl == NULL) || (p2Log == NULL) || (pSize == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}

	s32Err = pthread_mutex_lock(&(pCtrl->stMutex));
	if (s32Err != 0)
	{
		return MY_ERR(_Err_SYS + s32Err);
	}

	s32Err = FileCtrlGetAOldestBlockLog(pCtrl->pFileCtrl, p2Log, pSize);
	pthread_mutex_unlock(&(pCtrl->stMutex));

	return s32Err;
}

/*
 * 函数名      : LogFileCtrlGetAOldestBlockLog
 * 功能        : 删除一块最旧日志
 * 参数        : s32Handle [in] (int32_t) LogFileCtrlInit返回的句柄
 * 返回值      : int32_t 成功返回0，否则返回错误码
 * 作者        : 许龙杰
 */
int32_t LogFileCtrlDelAOldestBlockLog(int32_t s32Handle)
{
	int32_t s32Err = 0;
	StLogFileCtrl *pCtrl = (StLogFileCtrl *)s32Handle;

	if (pCtrl == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}

	s32Err = pthread_mutex_lock(&(pCtrl->stMutex));
	if (s32Err != 0)
	{
		return MY_ERR(_Err_SYS + s32Err);
	}

	s32Err = FileCtrlDelAOldestBlockLog(pCtrl->pFileCtrl);
	pthread_mutex_unlock(&(pCtrl->stMutex));

	return s32Err;
}

/*
 * 函数名      : LogFileCtrlDestroy
 * 功能        : 销毁日志控制和相关资源
 * 参数        : s32Handle [in] (int32_t) LogFileCtrlInit返回的句柄
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void LogFileCtrlDestroy(int32_t s32Handle)
{
	StLogFileCtrl *pCtrl = (StLogFileCtrl *)s32Handle;
	if (pCtrl == NULL)
	{
		return;
	}
	FileCtrlDestroy(pCtrl->pFileCtrl);
	pthread_mutex_destroy(&(pCtrl->stMutex));
	free(pCtrl);
}


