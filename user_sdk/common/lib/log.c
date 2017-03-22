/****************************************************************************
 * Copyright(c), 2001-2060, ********************* 版权所有
 ****************************************************************************
 * 文件名称             : log.c
 * 版本                 : 0.0.0
 * 作者                 : 许龙杰
 * 创建日期             : 2014-4-8
 * 描述                 : 日志SDK源文件
 ****************************************************************************/
#include <common.h>
#include "common_define.h"


/*
 * 函数名      : PrintLog
 * 功能        : 保存日志信息, 使用方法和 C99 printf一致
 * 参数        : pFmt [in]: (const char * 类型) 打印的格式
 *             : ...: [in]: 可变参数
 * 返回值      : (int32_t类型) 0表示成功, 否则错误
 * 作者        : 许龙杰
 */
int32_t PrintLog(const char *pFmt, ...)
{
	va_list ap;

	int32_t s32PID = getpid();
	int32_t s32Err;
#if 1
	char c8ProcessName[_POSIX_PATH_MAX];
	char c8StrMax[MAX_LINE];

	s32Err = GetProcessNameFromPID(c8ProcessName, MAX_LINE, s32PID);
	if (0 == s32Err)
	{
		time_t s32Time;
		struct tm stTime, *pTime;

		uint32_t u32Size;

		struct timeval stTimeVal;

		int32_t s32Client = 0;

		gettimeofday(&stTimeVal, NULL);
		s32Time = stTimeVal.tv_sec;
		pTime = localtime(&s32Time);
		if (NULL == pTime)
		{
			return MY_ERR(_Err_Time);
		}
		stTime = *pTime; /* 得到当前时间 */

		/* 打印时间和进程名和进程ID */
		u32Size = snprintf(c8StrMax, MAX_LINE, LOG_DATE_FORMAT"[%s][%d] ",
				stTime.tm_year + 1900, stTime.tm_mon + 1, stTime.tm_mday,
				stTime.tm_hour, stTime.tm_min, stTime.tm_sec,
				(int32_t) stTimeVal.tv_usec, c8ProcessName, s32PID);

		/* 打印用户信息 */
		va_start(ap, pFmt);
		u32Size = vsnprintf(c8StrMax + u32Size, MAX_LINE - u32Size - 1, pFmt,
				ap);
		va_end(ap);

		if (u32Size < 0)
		{
			return MY_ERR(_Err_InvalidParam);
		}

		u32Size = strlen(c8StrMax);
		if (c8StrMax[u32Size - 1] != '\n')
		{
			c8StrMax[u32Size - 1] = '\n';
			c8StrMax[u32Size] = 0;
			u32Size += 1;
		}
		s32Client = ClientConnect(LOG_SOCKET);
		if (s32Client < 0)
		{
			return s32Client;
		}

		//s32Err = MCSSyncSend(s32Client, 1000, 0x00006001, u32Size, c8StrMax);
		s32Err = MCSSyncSendData(s32Client, 1000, u32Size, c8StrMax);
		close(s32Client);
	}
#else
	char c8StrMax[MAX_LINE];

	s32Err = GetProcessNameFromPID(c8StrMax, MAX_LINE, s32PID);
	if (0 == s32Err)
	{
		time_t s32Time;
		struct tm stTime, *pTime;

		uint32_t u32Size;
		uint32_t u32SizeTmp;
		struct timeval stTimeVal;

		int32_t s32Client = 0;

		gettimeofday(&stTimeVal, NULL);
		s32Time = stTimeVal.tv_sec;
		pTime = localtime(&s32Time);
		if (NULL == pTime)
		{
			s32Err = MY_ERR(_Err_Time);
		}
		stTime = *pTime; /* 得到当前时间 */

		int32_t s32MCSHandle = MCSOpen(&s32Err);

		if (0 != s32Err)
		{
			goto end;
		}
		/* 插入进程名字 */
		s32Err = MCSInsertACmd(s32MCSHandle, 0x00006001, 1, strlen(c8StrMax) + 1,
				c8StrMax, true);
		if (0 != s32Err)
		{
			goto end1;
		}

		/* 打印时间和进程ID */
		u32Size = snprintf(c8StrMax, MAX_LINE, LOG_DATE_FORMAT" [% 10d] ",
				stTime.tm_year + 1900, stTime.tm_mon + 1, stTime.tm_mday,
				stTime.tm_hour, stTime.tm_min, stTime.tm_sec, (int32_t)stTimeVal.tv_usec,
				s32PID);

		/* 打印用户信息 */
		va_start(ap, pFmt);
		u32Size = vsnprintf(c8StrMax + u32Size, MAX_LINE - u32Size, pFmt, ap);
		va_end(ap);
		if (u32Size < 0)
		{
			s32Err = MY_ERR(_Err_InvalidParam);
			goto end1;
		}
		s32Err = MCSInsertACmd(s32MCSHandle, 0x00006002, 1, strlen(c8StrMax) + 1,
				c8StrMax, true);
		if (0 != s32Err)
		{
			goto end1;
		}

		const char *pBuf = MCSGetStreamBuf(s32MCSHandle, &u32Size, &s32Err);
		if (0 != s32Err)
		{
			goto end1;
		}

		s32Client = ClientConnect(LOG_SOCKET);
		if (s32Client < 0)
		{
			MCSClose(s32MCSHandle);
			return s32Client;
		}

		/* 设置套接字选项,接收和发送超时时间 */
		stTimeVal.tv_sec = 1;
		stTimeVal.tv_usec = 0;
		if(setsockopt(s32Client, SOL_SOCKET, SO_RCVTIMEO, &stTimeVal, sizeof(struct timeval)) < 0)
		{
			s32Err = MY_ERR(_Err_SYS + errno);
			goto end1;
		}

		if(setsockopt(s32Client, SOL_SOCKET, SO_SNDTIMEO, &stTimeVal, sizeof(struct timeval)) < 0)
		{
			s32Err = MY_ERR(_Err_SYS + errno);
			goto end1;
		}

		u32SizeTmp = send(s32Client, pBuf, u32Size, MSG_NOSIGNAL);
		if (u32Size != u32SizeTmp)
		{
			s32Err = MY_ERR(_Err_SYS + errno);
		}

end1:
		MCSClose(s32MCSHandle);
		close(s32Client);
	}
end:
#endif
	return s32Err;
}
