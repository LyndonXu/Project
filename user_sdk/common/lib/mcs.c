/****************************************************************************
 * Copyright(c), 2001-2060, ******************************* 版权所有
 ****************************************************************************
 * 文件名称             : mcs.c
 * 版本                 : 0.0.1
 * 作者                 : 许龙杰
 * 创建日期             : 2014年04月08日
 * 描述                 : MCS程序
 ****************************************************************************/
#include <common.h>
#include "common_define.h"


const uint8_t c_u8MixArr[4] = {0x4D, 0x43, 0x53, 0x01};

/*
 * 函数名      : LittleAndBigEndianTransfer
 * 功能        : 大端数据和小端数据之间的转换
 * 参数        : pDest[out] (char * 类型): 要转换的结果存放的位置
 *             : pSrc[in] (const char * 类型): 要转换的数据指针
 *             : u32Size[in] (uint32_t类型): pSrc 指向数据的字节数
 * 返回值      : 无
 * 作者        : 许龙杰
 * 例程:
 *  {
 *      uint32_t u32Tmp = 0x12345678, i;
 *      uint8_t u8Arr[4] = {0}, *pTmp;
 *      pTmp = (char *)&u32Tmp;
 *      for ( i = 0; i < 4; i++)
 *      {
 *          u8Arr[i] = pTmp[i];
 *      }
 *      printf("the little-endian data is 0x%08x, and its byte sequence is 0x%02hhx 0x%02hhx 0x%02hhx 0x%02hhx\n",
 *              u32Tmp, u8Arr[0], u8Arr[1], u8Arr[2], u8Arr[3]);
 *      LittleAndBigEndianTransfer(u8Arr, (char *)&u32Tmp, sizeof(uint32_t));
 *      printf("After transfer,  big-endian data's byte sequence is  0x%02hhx 0x%02hhx 0x%02hhx 0x%02hhx\n",
 *              u8Arr[0], u8Arr[1], u8Arr[2], u8Arr[3]);
 *
 *      printf("We want to transfer the big-endian data into little-endian\n");
 *      LittleAndBigEndianTransfer((char *)&u32Tmp1, u8Arr, sizeof(uint32_t));
 *      printf("After transfer, little-endian data is 0x%08x\n", u32Tmp1);
 *  }
 */
void LittleAndBigEndianTransfer(char *pDest, const char *pSrc, uint32_t u32Size)
{
    uint32_t i;
    for (i  = 0; i < u32Size; i++)
    {
        pDest[i] = pSrc[u32Size - i - 1];
    }
}

/*
 * 函数名      : MCSOpen
 * 功能        : 得到一个command stream 句柄
 *             必须与MCSClose成对出现
 * 参数        : pErr[out] (int32_t * 类型): 不为NULL时, 错误码, 0正确, 否则返回负数
 * 返回值      : int32_t 型数据, 0失败, 否则成功
 * 作者        : 许龙杰
 */
int32_t MCSOpen(int32_t *pErr)
{
    StMCS *pMCS = (StMCS *)calloc(1, sizeof(StMCS));
    if (NULL == pMCS)
    {
        if (pErr != NULL)
        {
            *pErr = MY_ERR(_Err_Mem);
        }
        return 0;
    }

    memcpy(pMCS->stMCSHeader.u8MixArr, c_u8MixArr, sizeof(c_u8MixArr));

    if (pErr != NULL)
    {
        *pErr = 0;
    }
    return (int32_t)pMCS;
}

/*
 * 函数名			: MCSClose
 * 功能			: 销毁MCSOpen返回的句柄中相关资源
 *   			  必须与MCSOpen成对出现
 * 参数			: s32MCSHandle[in] (int32_t类型): MCSOpen返回的句柄
 * 				: boIsReleaseStream [in] (bool类型): 是否释放构建好的流，如果不释放，用户必须使用MCSFree释放该流
 * 返回值			: 无
 * 作者			: 许龙杰
 */
void MCSCloseNoReleaseStream(int32_t s32MCSHandle, bool boIsReleaseStream)
{
    StMCS *pMCS = (StMCS *)s32MCSHandle;
    StMCSCmdTag *pCmd;

    if (NULL == pMCS)
    {
        return;
    }
    pCmd = pMCS->pFrontmostCmd;
    while (pCmd != NULL)
    {
        StMCSCmdTag *pCmdTmp = pCmd, *pCmdPart = NULL;
        if(pCmd->pData != NULL)
        {
            free(pCmd->pData);
        }
        pCmdPart = pCmd->pFrontmostPart;
        while (pCmdPart != NULL)/* 释放组合命令 */
        {
            StMCSCmdTag *pCmdPartTmp = pCmdPart;
            if (pCmdPart->pData != NULL)
            {
                free(pCmdPart->pData);
            }
            pCmdPart = pCmdPart->pLastPart;
            free(pCmdPartTmp);
        }
        pCmd = pCmd->pNextCmd;
        free(pCmdTmp);
    }
    if (boIsReleaseStream)
    {
		if(pMCS->pBuf != NULL)
		{
			free(pMCS->pBuf);
		}
    }

    free(pMCS);
}
/*
 * 函数名      : MCSClose
 * 功能        : 销毁MCSOpen返回的句柄中相关资源
 *             必须与MCSOpen成对出现
 * 参数        : s32MCSHandle[in] (int32_t类型): MCSOpen返回的句柄
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void MCSClose(int32_t s32MCSHandle)
{
	MCSCloseNoReleaseStream(s32MCSHandle, true);
}


/*
 * 函数名      : MCSFindPrevCmd
 * 功能        : 找到上一条命令
 * 参数        : pMCS[in] (StMCS *类型): MCSOpen返回的句柄
 *             : u32CmdNum[in] (uint32_t类型): 命令号
 * 返回值      : StMCSCmdTag * 型数据, NULL没有发现相符合的命令, 否则返回指针
 * 作者        : 许龙杰
 */
static StMCSCmdTag *MCSFindPrevCmd(StMCS *pMCS, uint32_t u32CmdNum)
{
    StMCSCmdTag *pCmd = NULL;
    pCmd = pMCS->pLastCmd;
    while (pCmd != NULL)
    {
        if (pCmd->stMCSCmdHeader.u32CmdNum == u32CmdNum)
        {
            return pCmd;
        }
        pCmd = pCmd->pPrevCmd;
    }
    return NULL;
}


/*
 * 函数名      : MCSInsertACmd
 * 功能        : 在句柄中插入一条命令
 * 参数        : s32MCSHandle[in] (int32_t类型): MCSOpen返回的句柄
 *             : u32CmdNum[in] (uint32_t类型): 命令号
 *             : u32CmdCnt[in] (uint32_t类型): 命令对应的命令数量
 *             : u32CmdSize[in] (uint32_t类型): 每一个命令的大小
 *             : pCmdData[in] (void *类型): 命令数据
 *             : boIsRepeatCmd[in] (bool 类型): 是否连接到以前命令
 * 返回值      : int32_t 型数据, 0成功, 否则失败
 * 作者        : 许龙杰
 */
