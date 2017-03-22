/****************************************************************************
 * Copyright(c), 2001-2060, ******************************* 版权所有
 ****************************************************************************
 * 文件名称             : thread.c
 * 版本                 : 0.0.1
 * 作者                 : 许龙杰
 * 创建日期             : 2014年4月18日
 * 描述                 : 
 ****************************************************************************/
#include <common.h>
#include "common_define.h"

/*
 * 函数名      : MakeThread
 * 功能        : 创建分离线程
 * 参数        : pThread [in] (YA_PFUN_THREAD 类型): 线程指针
 *             : pArg [in] (void * 类型): 线程参数
 *             : boIsDetach [in] (bool 类型): 是否设置分离属性, 默认设置
 *             : pThreadId [out] (pthread_t * 类型): 当 boIsDetach 为 YA_FALSE 并且pThreadId
 *               不为NULL时, 此变量用于保存线程ID
 *             : boIsFIFO [in] (bool 类型): 是否设置FIFO调度, 默认不设置
 * 返回值      : 正确返回0, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t MakeThread(PFUN_THREAD pThread, void *pArg,
        bool boIsDetach, pthread_t *pThreadId, bool boIsFIFO)
{
    int32_t s32Err;
    pthread_t tid;
    pthread_attr_t stAttr;
    struct sched_param stParam;

    s32Err = pthread_attr_init(&stAttr);
    if (s32Err != 0)
    {
        return MY_ERR(_Err_SYS + s32Err);
    }
    if (boIsFIFO)
    {
        /* 不继承父进程调度属性 */
        s32Err = pthread_attr_setinheritsched(&stAttr, PTHREAD_EXPLICIT_SCHED);
        if (s32Err != 0)
        {
            pthread_attr_destroy(&stAttr);
            return MY_ERR(_Err_SYS + s32Err);
        }

        /* 设置调度属性 */
        s32Err = pthread_attr_setschedpolicy(&stAttr, SCHED_FIFO);
        if (s32Err != 0)
        {
            pthread_attr_destroy(&stAttr);
            return MY_ERR(_Err_SYS + s32Err);
        }
        stParam.sched_priority = 10;
        s32Err = pthread_attr_setschedparam(&stAttr, &stParam);
        if (s32Err != 0)
        {
            pthread_attr_destroy(&stAttr);
            return MY_ERR(_Err_SYS + s32Err);
        }
    }
    if (boIsDetach)
    {
        /* 设置分离属性 */
        s32Err = pthread_attr_setdetachstate(&stAttr, PTHREAD_CREATE_DETACHED);
    }
    if (0 == s32Err)
    {
        s32Err = pthread_create(&tid, &stAttr, pThread, pArg);
    }
    pthread_attr_destroy(&stAttr);

    if (pThreadId != NULL)
    {
        *pThreadId = tid;
    }
    if (s32Err != 0)
    {
        return MY_ERR(_Err_SYS + s32Err);
    }
    return 0;
}