int32_t MCSInsertACmd(int32_t s32MCSHandle,
        uint32_t u32CmdNum, uint32_t u32CmdCnt, uint32_t u32CmdSize,
        const void *pCmdData,
        bool boIsRepeatCmd)
{
    StMCS *pMCS = (StMCS *)s32MCSHandle;
    StMCSCmdTag *pCmd = NULL, *pPrevCmd = NULL;
    char *pSaveData = NULL;
    uint32_t u32RealCmdSize = 0;

    if (NULL == pMCS)
    {
        return MY_ERR(_Err_Handle);
    }

    pCmd = (StMCSCmdTag *)calloc(1, sizeof(StMCSCmdTag));
    if (NULL == pCmd)
    {
        return MY_ERR(_Err_Mem);
    }
    u32RealCmdSize = u32CmdCnt * u32CmdSize;
    if (u32RealCmdSize != 0)
    {
        if (NULL == pCmdData)
        {
            free(pCmd);
            return MY_ERR(_Err_NULLPtr);
        }
        pSaveData = (char *)malloc(u32RealCmdSize);
        if (NULL == pSaveData)
        {
            free(pCmd);
            return MY_ERR(_Err_Mem);
        }
        memcpy(pSaveData, pCmdData, u32RealCmdSize);
    }
    pCmd->stMCSCmdHeader.u32CmdNum = u32CmdNum;
    pCmd->stMCSCmdHeader.u32CmdCnt = u32CmdCnt;
    pCmd->stMCSCmdHeader.u32CmdSize = u32CmdSize;
    pCmd->pData = pSaveData;
    pCmd->u32TotalCnt = u32CmdCnt;
    if (boIsRepeatCmd)
    {
        pPrevCmd = MCSFindPrevCmd(pMCS, u32CmdNum);
    }
    if (pPrevCmd != NULL)/* 增加命令到以前的命令结尾 */
    {
        if (u32CmdSize != pPrevCmd->stMCSCmdHeader.u32CmdSize)
        {
            free(pCmd);
            free(pSaveData);
            return MY_ERR(_Err_CmdLen);  /* 错误命令长度 */
        }
        if (0 == pPrevCmd->stMCSCmdHeader.u32CmdCnt)
        {
            free(pCmd);
            free(pSaveData);
            return MY_ERR(_Err_CmdType);  /* 重复命令 */
        }

        if (NULL == pPrevCmd->pFrontmostPart)
        {
            pPrevCmd->pFrontmostPart = pCmd;
        }
        pCmd->pFrontmostPart = pPrevCmd->pLastPart;
        if (pPrevCmd->pLastPart != NULL)
        {
            pPrevCmd->pLastPart->pLastPart = pCmd;
        }
        pPrevCmd->pLastPart = pCmd;
        pPrevCmd->u32TotalCnt += u32CmdCnt;
    }
    else /* 新命令号 */
    {
        u32RealCmdSize += sizeof(StMCSCmdHeader);  /* 得到此命令头的大小 */

        if (NULL == pMCS->pFrontmostCmd)
        {
            pMCS->pFrontmostCmd = pCmd;
        }

        pCmd->pPrevCmd = pMCS->pLastCmd;

        if (pMCS->pLastCmd != NULL)
        {
            pMCS->pLastCmd->pNextCmd = pCmd;
        }

        pMCS->pLastCmd = pCmd;

        pMCS->stMCSHeader.u32CmdCnt++;
    }

    pMCS->stMCSHeader.u32CmdTotalSize += u32RealCmdSize;

    return 0;
}

/*
 * 函数名		: MCSMakeAnArrayVarialbleCmd
 * 功能		: 组建一个MCS流, 使用MCSFree 释放该指针
 * 参数		: u32CmdNum [in]: (uint32_t 类型) 命令号
 * 			  pData [in]: (void * 类型) 数据
 * 			  u32CmdCnt [in]: (uint32_t 类型) 数据数量
 * 			  u32CmdSize [in]: (uint32_t 类型) 单个数组的长度
 * 			  pCmdLength [out]: (uint32_t * 类型) 组建成功的长度
 * 返回值		: (void * 类型) 非NULL表示成功，指向数据, 否则表示错误
 * 作者		: 许龙杰
 */
void *MCSMakeAnArrayVarialbleCmd(uint32_t u32CmdNum, void *pData,
	uint32_t u32CmdCnt, uint32_t u32CmdSize, uint32_t *pCmdLength)
{
	int32_t s32Err = 0;
	void *pMCS;
	int32_t s32Handle = MCSOpen(&s32Err);
	if (s32Handle == 0)
	{
		return NULL;
	}
	s32Err = MCSInsertACmd(s32Handle, u32CmdNum, u32CmdCnt, u32CmdSize, pData, false);
	if (s32Err != 0)
	{
		return NULL;
	}
	pMCS = (void *)MCSGetStreamBuf(s32Handle, pCmdLength, &s32Err);

	if (pMCS == NULL)
	{
		MCSClose(s32Handle);
		return NULL;
	}
	else
	{
		MCSCloseNoReleaseStream(s32Handle, false);
		return pMCS;
	}
}

/*
 * 函数名      : MCSMalloc
 * 功能        : 得到一些内存
 * 参数        : u32Size[in] (uint32_t类型): 要申请内存的大小
 * 返回值      : void * 型数据, NULL失败，否则成功
 * 作者        : 许龙杰
 */
void *MCSMalloc(uint32_t u32Size)
{
    return malloc(u32Size);
}
/*
 * 函数名      : MCSFree
 * 功能        : 释放MCSMalloc得到的内存
 * 参数        : pBuf[in] (void *类型): MCSMalloc的返回值
 * 返回值      : void * 型数据, NULL失败，否则成功
 * 作者        : 许龙杰
 */
void MCSFree(void *pBuf)
{
    free(pBuf);
}
/*
 * 函数名      : MCSInsertACmdWithNoCopy
 * 功能        : 在句柄中插入一条命令, 但不拷贝命令载体, 命令载体必须使用MCSMalloc得到，
 *               成功之后一定不能使用MCSFree释放之，否则必须释放之
 * 参数        : s32MCSHandle[in] (int32_t类型): MCSOpen返回的句柄
 *             : u32CmdNum[in] (uint32_t类型): 命令号
 *             : u32CmdCnt[in] (uint32_t类型): 命令对应的命令数量
 *             : u32CmdSize[in] (uint32_t类型): 每一个命令的大小
 *             : pCmdData[in] (void *类型): 命令数据
 *             : boIsRepeatCmd[in] (bool 类型): 是否连接到以前命令
 * 返回值      : int32_t 型数据, 0成功, 否则失败
 * 作者        : 许龙杰
 */
int32_t MCSInsertACmdWithNoCopy(int32_t s32MCSHandle,
        uint32_t u32CmdNum, uint32_t u32CmdCnt, uint32_t u32CmdSize,
        const void *pCmdData,
        bool boIsRepeatCmd)
{
    StMCS *pMCS = (StMCS *)s32MCSHandle;
    StMCSCmdTag *pCmd = NULL, *pPrevCmd = NULL;
    char *pSaveData = NULL;
    uint32_t u32RealCmdSize = 0;

    if (NULL == pMCS)
    {
        return MY_ERR(_Err_Handle);
    }

    pCmd = (StMCSCmdTag *)calloc(1, sizeof(StMCSCmdTag));
    if (NULL == pCmd)
    {
        return MY_ERR(_Err_Mem);
    }
    u32RealCmdSize = u32CmdCnt * u32CmdSize;
    if (u32RealCmdSize != 0)
    {
        if (NULL == pCmdData)
        {
            free(pCmd);
            return MY_ERR(_Err_NULLPtr);
        }
        pSaveData = (char *)pCmdData;
    }
    pCmd->stMCSCmdHeader.u32CmdNum = u32CmdNum;
    pCmd->stMCSCmdHeader.u32CmdCnt = u32CmdCnt;
    pCmd->stMCSCmdHeader.u32CmdSize = u32CmdSize;
    pCmd->pData = pSaveData;
    pCmd->u32TotalCnt = u32CmdCnt;
    if (boIsRepeatCmd)
    {
        pPrevCmd = MCSFindPrevCmd(pMCS, u32CmdNum);
    }
    if (pPrevCmd != NULL)/* 增加命令到以前的命令结尾 */
    {
        if (u32CmdSize != pPrevCmd->stMCSCmdHeader.u32CmdSize)
        {
            free(pCmd);
            free(pSaveData);
            return MY_ERR(_Err_CmdLen);  /* 错误命令长度 */
        }
        if (0 == pPrevCmd->stMCSCmdHeader.u32CmdCnt)
        {
            free(pCmd);
            free(pSaveData);
            return MY_ERR(_Err_CmdType);  /* 重复命令 */
        }

        if (NULL == pPrevCmd->pFrontmostPart)
        {
            pPrevCmd->pFrontmostPart = pCmd;
        }
        pCmd->pFrontmostPart = pPrevCmd->pLastPart;
        if (pPrevCmd->pLastPart != NULL)
        {
            pPrevCmd->pLastPart->pLastPart = pCmd;
        }
        pPrevCmd->pLastPart = pCmd;
        pPrevCmd->u32TotalCnt += u32CmdCnt;
    }
    else /* 新命令号 */
    {
        u32RealCmdSize += sizeof(StMCSCmdHeader);  /* 得到此命令头的大小 */

        if (NULL == pMCS->pFrontmostCmd)
        {
            pMCS->pFrontmostCmd = pCmd;
        }

        pCmd->pPrevCmd = pMCS->pLastCmd;

        if (pMCS->pLastCmd != NULL)
        {
            pMCS->pLastCmd->pNextCmd = pCmd;
        }

        pMCS->pLastCmd = pCmd;

        pMCS->stMCSHeader.u32CmdCnt++;
    }

    pMCS->stMCSHeader.u32CmdTotalSize += u32RealCmdSize;

    return 0;
}


/*
 * 函数名      : MCSInsertAMCSCallBack
 * 功能        : MCSInsertAMCS的回调函数
 * 返回值      : int32_t 型数据, 0成功, 否则失败
 * 作者        : 许龙杰
 */
static int32_t MCSInsertAMCSCallBack(uint32_t u32CmdNum, uint32_t u32CmdCnt,
        uint32_t u32CmdSize, const char *pCmdData, void *pContext)
{
    return MCSInsertACmd((int32_t)pContext, u32CmdNum, u32CmdCnt, u32CmdSize, pCmdData, false);
}
/*
 * 函数名      : MCSInsertAMCS
 * 功能        : 在句柄中插入一条MCS流
 * 参数        : s32MCSHandle[in] (int32_t类型): MCSOpen返回的句柄
 *             : pMCS[in] (const char * 类型): 指向命令流
 *             : u32MCSSize[in] (uint32_t类型): 不为 0 的时候指定命令流的大小
 * 返回值      : int32_t 型数据, 0成功, 否则失败
 * 作者        : 许龙杰
 */
int32_t MCSInsertAMCS(int32_t s32MCSHandle, const char *pMCS, uint32_t u32MCSSize)
{
    int32_t s32Err = 0;
    if (0 == s32MCSHandle)
    {
        return MY_ERR(_Err_Handle);
    }
    s32Err = MCSResolve(pMCS, u32MCSSize, MCSInsertAMCSCallBack, (void *)s32MCSHandle);
    return s32Err;
}
/*
 * 函数名      : MCSGetStreamBuf
 * 功能        : 得到句柄中命令流
 * 参数        : s32MCSHandle[in] (int32_t类型): MCSOpen返回的句柄
 *             : pSize[out] (uint32_t类型): 不为 NULL 的时候保存命令流的大小
 *             : pErr[out] (int32_t * 类型): 不为NULL时, 错误码, 0正确, 否则返回负数
 * 返回值      :  char * 类型指针, 0错误, 否则正确
 * 作者        : 许龙杰
 */
const char *MCSGetStreamBuf(int32_t s32MCSHandle, uint32_t *pSize, int32_t *pErr)
{
    StMCS *pMCS = (StMCS *)s32MCSHandle;
    uint32_t u32TotalSize;
    StMCSCmdTag *pCmd = NULL, *pCmdPart = NULL;
    char *pData;

    if (NULL == pMCS)
    {
        if (pErr != NULL)
        {
            *pErr = MY_ERR(_Err_Handle);
        }
        if (pSize != NULL)
        {
            *pSize = 0;
        }
        return NULL;
    }
    if (0 == pMCS->stMCSHeader.u32CmdCnt)
    {
        if (pErr != NULL)
        {
            *pErr = MY_ERR(_Err_NoneCmdData);
        }
        if (pSize != NULL)
        {
            *pSize = 0;
        }
        return NULL;  /* 没有命令 */
    }

    u32TotalSize = pMCS->stMCSHeader.u32CmdTotalSize + sizeof(StMCSHeader);

    if (u32TotalSize == pMCS->u32TotalSize)  /* 没有新的命令加入句柄 */
    {
        return pMCS->pBuf;
    }

    if (pMCS->pBuf != NULL) /* 释放以前的数据 */
    {
        free(pMCS->pBuf);
    }

    pData = (char *)malloc(u32TotalSize);
    if (NULL == pData)
    {
        if (pErr != NULL)
        {
            *pErr = MY_ERR(_Err_Mem);
        }
        return NULL;
    }
    pMCS->pBuf = pData;
    pMCS->u32TotalSize = u32TotalSize;

    if (pSize != NULL)
    {
        *pSize = u32TotalSize;
    }

    memcpy(pData, pMCS->stMCSHeader.u8MixArr, sizeof(pMCS->stMCSHeader.u8MixArr));
    pData += sizeof(pMCS->stMCSHeader.u8MixArr);

    LittleAndBigEndianTransfer(pData, (char *)(&(pMCS->stMCSHeader.u32CmdCnt)), sizeof(uint32_t));
    pData += sizeof(uint32_t);

    LittleAndBigEndianTransfer(pData, (char *)(&(pMCS->stMCSHeader.u32CmdTotalSize)), sizeof(uint32_t));
    pData += sizeof(uint32_t);

    pData += sizeof(pMCS->stMCSHeader.u8CheckSumArr);

    pCmd = pMCS->pFrontmostCmd;
    while (pCmd != NULL)
    {

        u32TotalSize = pCmd->stMCSCmdHeader.u32CmdNum;
        LittleAndBigEndianTransfer(pData, (char *)(&u32TotalSize), sizeof(uint32_t));
        pData += sizeof(uint32_t);

        u32TotalSize = pCmd->u32TotalCnt;
        LittleAndBigEndianTransfer(pData, (char *)(&u32TotalSize), sizeof(uint32_t));
        pData += sizeof(uint32_t);

        u32TotalSize = pCmd->stMCSCmdHeader.u32CmdSize;
        LittleAndBigEndianTransfer(pData, (char *)(&u32TotalSize), sizeof(uint32_t));
        pData += sizeof(uint32_t);

        u32TotalSize *= pCmd->stMCSCmdHeader.u32CmdCnt; /* 得到此指针负载的实际数据数量 */
        memcpy(pData, pCmd->pData, u32TotalSize);
        pData += u32TotalSize;

        pCmdPart = pCmd->pFrontmostPart;
        while (pCmdPart != NULL)
        {
            u32TotalSize = pCmdPart->stMCSCmdHeader.u32CmdCnt * pCmdPart->stMCSCmdHeader.u32CmdSize;
            if (u32TotalSize != 0)
            {
                memcpy(pData, pCmdPart->pData, u32TotalSize);
                pData += u32TotalSize;
            }
            pCmdPart = pCmdPart->pLastPart;
        }

        pCmd = pCmd->pNextCmd;
    }

    return (const void *)pMCS->pBuf;
}

/*
 * 函数名      : MCSGetCmdLength
 * 功能        : 从MCS流中得到负载的命令大小
 * 参数        : pMCS[in] (const char * 类型): 指向命令流
 *             : pMCSCmdSize[out] (uint32_t *类型): 成功在*pMCSCmdSize中保存大小
 * 返回值      : int32_t 型数据, 0成功, 否则失败
 * 作者        : 许龙杰
 */
int32_t MCSGetCmdLength(const char *pMCS, uint32_t *pMCSCmdSize)
{
    uint32_t u32Tmp, u32Tmp1;
    if (NULL == pMCS )
    {
        return MY_ERR(_Err_Handle);
    }
    if (NULL == pMCSCmdSize)
    {
        return MY_ERR(_Err_NULLPtr);
    }

    {
    	const uint32_t *pTmp = (const uint32_t *)c_u8MixArr;
		u32Tmp = *pTmp;
	}
    u32Tmp1 = *((uint32_t *)pMCS);

    /* memcpy(&u32Tmp, c_u8MixArr, sizeof(c_u8MixArr));
    memcpy(&u32Tmp1, pMCS, sizeof(c_u8MixArr)); */

    if (u32Tmp != u32Tmp1)/* 表示以及版本错误 */
    {
        return MY_ERR(_Err_Identification);
    }
    LittleAndBigEndianTransfer((char *)pMCSCmdSize, pMCS + 8, sizeof(uint32_t));
    return 0;
}

/*
 * 函数名      : MCSGetCmdCnt
 * 功能        : 从MCS流中得到负载的命令数量
 * 参数        : pMCS[in] (const char * 类型): 指向命令流
 *             : pMCSCmdCnt[out] (uint32_t *类型): 成功在*pMCSCmdCnt中保存大小
 * 返回值      : int32_t 型数据, 0成功, 否则失败
 * 作者        : 许龙杰
 */
int32_t MCSGetCmdCnt(const char *pMCS, uint32_t *pMCSCmdCnt)
{
    uint32_t u32Tmp, u32Tmp1;
    if (NULL == pMCS )
    {
        return MY_ERR(_Err_Handle);
    }
    if (NULL == pMCSCmdCnt)
    {
        return MY_ERR(_Err_NULLPtr);
    }

    {
    	const uint32_t *pTmp = (const uint32_t *)c_u8MixArr;
		u32Tmp = *pTmp;
	}
	u32Tmp1 = *((uint32_t *)pMCS);

    /* memcpy(&u32Tmp, c_u8MixArr, sizeof(c_u8MixArr));
    memcpy(&u32Tmp1, pMCS, sizeof(c_u8MixArr)); */

    if (u32Tmp != u32Tmp1)/* 表示以及版本错误 */
    {
        return MY_ERR(_Err_Identification);
    }
    LittleAndBigEndianTransfer((char *)pMCSCmdCnt, pMCS + 4, sizeof(uint32_t));
    return 0;
}

/*
 * 函数名      : MCSResolve
 * 功能        : 解析命令流
 * 参数        : pMCS[in] (const char * 类型): 指向命令流
 *             : u32MCSSize[in] (uint32_t类型): 不为 0 的时候指定命令流的大小
 *             : pFunCallBack[in] (YA_PFUN_MCS_Resolve_CallBack类型): 命令解析回调函数
 *             : pContext[in] (void *类型): 用户上下文
 * 返回值      : int32_t 型数据, 0成功, 否则失败
 * 作者        : 许龙杰
 */
int32_t MCSResolve(const char *pMCS, uint32_t u32MCSSize, PFUN_MCS_Resolve_CallBack pFunCallBack, void *pContext)
{
    uint32_t u32Tmp, u32Tmp1, u32Tmp2;
    const char *pMCSEnd;
    if (NULL == pMCS )
    {
        return MY_ERR(_Err_Handle);
    }
    if (NULL == pFunCallBack)
    {
        return MY_ERR(_Err_NULLPtr);
    }
#if 0
    memcpy(&u32Tmp, c_u8MixArr, sizeof(c_u8MixArr));
    memcpy(&u32Tmp1, pMCS, sizeof(c_u8MixArr));

    if (u32Tmp != u32Tmp1)/* 表示以及版本错误 */
    {
        return -1;
    }

    if (0 == u32MCSSize)
    {
        /* 得到MCS命令流中的命令包总大小 */
        LittleAndBigEndianTransfer((char *)&u32MCSSize, pMCS + 8, sizeof(uint32_t));
        u32MCSSize += sizeof(StMCSHeader);  /* 得到MCS命令流的大小 */
    }
#else
    if (0 == u32MCSSize)
    {
        int32_t s32Err;
        /* 得到MCS命令流中的命令包总大小 */
        if ((s32Err = MCSGetCmdLength(pMCS, &u32MCSSize)) != 0)
        {
            return s32Err;
        }
        u32MCSSize += sizeof(StMCSHeader);  /* 得到MCS命令流的大小 */
    }
#endif

    {
	int32_t s32Err;
	/* 得到MCS命令流中的命令数量 */
	if ((s32Err = MCSGetCmdCnt(pMCS, &u32Tmp)) != 0)
	{
		return s32Err;
	}
    }

    pMCSEnd = pMCS + u32MCSSize;

    pMCS += sizeof(StMCSHeader);

    if (u32Tmp == 0)	/* 只有数据 */
    {
    	return pFunCallBack(0, 1, u32MCSSize - sizeof(StMCSHeader), pMCS, pContext);
    }

    while(pMCS < pMCSEnd)
    {
        int32_t s32Err;
        LittleAndBigEndianTransfer((char *)&u32Tmp, pMCS, sizeof(uint32_t));   /* 命令号 */
        pMCS += sizeof(uint32_t);

        LittleAndBigEndianTransfer((char *)&u32Tmp1, pMCS, sizeof(uint32_t));   /* 此命令号命令的数量 */
        pMCS += sizeof(uint32_t);

        LittleAndBigEndianTransfer((char *)&u32Tmp2, pMCS, sizeof(uint32_t));   /* 每一个命令的大小 */
        pMCS += sizeof(uint32_t);

        s32Err = pFunCallBack(u32Tmp, u32Tmp1, u32Tmp2, pMCS, pContext);

        if (s32Err != 0)
        {
            return s32Err;
        }

        pMCS += u32Tmp1 * u32Tmp2;

    }
    return 0;
}


/*
 * 函数名		: MCSSyncReceiveWithLimit
 * 功能			: 以MCS头作为同步头从SOCKET中接收数据, 与 MCSSyncFree成对使用, 负载长度不超u32MaxLength
 * 参数			: s32Socket[in] (int32_t类型): 要接收的SOCKET
 *				: boWantSyncHead [in] (bool类型): 是否希望在数据前端增加同步头
 *				： u32MaxLength [in]  (uint32_t): 最大负载长度
 *				: u32TimeOut[in] (uint32_t类型): 超时时间(ms)
 *				: pSize[out] (uint32_t * 类型): 保存数据的长度
 *				: pErr[out] (int32_t * 类型): 不为NULL时, *pErr中保存错误码
 * 返回值		: int32_t 型数据, 0成功, 否则失败
 * 作者			: 许龙杰
 */
void *MCSSyncReceiveWithLimit(int32_t s32Socket, bool boWantSyncHead,
		uint32_t u32MaxLength, uint32_t u32TimeOut, uint32_t *pSize, int32_t *pErr)
{
    void *pMCSStream = NULL;
    char c8MCSHeader[16];
    uint32_t u32Size = 0;
    uint32_t u32RemainLen = sizeof(c8MCSHeader);
    int32_t s32RecvLen = 0;
    char *pTmp = c8MCSHeader;
    int32_t s32Err = 0;

    fd_set stSet;
    struct timeval stTimeout;
    if (s32Socket <= 0)
    {
        if (pErr != NULL)
        {
            *pErr = MY_ERR(_Err_InvalidParam);
        }
        return NULL;
    }

    if (NULL == pSize)
    {
        if (pErr != NULL)
        {
            *pErr = MY_ERR(_Err_NULLPtr);
        }
        return NULL;
    }

    stTimeout.tv_sec = u32TimeOut / 1000;
    stTimeout.tv_usec = (u32TimeOut % 1000) * 1000;
    FD_ZERO(&stSet);
    FD_SET(s32Socket, &stSet);

    if (select(s32Socket + 1, &stSet, NULL, NULL, &stTimeout) <= 0)
    {
        if (pErr != NULL)
        {
            *pErr = MY_ERR(_Err_TimeOut);
        }
        *pSize = 0;
        return NULL;
    }
    /* 设置套接字选项,接收和发送超时时间 */
    if(setsockopt(s32Socket, SOL_SOCKET, SO_RCVTIMEO, &stTimeout, sizeof(struct timeval)) < 0)
    {
        if (pErr != NULL)
        {
            *pErr = MY_ERR(_Err_TimeOut);
        }
        *pSize = 0;
        return NULL;
    }

    if(setsockopt(s32Socket, SOL_SOCKET, SO_SNDTIMEO, &stTimeout, sizeof(struct timeval)) < 0)
    {
        if (pErr != NULL)
        {
            *pErr = MY_ERR(_Err_TimeOut);
        }
        *pSize = 0;
        return NULL;
    }

    while (u32RemainLen > 0) /* 接收同步头 */
    {
        s32RecvLen = recv(s32Socket, pTmp, u32RemainLen, 0);
        if (s32RecvLen <= 0)
        {
            s32Err = MY_ERR(_Err_SYS + errno);
            goto end;
        }
        u32RemainLen -= s32RecvLen;
        pTmp += s32RecvLen;
    }

    s32Err = MCSGetCmdLength(c8MCSHeader, &u32RemainLen); /* 解析负载命令长度 */
    if (s32Err != 0)
    {
        goto end;
    }

    if (u32MaxLength < u32RemainLen)
    {
    	char c8Tmp[256];
        s32RecvLen = 0;
        while ((int32_t) u32RemainLen > 0)/* 接收数据 */
        {
        	uint32_t u32NeedRecv = u32RemainLen > 256 ? 256 : u32RemainLen;
            s32RecvLen = recv(s32Socket, c8Tmp, u32NeedRecv, 0);
            if (s32RecvLen <= 0)
            {
            	break;
            }
            u32RemainLen -= s32RecvLen;
        }
        s32Err = MY_ERR(_Err_CmdLen);
    	goto end;
    }

    u32Size = u32RemainLen;
    if (boWantSyncHead)
    {
		u32Size += sizeof(c8MCSHeader);
    }

    pMCSStream = malloc(u32Size);
    if (NULL == pMCSStream)
    {
        /* 没有内存, 说明此进程, 已经崩溃, 退出以保证其他进程的运行 */
        kill(getpid(), SIGTERM);
        goto end;
    }

    pTmp = pMCSStream;
    if (boWantSyncHead)
    {
    	pTmp += sizeof(c8MCSHeader);
    }

    s32RecvLen = 0;
    while ((int32_t) u32RemainLen > 0)/* 接收数据 */
    {
        s32RecvLen = recv(s32Socket, pTmp, u32RemainLen, 0);
        if (s32RecvLen <= 0)
        {
            free(pMCSStream);
            pMCSStream = NULL;
            s32Err = MY_ERR(_Err_SYS + errno);
            u32Size = 0;
            goto end;
        }
        u32RemainLen -= s32RecvLen;
        pTmp += s32RecvLen;
    }
    if (boWantSyncHead)
    {
    	memcpy(pMCSStream, c8MCSHeader, sizeof(c8MCSHeader));/* 组装成一个完整的MCS流 */
    }
end:
    *pSize = u32Size;
    if (pErr != NULL)
    {
        *pErr =s32Err;
    }
    return pMCSStream;
}


/*
 * 函数名		: MCSSyncReceiveCmdWithCB
 * 功能			: 以MCS头作为同步头从SOCKET中接收数据, 比对命令号, 并将数据有效数据回调
 * 参数			: s32Socket[in] (int32_t类型): 要接收的SOCKET
 *				：u32WantCmd [in]  (uint32_t): 要接收的命令号
 *				: u32TimeOut[in] (uint32_t类型): 超时时间(ms)
 *				: pFun[in] (PFUN_MCSRecvCMDCB * 类型): 回调函数指针
 *				: pContext[in] (void * 类型): 回调函数上下文指针
 * 返回值		: int32_t 型数据, 0成功, 否则失败
 * 作者			: 许龙杰
 */
int32_t MCSSyncReceiveCmdWithCB(int32_t s32Socket, uint32_t u32WantCmd, uint32_t u32TimeOut,
		PFUN_MCSRecvCMDCB pFun, void *pContext)
{
    char c8MCSHeader[16];
    char c8MCSCmdInfo[3 * sizeof(uint32_t)];
    uint32_t u32Size = 0;
    uint32_t u32RemainLen = sizeof(c8MCSHeader);
    int32_t s32RecvLen = 0;
    char *pTmp = c8MCSHeader;
    int32_t s32Err = 0;

    fd_set stSet;
    struct timeval stTimeout;
    if (s32Socket <= 0)
    {
        return MY_ERR(_Err_InvalidParam);
    }

    stTimeout.tv_sec = u32TimeOut / 1000;
    stTimeout.tv_usec = (u32TimeOut % 1000) * 1000;
    FD_ZERO(&stSet);
    FD_SET(s32Socket, &stSet);

    if (select(s32Socket + 1, &stSet, NULL, NULL, &stTimeout) <= 0)
    {
        return MY_ERR(_Err_TimeOut);
    }
    /* 设置套接字选项,接收和发送超时时间 */
    if(setsockopt(s32Socket, SOL_SOCKET, SO_RCVTIMEO, &stTimeout, sizeof(struct timeval)) < 0)
    {
        return MY_ERR(_Err_SYS + errno);
    }

    if(setsockopt(s32Socket, SOL_SOCKET, SO_SNDTIMEO, &stTimeout, sizeof(struct timeval)) < 0)
    {
        return MY_ERR(_Err_SYS + errno);
    }

    while (u32RemainLen > 0) /* 接收同步头 */
    {
        s32RecvLen = recv(s32Socket, pTmp, u32RemainLen, 0);
        if (s32RecvLen <= 0)
        {
            s32Err = MY_ERR(_Err_SYS + errno);
            goto end;
        }
        u32RemainLen -= s32RecvLen;
        pTmp += s32RecvLen;
    }

    s32Err = MCSGetCmdLength(c8MCSHeader, &u32RemainLen); /* 解析负载命令长度 */
    if (s32Err != 0)
    {
        goto end;
    }

    u32Size = u32RemainLen;
    if (u32Size < 3 * sizeof(uint32_t))
    {
    	s32Err = MY_ERR(_Err_CmdLen);
        goto end;
	}

    u32RemainLen = 3 * sizeof(uint32_t);
    pTmp = c8MCSCmdInfo;
    while ((int32_t)u32RemainLen > 0) /* 接收同步头 */
    {
        s32RecvLen = recv(s32Socket, pTmp, u32RemainLen, 0);
        if (s32RecvLen <= 0)
        {
            s32Err = MY_ERR(_Err_SYS + errno);
            goto end;
        }
        u32RemainLen -= s32RecvLen;
        pTmp += s32RecvLen;
    }
    do
    {
		uint32_t u32Cmd, u32Cnt, u32Length;
		char *pData = c8MCSCmdInfo;
		/* command */
		LittleAndBigEndianTransfer((char *)(&u32Cmd), (const char *)pData, sizeof(uint32_t));
		pData += sizeof(uint32_t);
		/* count */
		LittleAndBigEndianTransfer((char *)(&u32Cnt), (const char *)pData, sizeof(uint32_t));
		pData += sizeof(uint32_t);
		/* length */
		LittleAndBigEndianTransfer((char *)(&u32Length), (const char *)pData, sizeof(uint32_t));
		pData += sizeof(uint32_t);
		if (u32Cmd != u32WantCmd)
		{
            s32Err = MY_ERR(_Err_CmdType);
            goto end;
		}
		if (u32Size - 3 * sizeof(uint32_t) != u32Length)
		{
            s32Err = MY_ERR(_Err_CmdLen);
            goto end;
		}
		u32Size = u32Length;
    } while (0);

    do
    {
    	pTmp = (char *)malloc(4096);
    	if (pTmp == NULL)
    	{
    		s32Err = MY_ERR(_Err_Mem);
    		goto end;
    	}
		s32RecvLen = 0;
		u32RemainLen = u32Size;
		while ((int32_t) u32RemainLen > 0)/* 接收数据 */
		{
			uint32_t u32NeedRecv = u32RemainLen > 4096 ? 4096 : u32RemainLen;
			s32RecvLen = recv(s32Socket, pTmp, u32NeedRecv, 0);
			if (s32RecvLen <= 0)
			{
				s32Err = MY_ERR(_Err_SYS + errno);
				break;
			}
			u32RemainLen -= s32RecvLen;
			if (pFun != NULL)
			{
				pFun(pTmp, s32RecvLen, pContext);
			}
		}
		free(pTmp);

    } while(0);

end:
    return s32Err;
}

/*
 * 函数名      : MCSSyncReceive
 * 功能        : 以MCS头作为同步头从SOCKET中接收数据, 与 MCSSyncFree成对使用
 * 参数        : s32Socket[in] (int32_t类型): 要接收的SOCKET
 *             : boWantSyncHead [in] (bool类型): 是否希望在数据前端增加同步头
 *             : u32TimeOut[in] (uint32_t类型): 超时时间(ms)
 *             : pSize[out] (uint32_t * 类型): 保存数据的长度
 *             : pErr[out] (int32_t * 类型): 不为NULL时, *pErr中保存错误码
 * 返回值      : int32_t 型数据, 0成功, 否则失败
 * 作者        : 许龙杰
 */
void *MCSSyncReceive(int32_t s32Socket, bool boWantSyncHead,
		uint32_t u32TimeOut, uint32_t *pSize, int32_t *pErr)
{
	return MCSSyncReceiveWithLimit(s32Socket, boWantSyncHead, ~0, u32TimeOut, pSize, pErr);
}
/*
 * 函数名      : MCSSyncFree
 * 功能        : 释放数据, 与 MCSSyncReceive成对使用
 * 参数        : 无
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void MCSSyncFree(void *pData)
{
    if (pData != NULL)
    {
        free(pData);
    }
}

/*
 * 函数名      : MCSSyncSendArr
 * 功能        : 以MCS头作为同步头向SOCKET中发送一组命令数据
 * 参数        : s32Socket[in] (int32_t类型): 要放送到的SOCKET
 *             : u32TimeOut[in] (uint32_t类型): 超时(ms)
 *             : u32CommandNum[in] (uint32_t 类型): 数据对应的命令号(根据情况可以添0)
 *             : u32Cnt[in] (uint32_t 类型): 数组数量
 *             : u32ElementSize[in] (uint32_t 类型): 数组一个元素的大小
 *             : pData[in] (const void * 类型): 数组数据
 * 返回值      : int32_t 型数据, 0成功, 否则失败
 * 作者        : 许龙杰
 */
int32_t MCSSyncSendArr(int32_t s32Socket,  uint32_t u32TimeOut, uint32_t u32CommandNum,
		uint32_t u32Cnt, uint32_t u32ElementSize, const void *pData)
{
    int32_t s32Err = 0;
    uint32_t u32Tmp = 0;
    char c8HeaderMix[sizeof(StMCSHeader) + sizeof(StMCSCmdHeader)] = {0};
    StMCSHeader *pMCSHeader;
    StMCSCmdHeader *pMCSCmdHeader;
    struct timeval stTimeout;
    uint32_t u32Size = u32Cnt * u32ElementSize;

    if (u32Size != 0 && pData == NULL)
    {
    	return MY_ERR(_Err_InvalidParam);
    }

    if (s32Socket <= 0)
    {
        return MY_ERR(_Err_InvalidParam);
    }

    pMCSHeader = (StMCSHeader *)c8HeaderMix;
    pMCSCmdHeader = (StMCSCmdHeader *)(c8HeaderMix + sizeof(StMCSHeader));
    memcpy(pMCSHeader->u8MixArr, c_u8MixArr, sizeof(c_u8MixArr));

    u32Tmp = 1;
    LittleAndBigEndianTransfer((char *)(&(pMCSHeader->u32CmdCnt)),
            (char *)(&u32Tmp), sizeof(uint32_t));

    u32Tmp = sizeof(StMCSCmdHeader) + u32Size;
    LittleAndBigEndianTransfer((char *)(&(pMCSHeader->u32CmdTotalSize)),
            (char *)(&u32Tmp), sizeof(uint32_t));

    LittleAndBigEndianTransfer((char *)(&(pMCSCmdHeader->u32CmdNum)),
            (char *)(&u32CommandNum), sizeof(uint32_t));
    LittleAndBigEndianTransfer((char *)(&(pMCSCmdHeader->u32CmdCnt)),
            (char *)(&u32Cnt), sizeof(uint32_t));
    LittleAndBigEndianTransfer((char *)(&(pMCSCmdHeader->u32CmdSize)),
            (char *)(&u32ElementSize), sizeof(uint32_t));

    /* 设置套接字选项,接收和发送超时时间 */
    stTimeout.tv_sec  = u32TimeOut / 1000;
    stTimeout.tv_usec = (u32TimeOut % 1000) * 1000;
    if(setsockopt(s32Socket, SOL_SOCKET, SO_RCVTIMEO, &stTimeout, sizeof(struct timeval)) < 0)
    {
         return MY_ERR(_Err_SYS + errno);
    }

    if(setsockopt(s32Socket, SOL_SOCKET, SO_SNDTIMEO, &stTimeout, sizeof(struct timeval)) < 0)
    {
        return MY_ERR(_Err_SYS + errno);
    }

    u32Tmp = send(s32Socket, c8HeaderMix, sizeof(c8HeaderMix), MSG_NOSIGNAL);
    if (u32Tmp != sizeof(c8HeaderMix))
    {
        s32Err = MY_ERR(_Err_SYS + errno);
        goto end;
    }
    if (pData != NULL)
    {
        u32Tmp = send(s32Socket, pData, u32Size, MSG_NOSIGNAL);
        if (u32Tmp != u32Size)
        {
            s32Err = MY_ERR(_Err_SYS + errno);
            goto end;
        }
    }

end:
    return s32Err;

}
/*
 * 函数名      : MCSSyncSend
 * 功能        : 以MCS头作为同步头向SOCKET中发送命令数据
 * 参数        : s32Socket[in] (int32_t类型): 要放送到的SOCKET
 *             : u32TimeOut[in] (uint32_t类型): 超时(ms)
 *             : u32CommandNum[in] (uint32_t 类型): 数据对应的命令号(根据情况可以添0)
 *             : u32Size[in] (uint32_t 类型): 数据的长度
 *             : pData[in] (const void * 类型): 数据
 * 返回值      : int32_t 型数据, 0成功, 否则失败
 * 作者        : 许龙杰
 */
int32_t MCSSyncSend(int32_t s32Socket,  uint32_t u32TimeOut, uint32_t u32CommandNum, uint32_t u32Size, const void *pData)
{
#if 1
	return MCSSyncSendArr(s32Socket, u32TimeOut, u32CommandNum, 1, u32Size, pData);
#else
    int32_t s32Err = 0;
    uint32_t u32Tmp = 0;
    char c8HeaderMix[sizeof(StMCSHeader) + sizeof(StMCSCmdHeader)] = {0};
    StMCSHeader *pMCSHeader;
    StMCSCmdHeader *pMCSCmdHeader;
    struct timeval stTimeout;

    if (s32Socket <= 0)
    {
        return MY_ERR(_Err_InvalidParam);
    }

    pMCSHeader = (StMCSHeader *)c8HeaderMix;
    pMCSCmdHeader = (StMCSCmdHeader *)(c8HeaderMix + sizeof(StMCSHeader));
    memcpy(pMCSHeader->u8MixArr, c_u8MixArr, sizeof(c_u8MixArr));

    u32Tmp = 1;
    LittleAndBigEndianTransfer((char *)(&(pMCSHeader->u32CmdCnt)),
            (char *)(&u32Tmp), sizeof(uint32_t));
    LittleAndBigEndianTransfer((char *)(&(pMCSCmdHeader->u32CmdCnt)),
            (char *)(&u32Tmp), sizeof(uint32_t));
    u32Tmp = sizeof(StMCSCmdHeader) + u32Size;
    LittleAndBigEndianTransfer((char *)(&(pMCSHeader->u32CmdTotalSize)),
            (char *)(&u32Tmp), sizeof(uint32_t));

    LittleAndBigEndianTransfer((char *)(&(pMCSCmdHeader->u32CmdNum)),
            (char *)(&u32CommandNum), sizeof(uint32_t));
    LittleAndBigEndianTransfer((char *)(&(pMCSCmdHeader->u32CmdSize)),
            (char *)(&u32Size), sizeof(uint32_t));

    /* 设置套接字选项,接收和发送超时时间 */
    stTimeout.tv_sec  = u32TimeOut / 1000;
    stTimeout.tv_usec = (u32TimeOut % 1000) * 1000;
    if(setsockopt(s32Socket, SOL_SOCKET, SO_RCVTIMEO, &stTimeout, sizeof(struct timeval)) < 0)
    {
         return MY_ERR(_Err_SYS + errno);
    }

    if(setsockopt(s32Socket, SOL_SOCKET, SO_SNDTIMEO, &stTimeout, sizeof(struct timeval)) < 0)
    {
        return MY_ERR(_Err_SYS + errno);
    }

    u32Tmp = send(s32Socket, c8HeaderMix, sizeof(c8HeaderMix), MSG_NOSIGNAL);
    if (u32Tmp != sizeof(c8HeaderMix))
    {
        s32Err = MY_ERR(_Err_SYS + errno);
        goto end;
    }
    if (pData != NULL)
    {
        u32Tmp = send(s32Socket, pData, u32Size, MSG_NOSIGNAL);
        if (u32Tmp != u32Size)
        {
            s32Err = MY_ERR(_Err_SYS + errno);
            goto end;
        }
    }

end:
    return s32Err;
#endif
}
/*
 * 函数名      : MCSSyncSendFile
 * 功能        : 以MCS头作为同步头向SOCKET中发送文件数据
 * 参数        : s32Socket[in] (int32_t类型): 要放送到的SOCKET
 *             : u32TimeOut[in] (uint32_t类型): 超时(ms)
 *             : u32CommandNum[in] (uint32_t 类型): 数据对应的命令号(根据情况可以添0)
 *             : pFileName[in] (const char * 类型): 要发送文件的名称
 * 返回值      : int32_t 型数据, 0成功, 否则失败
 * 作者        : 许龙杰
 */
int32_t MCSSyncSendFile(int32_t s32Socket,  uint32_t u32TimeOut,
		uint32_t u32CommandNum, const char *pFileName)
{
    int32_t s32Err = 0;
    uint32_t u32Tmp = 0;
    char c8HeaderMix[sizeof(StMCSHeader) + sizeof(StMCSCmdHeader)] = {0};
    StMCSHeader *pMCSHeader;
    StMCSCmdHeader *pMCSCmdHeader;
    struct timeval stTimeout;
    uint32_t u32Size = 0;
    FILE *pFile = NULL;

    if (pFileName == NULL)
    {
    	return MY_ERR(_Err_InvalidParam);
    }

    if (s32Socket <= 0)
    {
        return MY_ERR(_Err_InvalidParam);
    }

    pFile = fopen(pFileName, "rb");
    if (pFile == NULL)
    {
        return MY_ERR(_Err_InvalidParam);
    }

    fseek(pFile, 0, SEEK_END);
    u32Size = ftell(pFile);

    pMCSHeader = (StMCSHeader *)c8HeaderMix;
    pMCSCmdHeader = (StMCSCmdHeader *)(c8HeaderMix + sizeof(StMCSHeader));
    memcpy(pMCSHeader->u8MixArr, c_u8MixArr, sizeof(c_u8MixArr));

    u32Tmp = 1;
    LittleAndBigEndianTransfer((char *)(&(pMCSHeader->u32CmdCnt)),
            (char *)(&u32Tmp), sizeof(uint32_t));

    u32Tmp = sizeof(StMCSCmdHeader) + u32Size;
    LittleAndBigEndianTransfer((char *)(&(pMCSHeader->u32CmdTotalSize)),
            (char *)(&u32Tmp), sizeof(uint32_t));

    LittleAndBigEndianTransfer((char *)(&(pMCSCmdHeader->u32CmdNum)),
            (char *)(&u32CommandNum), sizeof(uint32_t));

    u32Tmp = 1;
    LittleAndBigEndianTransfer((char *)(&(pMCSCmdHeader->u32CmdCnt)),
            (char *)(&u32Tmp), sizeof(uint32_t));
    LittleAndBigEndianTransfer((char *)(&(pMCSCmdHeader->u32CmdSize)),
            (char *)(&u32Size), sizeof(uint32_t));

    /* 设置套接字选项,接收和发送超时时间 */
    stTimeout.tv_sec  = u32TimeOut / 1000;
    stTimeout.tv_usec = (u32TimeOut % 1000) * 1000;
    if(setsockopt(s32Socket, SOL_SOCKET, SO_RCVTIMEO, &stTimeout, sizeof(struct timeval)) < 0)
    {
         s32Err = MY_ERR(_Err_SYS + errno);
         goto end;
    }

    if(setsockopt(s32Socket, SOL_SOCKET, SO_SNDTIMEO, &stTimeout, sizeof(struct timeval)) < 0)
    {
        s32Err = MY_ERR(_Err_SYS + errno);
        goto end;
    }

    u32Tmp = send(s32Socket, c8HeaderMix, sizeof(c8HeaderMix), MSG_NOSIGNAL);
    if (u32Tmp != sizeof(c8HeaderMix))
    {
        s32Err = MY_ERR(_Err_SYS + errno);
        goto end;
    }
    do
    {
    	void *pData = malloc(4096);
    	if (pData == NULL)
    	{
    		s32Err = MY_ERR(_Err_Mem);
    		goto end;
    	}
        fseek(pFile, 0, SEEK_SET);
		while(((int32_t)u32Size) > 0)
		{
			uint32_t u32NeedSend = u32Size > 4096 ? 4096 : u32Size;
			int32_t s32ReadCnt = fread(pData, 1, u32NeedSend, pFile);
			if (s32ReadCnt != u32NeedSend)
			{
				break;
			}
			u32Tmp = send(s32Socket, pData, u32NeedSend, MSG_NOSIGNAL);
			if (u32Tmp != u32NeedSend)
			{
				s32Err = MY_ERR(_Err_SYS + errno);
				break;
			}
			u32Size -= u32NeedSend;
		}
    } while (0);

end:
	if (pFile != NULL)
	{
		fclose(pFile);
	}
    return s32Err;

}


/*
 * 函数名      : MCSSyncSendData
 * 功能        : 以MCS头作为同步头向SOCKET中发送数据
 * 参数        : s32Socket[in] (int32_t类型): 要放送到的SOCKET
 *             : u32TimeOut[in] (uint32_t类型): 超时(ms)
 *             : u32Size[in] (uint32_t 类型): 数据的长度
 *             : pData[in] (const void * 类型): 数据
 * 返回值      : int32_t 型数据, 0成功, 否则失败
 * 作者        : 许龙杰
 */
int32_t MCSSyncSendData(int32_t s32Socket,  uint32_t u32TimeOut, uint32_t u32Size, const void *pData)
{
    int32_t s32Err = 0;
    uint32_t u32Tmp = 0;
    char c8HeaderMix[sizeof(StMCSHeader)] = {0};
    StMCSHeader *pMCSHeader;
    struct timeval stTimeout;

    if (s32Socket <= 0)
    {
        return MY_ERR(_Err_InvalidParam);
    }

    pMCSHeader = (StMCSHeader *)c8HeaderMix;
    memcpy(pMCSHeader->u8MixArr, c_u8MixArr, sizeof(c_u8MixArr));

    u32Tmp = 0;
    LittleAndBigEndianTransfer((char *)(&(pMCSHeader->u32CmdCnt)),
            (char *)(&u32Tmp), sizeof(uint32_t));

    u32Tmp = u32Size;
    LittleAndBigEndianTransfer((char *)(&(pMCSHeader->u32CmdTotalSize)),
            (char *)(&u32Tmp), sizeof(uint32_t));


    /* 设置套接字选项,接收和发送超时时间 */
    stTimeout.tv_sec  = u32TimeOut / 1000;
    stTimeout.tv_usec = (u32TimeOut % 1000) * 1000;
    if(setsockopt(s32Socket, SOL_SOCKET, SO_RCVTIMEO, &stTimeout, sizeof(struct timeval)) < 0)
    {
        close(s32Socket);
        return MY_ERR(_Err_SYS + errno);
    }

    if(setsockopt(s32Socket, SOL_SOCKET, SO_SNDTIMEO, &stTimeout, sizeof(struct timeval)) < 0)
    {
        close(s32Socket);
        return MY_ERR(_Err_SYS + errno);
    }

    u32Tmp = send(s32Socket, c8HeaderMix, sizeof(c8HeaderMix), MSG_NOSIGNAL);
    if (u32Tmp != sizeof(c8HeaderMix))
    {
        s32Err = MY_ERR(_Err_SYS + errno);
        goto end;
    }
    if (pData != NULL)
    {
        u32Tmp = send(s32Socket, pData, u32Size, MSG_NOSIGNAL);
        if (u32Tmp != u32Size)
        {
            s32Err = MY_ERR(_Err_SYS + errno);
            goto end;
        }
    }
end:
    return s32Err;
}


/*
 * 函数名      : MCSSyncSendV
 * 功能        : 以MCS头作为同步头向SOCKET中发送多个缓冲数据
 * 参数        : s32Socket[in] (int32_t类型): 要放送到的SOCKET
 *             : u32TimeOut[in] (uint32_t类型): 超时(ms)
 *             : u32CommandNum[in] (uint32_t 类型): 数据对应的命令号(根据情况可以添0)
 *             : pSendV[in] (const StSendV * 类型): 多个缓冲数据头指针, 详见其定义
 *             : u32SendVCnt [in] (uint32_t 类型): 有多少个这样的StSendV
 * 返回值      : int32_t 型数据, 0成功, 否则失败
 * 作者        : 许龙杰
 */
int32_t MCSSyncSendV(int32_t s32Socket, uint32_t u32TimeOut, uint32_t u32CommandNum, const StSendV *pSendV, uint32_t u32SendVCnt)
{
    int32_t s32Err = 0;
    uint32_t u32Tmp = 0, i;
    uint32_t u32Size = 0;
    char c8HeaderMix[sizeof(StMCSHeader) + sizeof(StMCSCmdHeader)] = {0};
    StMCSHeader *pMCSHeader;
    StMCSCmdHeader *pMCSCmdHeader;
    struct timeval stTimeout;

    if ((s32Socket <= 0) || (NULL == pSendV))
    {
        return MY_ERR(_Err_InvalidParam);
    }

    for (i = 0; i < u32SendVCnt; i++)
    {
        u32Size += pSendV[i].u32Size;
    }

    pMCSHeader = (StMCSHeader *)c8HeaderMix;
    pMCSCmdHeader = (StMCSCmdHeader *)(c8HeaderMix + sizeof(StMCSHeader));
    memcpy(pMCSHeader->u8MixArr, c_u8MixArr, sizeof(c_u8MixArr));

    u32Tmp = 1;
    LittleAndBigEndianTransfer((char *)(&(pMCSHeader->u32CmdCnt)),
            (char *)(&u32Tmp), sizeof(uint32_t));
    LittleAndBigEndianTransfer((char *)(&(pMCSCmdHeader->u32CmdCnt)),
            (char *)(&u32Tmp), sizeof(uint32_t));

    u32Tmp = sizeof(StMCSCmdHeader) + u32Size;
    LittleAndBigEndianTransfer((char *)(&(pMCSHeader->u32CmdTotalSize)),
            (char *)(&u32Tmp), sizeof(uint32_t));

    LittleAndBigEndianTransfer((char *)(&(pMCSCmdHeader->u32CmdNum)),
            (char *)(&u32CommandNum), sizeof(uint32_t));

    LittleAndBigEndianTransfer((char *)(&(pMCSCmdHeader->u32CmdSize)),
            (char *)(&u32Size), sizeof(uint32_t));

    /* 设置套接字选项,接收和发送超时时间 */
    stTimeout.tv_sec  = u32TimeOut / 1000;
    stTimeout.tv_usec = (u32TimeOut % 1000) * 1000;
    if(setsockopt(s32Socket, SOL_SOCKET, SO_RCVTIMEO, &stTimeout, sizeof(struct timeval)) < 0)
    {
        close(s32Socket);
        return MY_ERR(_Err_SYS + errno);
    }

    if(setsockopt(s32Socket, SOL_SOCKET, SO_SNDTIMEO, &stTimeout, sizeof(struct timeval)) < 0)
    {
        close(s32Socket);
        return MY_ERR(_Err_SYS + errno);
    }


    u32Tmp = send(s32Socket, c8HeaderMix, sizeof(c8HeaderMix), MSG_NOSIGNAL);
    if (u32Tmp != sizeof(c8HeaderMix))
    {
        s32Err = MY_ERR(_Err_SYS + errno);
        goto end;
    }
    for (i = 0; i < u32SendVCnt; i++)
    {
        if (pSendV[i].pData != NULL)
        {
            u32Tmp = send(s32Socket, pSendV[i].pData, pSendV[i].u32Size, MSG_NOSIGNAL);
            if (u32Tmp != pSendV[i].u32Size)
            {
                s32Err = MY_ERR(_Err_SYS + errno);
                goto end;
            }
        }
    }
end:
    return s32Err;
}


/*
 * 函数名      : MCSSyncSendAMCS
 * 功能        : 以MCS头作为同步头向SOCKET中发送一个MCS数据
 * 参数        : s32Socket[in] (int32_t类型): 要放送到的SOCKET
 *             : u32TimeOut[in] (uint32_t类型): 超时(ms)
 *             : s32MCSHandle[in] (int32_t类型): MCSOpen返回的句柄
 *             : u32Size[in] (uint32_t类型): 不为0时指明MCS的大小
 * 返回值      : int32_t, 0正确, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t MCSSyncSendAMCS(int32_t s32Socket, uint32_t u32TimeOut, int32_t s32MCSHandle, uint32_t u32Size)
{
    StMCS *pMCS = (StMCS *)s32MCSHandle;
    uint32_t u32TotalSize;
    StMCSCmdTag *pCmd = NULL, *pCmdPart = NULL;
    char c8HeaderMix[sizeof(StMCSHeader) + sizeof(StMCSCmdHeader)] = {0};
    StMCSHeader *pMCSHeader;
    StMCSCmdHeader *pMCSCmdHeader;

    struct timeval stTimeout;
    uint32_t u32Tmp;

    if ((NULL == pMCS) || (s32Socket <= 0))
    {
        return MY_ERR(_Err_InvalidParam);
    }
    if (0 == pMCS->stMCSHeader.u32CmdCnt)
    {
        return MY_ERR(_Err_NoneCmdData);  /* 没有命令 */
    }
    if (0 == u32Size)
    {
        u32TotalSize = pMCS->stMCSHeader.u32CmdTotalSize + sizeof(StMCSHeader);
    }
    else
    {
        u32TotalSize = u32Size;
    }


    /* 设置套接字选项,接收和发送超时时间 */
    stTimeout.tv_sec  = u32TimeOut / 1000;
    stTimeout.tv_usec = (u32TimeOut % 1000) * 1000;
    if(setsockopt(s32Socket, SOL_SOCKET, SO_RCVTIMEO, &stTimeout, sizeof(struct timeval)) < 0)
    {
        close(s32Socket);
        return MY_ERR(_Err_SYS + errno);
    }

    if(setsockopt(s32Socket, SOL_SOCKET, SO_SNDTIMEO, &stTimeout, sizeof(struct timeval)) < 0)
    {
        close(s32Socket);
        return MY_ERR(_Err_SYS + errno);
    }

    pMCSHeader = (StMCSHeader *)c8HeaderMix;
    pMCSCmdHeader = (StMCSCmdHeader *)(c8HeaderMix + sizeof(StMCSHeader));
    memcpy(pMCSHeader->u8MixArr, c_u8MixArr, sizeof(c_u8MixArr));

    LittleAndBigEndianTransfer((char *)(&(pMCSHeader->u32CmdCnt)),
            (char *)(&(pMCS->stMCSHeader.u32CmdCnt)), sizeof(uint32_t));

    LittleAndBigEndianTransfer((char *)(&(pMCSHeader->u32CmdTotalSize)),
            (char *)(&(pMCS->stMCSHeader.u32CmdTotalSize)), sizeof(uint32_t));

    u32Tmp = send(s32Socket, pMCSHeader, sizeof(StMCSHeader), MSG_NOSIGNAL);
    if (u32Tmp != sizeof(StMCSHeader))
    {
        return MY_ERR(_Err_SYS + errno);
    }

    pCmd = pMCS->pFrontmostCmd;
    while (pCmd != NULL)
    {

        u32TotalSize = pCmd->stMCSCmdHeader.u32CmdNum;
        LittleAndBigEndianTransfer((char *)(&(pMCSCmdHeader->u32CmdNum)),
                (char *)(&u32TotalSize), sizeof(uint32_t));

        u32TotalSize = pCmd->u32TotalCnt;
        LittleAndBigEndianTransfer((char *)(&(pMCSCmdHeader->u32CmdCnt)),
                (char *)(&u32TotalSize), sizeof(uint32_t));


        u32TotalSize = pCmd->stMCSCmdHeader.u32CmdSize;
        LittleAndBigEndianTransfer((char *)(&(pMCSCmdHeader->u32CmdSize)),
                (char *)(&u32TotalSize), sizeof(uint32_t));

        u32Tmp = send(s32Socket, pMCSCmdHeader, sizeof(StMCSCmdHeader), MSG_NOSIGNAL);
        if (u32Tmp != sizeof(StMCSCmdHeader))
        {
            return MY_ERR(_Err_SYS + errno);
        }

        u32TotalSize *= pCmd->stMCSCmdHeader.u32CmdCnt; /* 得到此指针负载的实际数据数量 */
        if (u32TotalSize != 0)
        {
            u32Tmp = send(s32Socket, pCmd->pData, u32TotalSize, MSG_NOSIGNAL);
            if (u32Tmp != u32TotalSize)
            {
                return MY_ERR(_Err_SYS + errno);
            }
        }

        pCmdPart = pCmd->pFrontmostPart;
        while (pCmdPart != NULL)
        {
            u32TotalSize = pCmdPart->stMCSCmdHeader.u32CmdCnt * pCmdPart->stMCSCmdHeader.u32CmdSize;
            if (u32TotalSize != 0)
            {
                u32Tmp = send(s32Socket, pCmdPart->pData, u32TotalSize, MSG_NOSIGNAL);
                if (u32Tmp != u32TotalSize)
                {
                    return MY_ERR(_Err_SYS + errno);
                }
            }
            pCmdPart = pCmdPart->pLastPart;
        }

        pCmd = pCmd->pNextCmd;
    }

    return 0;
}
