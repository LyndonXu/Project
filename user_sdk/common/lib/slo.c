/****************************************************************************
 * Copyright(c), 2001-2060, ************************** 版权所有
 ****************************************************************************
 * 文件名称             : slo.c
 * 版本                 : 0.0.1
 * 作者                 : 许龙杰
 * 创建日期             : 2014年04月08日
 * 描述                 :
 ****************************************************************************/
#include <common.h>
#include "common_define.h"




/* get the address of the entity */
static inline uint32_t GetEntityAddrInline(StSLO *pSLO)
{
	return ((uint32_t)pSLO + pSLO->u32EntityOffset);
}

/* get the entity address from the index */
static inline uint32_t GetEntityElementAddrInline(StSLO *pSLO, uint16_t u16Index)
{
	return GetEntityAddrInline(pSLO) + (uint32_t)u16Index * pSLO->u32EntityWithIndexSize;
}

/* get the cursor of the entity from the index */
static inline StSLOCursor *GetEntityCursorInline(StSLO *pSLO, uint16_t u16Index)
{
	StSLOCursor *pCursor;
	uint32_t u32EntitySize = pSLO->u32EntityWithIndexSize;
	uint32_t u32EntityAddr = GetEntityAddrInline(pSLO) + (uint32_t)u16Index * u32EntitySize;
	/* get the entity size and the entity cursor*/
	u32EntitySize -= sizeof(StSLOCursor);
	pCursor = (StSLOCursor *)(u32EntityAddr + u32EntitySize);
	return pCursor;
}

/* get the cursor of the entity from the index */
#define GetEntityCursor(Cursor, pSLO, Index) \
uint32_t u32EntitySize = pSLO->u32EntityWithIndexSize;\
uint32_t u32EntityAddr = GetEntityAddrInline(pSLO) + (uint32_t)Index * u32EntitySize; \
/* get the entity size and the entity cursor*/\
u32EntitySize -=  sizeof(StSLOCursor); \
Cursor = (StSLOCursor *)(u32EntityAddr + u32EntitySize);

/* get the address of the addition data, if there is no addition data in the SLO, it will return NULL */
static inline void *GetAdditionDataAddr(StSLO *pSLO)
{
	if (pSLO->u32AdditionDataSize == 0)
	{
		return NULL;
	}
	else
	{
		return (void *)((uint32_t)pSLO + pSLO->u32AdditionDataOffset);
	}
}

/* flag the flag bit for this index */
static inline void FlagFlag(StSLO *pSLO, uint16_t u16Index)
{
	uint32_t *pFlag = (uint32_t *)((uint32_t)pSLO + pSLO->u32FlagDataOffset);
	pFlag[u16Index >> 5] |= (1 << (u16Index & 0x1F));
}

/* clear the flag bit for this index */
static inline void ClearFlag(StSLO *pSLO, uint16_t u16Index)
{
	uint32_t *pFlag = (uint32_t *)((uint32_t)pSLO + pSLO->u32FlagDataOffset);
	pFlag[u16Index >> 5] &= (~(1 << (u16Index & 0x1F)));
}

/* check whether the index is been flagged */
static inline bool IsFlag(StSLO *pSLO, uint16_t u16Index)
{
	uint32_t *pFlag = (uint32_t *)((uint32_t)pSLO + pSLO->u32FlagDataOffset);
	if (((pFlag[u16Index >> 5] >> (u16Index & 0x1F)) & 0x01) == 0x01)
	{
		return true;
	}
	else
	{
		return false;
	}
}




/*
 * 函数名      : SLOInit
 * 功能        : 初始化共享内存链表，并返回句柄，必须与SLODestroy成对使用，
 * 				 不一定要与SLOTombDestroy成对
 * 参数        : pName[in] (const char *类型): 共享内存的名字
 * 			   : u16Capacity[in] (uint16_t): 期望的容量
 * 			   : u32EntitySize[in] (uint32_t): 实体的大小
 * 			   : u32AdditionDataSize[in] (uint32_t): 附加数据的大小
 * 			   : pErr[out] (int32_t *): 在指针不为NULL的情况下返回错误码
 * 返回值      : int32_t 型数据, 0失败, 否则成功
 * 作者        : 许龙杰
 */
int32_t SLOInit(const char *pName, uint16_t u16Capacity, uint32_t u32EntitySize, uint32_t u32AdditionDataSize, int32_t *pErr)
{
	uint32_t u32MemSize = 0;
	StSLOCursor *pCursor;
	uint32_t i;
	StSLOHandle *pSLOHandle;
	StSLO *pSLO;
	int32_t s32LockHandle = -1, s32SHMHandle = -1;
	int32_t s32Err = 0;
	uint32_t u32FlagCnt = 0;
	uint32_t u32RealEntitySize;

	if ((pName == NULL) || (u16Capacity == 0) || ( u32EntitySize == 0))
	{
		s32Err = MY_ERR(_Err_InvalidParam);
		goto err1;
	}

	u32RealEntitySize = u32EntitySize;
	u32AdditionDataSize = ((u32AdditionDataSize + 3) / 4) * 4;
	u32EntitySize = ((u32EntitySize + 3) / 4) * 4;
	u32FlagCnt = ((u16Capacity + 31) / 32) * 4;

	u32MemSize = sizeof(StSLO) + u32FlagCnt + u32AdditionDataSize +
			(u32EntitySize + sizeof(StSLOCursor)) * u16Capacity;


	PRINT("before aligning, the memory size is %d\n", u32MemSize);
	/* integral multiple of the system's page size */
	{
		int32_t s32PageSize = getpagesize();
		u32MemSize = ((u32MemSize + (s32PageSize - 1)) / s32PageSize) * s32PageSize;
	}
	PRINT("memory size is %d\n", u32MemSize);

	s32LockHandle = LockOpen(pName);
	if (s32LockHandle < 0)
	{
		s32Err = s32LockHandle;
		goto err1;
	}
	s32SHMHandle = GetTheShmId(pName, u32MemSize);
	if (s32SHMHandle < 0)
	{
		s32Err = s32SHMHandle;
		goto err2;
	}

	pSLOHandle = calloc(1, sizeof(StSLOHandle));
	if (NULL == pSLOHandle)
	{
		s32Err = MY_ERR(_Err_Mem);
		goto err3;
	}
	pSLO = (StSLO *)shmat(s32SHMHandle, NULL, 0);
	if (pSLO == (StSLO *)(-1))
	{
		s32Err = MY_ERR(_Err_SYS + errno);
		goto err4;
	}
	pSLOHandle->pSLO = pSLO;
	pSLOHandle->s32LockHandle = s32LockHandle;
	pSLOHandle->s32SHMHandle = s32SHMHandle;

	s32Err = LockLock(s32LockHandle);
	if (s32Err != 0)
	{
		goto err4;
	}
	if ((pSLO->stMemCheck.s32FlagHead == 0x01234567) &&
			(pSLO->stMemCheck.s32FlagTail == 0x89ABCDEF) &&
			(pSLO->stMemCheck.s32CheckSum == CheckSum((int32_t *)pSLO, offsetof(StMemCheck, s32CheckSum) / sizeof(int32_t))))
	{
		PRINT("the shared memory has been initialized\n");
		goto ok;
	}
	PRINT("I will initialize the shared memory\n");
	memset(pSLO, 0, u32MemSize);
	pSLO->stMemCheck.s32FlagHead = 0x01234567;
	pSLO->stMemCheck.s32FlagTail = 0x89ABCDEF;

	for (i = 0; i < 8; i++)
	{
		pSLO->stMemCheck.s16RandArray[i] = rand();
	}
	pSLO->stMemCheck.s32CheckSum = CheckSum((int32_t *)pSLO, offsetof(StMemCheck, s32CheckSum) / sizeof(int32_t));
	pSLO->u32SLOTotalSize = u32MemSize;
	pSLO->u32RealEntitySize = u32RealEntitySize;
	pSLO->u32EntityWithIndexSize = u32EntitySize + sizeof(StSLOCursor);
	pSLO->u32FlagDataSize = u32FlagCnt;
	pSLO->u32FlagDataOffset = sizeof(StSLO);
	pSLO->u32AdditionDataSize = u32AdditionDataSize;
	pSLO->u32AdditionDataOffset = pSLO->u32FlagDataOffset + u32FlagCnt;
	pSLO->u32EntityOffset = pSLO->u32AdditionDataOffset + u32AdditionDataSize;
	pSLO->u16SLOTotalCapacity = u16Capacity;
	pSLO->u16SLOCurrentCapacity = 0;
	pSLO->u16HeadIdle = 0;
	pSLO->u16TailIdle = u16Capacity - 1;
	pSLO->u16Head = ~0;
	pSLO->u16Tail = ~0;

	PRINT("entity offset is 0x%08x\n", pSLO->u32EntityOffset);

	pCursor = (StSLOCursor *)(GetEntityAddrInline(pSLO) + u32EntitySize);
	for (i = 0; i < pSLO->u16TailIdle; i++)
	{
		pCursor->u16Prev = i - 1;
		pCursor->u16Next = i + 1;
		pCursor = (StSLOCursor *)((uint32_t)pCursor + pSLO->u32EntityWithIndexSize);
	}
	pCursor->u16Prev = i - 1;
	pCursor->u16Next = ~0;

ok:
	if (pErr != NULL)
	{
		*pErr = 0;
	}
	LockUnlock(s32LockHandle);
	return (int32_t)pSLOHandle;
err4:
	free(pSLOHandle);
err3:
	ReleaseAShmId(s32SHMHandle);
err2:
	LockClose(s32LockHandle);
err1:
	if (pErr != NULL)
	{
		*pErr = s32Err;
	}
	return 0;
}



/*
 * 函数名      : SLOInsertAnEntitySLO
 * 功能        : 向共享内存链表插入一个实体(pFunCompare为NULL时直接插入，
 * 				 pFunCompare不为NULL是返回值为0的情况下)
 * 参数        : pSLO[in] (StSLO *): 详见定义
 * 			   : pData[in] (void *): 实体数据
 * 			   : pFunCompare[in] (PFUN_SLO_Compare): 详见定义(pData将传递到pFunCompare的pRight参数)
 * 返回值      : int32_t 型数据, 正数表示实体的索引, 否则表示错误码
 * 作者        : 许龙杰
 */
static int32_t SLOInsertAnEntitySLO(StSLO *pSLO, void *pData, PFUN_SLO_Compare pFunCompare)
{
	uint16_t u16HeadIdle = 0;

	if (pSLO == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	if (pSLO->u16HeadIdle >= pSLO->u16SLOTotalCapacity)
	{
		return MY_ERR(_Err_SLO_NoSpace);	/* no space */
	}

	if (pFunCompare != NULL)
	{
		/* we should compare the data now */
		uint16_t u16End = pSLO->u16SLOTotalCapacity;
		uint16_t u16Head = pSLO->u16Head;
		while (u16Head < u16End)
		{
			uint32_t u32EntityAddr = GetEntityAddrInline(pSLO) +
							(uint32_t)u16Head * pSLO->u32EntityWithIndexSize;
			if (pFunCompare((void *)(u32EntityAddr), GetAdditionDataAddr(pSLO), pData) == 0)
			{
				return MY_ERR(_Err_SLO_Exist);
			}
			{
				StSLOCursor *pEntity = (StSLOCursor *)(u32EntityAddr + pSLO->u32EntityWithIndexSize - sizeof(StSLOCursor));
				u16Head = pEntity->u16Next;
			}
		}
	}

	u16HeadIdle = pSLO->u16HeadIdle;
	{
		StSLOCursor *pCursor = NULL;
		GetEntityCursor(pCursor, pSLO, u16HeadIdle);

		/* copy the data to the entity */
		memcpy((void *)u32EntityAddr, pData, pSLO->u32RealEntitySize);


		/* delete the entity from the idle list */
		pSLO->u16HeadIdle = pCursor->u16Next;
		if (pSLO->u16HeadIdle >= pSLO->u16SLOTotalCapacity)
		{
			/* the last space */
			pSLO->u16TailIdle = ~0;
		}
		else
		{
			StSLOCursor *pNext = NULL;
			GetEntityCursor(pNext, pSLO, pSLO->u16HeadIdle);
			pNext->u16Prev = ~0;
		}

		/* inserte the entity into the using list */
		if (pSLO->u16Tail >= pSLO->u16SLOTotalCapacity)
		{
			pSLO->u16Tail = u16HeadIdle;
		}
		pCursor->u16Prev = ~0;
		pCursor->u16Next = pSLO->u16Head;
		if (pSLO->u16Head < pSLO->u16SLOTotalCapacity)
		{
			StSLOCursor *pHead = NULL;
			GetEntityCursor(pHead, pSLO, pSLO->u16Head);
			pHead->u16Prev = u16HeadIdle;
		}

		pSLO->u16Head = u16HeadIdle;
	}

	FlagFlag(pSLO, u16HeadIdle);

	pSLO->u16SLOCurrentCapacity++;

	return u16HeadIdle;
}

/*
 * 函数名      : SLOInsertAnEntity
 * 功能        : 向共享内存链表插入一个实体(pFunCompare为NULL时直接插入，
 * 				 pFunCompare不为NULL是返回值为0的情况下)
 * 参数        : s32Handle[in] (int32_t): SLOInit的返回值
 * 			   : pData[in] (void *): 实体数据
 * 			   : pFunCompare[in] (PFUN_SLO_Compare): 详见定义(pData将传递到pFunCompare的pRight参数)
 * 返回值      : int32_t 型数据, 非负数数表示实体的索引, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t SLOInsertAnEntity(int32_t s32Handle, void *pData, PFUN_SLO_Compare pFunCompare)
{
	StSLOHandle *pHandle = (StSLOHandle *)s32Handle;
	int32_t s32Err;
	if (NULL == pHandle)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	s32Err = LockLock(pHandle->s32LockHandle);
	if (s32Err != 0)
	{
		return s32Err;
	}
	s32Err = SLOInsertAnEntitySLO(pHandle->pSLO, pData, pFunCompare);
	LockUnlock(pHandle->s32LockHandle);
	return s32Err;
}


/* delete an entity from the SLO by using the index directly */
static int32_t SLODeleteAnEntityUsingIndexInner(StSLO *pSLO, uint16_t u16Entity)
{
	StSLOCursor *pEntity;
	pEntity = GetEntityCursorInline(pSLO, u16Entity);

	/* re-link the previous cursor */
	if (u16Entity == pSLO->u16Head)
	{
		pSLO->u16Head = pEntity->u16Next;
	}
	else
	{
		StSLOCursor *pPrev = GetEntityCursorInline(pSLO, pEntity->u16Prev);
		pPrev->u16Next = pEntity->u16Next;
	}

	/* re-link the next cursor */
	if (u16Entity == pSLO->u16Tail)
	{
		pSLO->u16Tail = pEntity->u16Prev;
	}
	else
	{
		StSLOCursor *pNext = GetEntityCursorInline(pSLO, pEntity->u16Next);
		pNext->u16Prev = pEntity->u16Prev;
	}
	pEntity->u16Prev = ~0;
	pEntity->u16Next = ~0;
	/* now, I have deleted the entity from the using list */

	/* and, I will insert the entity into the idle list */
	if (pSLO->u16TailIdle >= pSLO->u16SLOTotalCapacity)
	{
		pSLO->u16TailIdle = u16Entity;
	}
	if (pSLO->u16HeadIdle < pSLO->u16SLOTotalCapacity)
	{
		StSLOCursor *pHeadIdle = GetEntityCursorInline(pSLO, pSLO->u16HeadIdle);
		pHeadIdle->u16Prev = u16Entity;
		pEntity->u16Next = pSLO->u16HeadIdle;
	}
	pSLO->u16HeadIdle = u16Entity;

	ClearFlag(pSLO, u16Entity);

	pSLO->u16SLOCurrentCapacity--;
	return 0;
}

/* delete an entity from the SLO by using the index safely(traversal the using list) */
static int32_t SLODeleteAnEntityUsingIndexTraversal(StSLO *pSLO, int32_t s32Index)
{
	uint16_t u16Entity = s32Index;

	if (pSLO == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	if (u16Entity >= pSLO->u16SLOTotalCapacity)
	{
		return MY_ERR(_Err_SLO_Index);	/* error index */
	}
	if (pSLO->u16Head >= pSLO->u16SLOTotalCapacity)
	{
		return MY_ERR(_Err_SLO_NoElement);	/* no element */
	}
	{
		/* we should compare the data now */
		uint16_t u16End = pSLO->u16SLOTotalCapacity;
		uint16_t u16EntityTmp = pSLO->u16Head;
		while (u16EntityTmp < u16End)
		{
			uint32_t u32EntityAddr = GetEntityAddrInline(pSLO) +
				(uint32_t)u16EntityTmp * pSLO->u32EntityWithIndexSize;
			if (u16EntityTmp == u16Entity)
			{
				return SLODeleteAnEntityUsingIndexInner(pSLO, u16Entity);
			}
			else
			{
				StSLOCursor *pEntity = (StSLOCursor *)(u32EntityAddr + pSLO->u32EntityWithIndexSize - sizeof(StSLOCursor));
				u16EntityTmp = pEntity->u16Next;
			}
		}
	}
	return MY_ERR(_Err_SLO_NotExist);
}

/* 废弃，使用扫描方式和实体的索引删除一个实体 */
int32_t SLODeleteAnEntityUsingIndex(int32_t s32Handle, int32_t s32Index)
{
	StSLOHandle *pHandle = (StSLOHandle *)s32Handle;
	int32_t s32Err;
	if (NULL == pHandle)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	s32Err = LockLock(pHandle->s32LockHandle);
	if (s32Err != 0)
	{
		return s32Err;
	}
	s32Err = SLODeleteAnEntityUsingIndexTraversal(pHandle->pSLO, s32Index);
	LockUnlock(pHandle->s32LockHandle);
	return s32Err;
}

/*
 * 函数名      : SLODeleteAnEntityUsingIndexDirectly
 * 功能        : 通过索引从共享内存链表删除一个实体
 * 参数        : s32Handle[in] (int32_t): SLOInit的返回值
 * 			   : s32Index[in] (int32_t): 实体的索引
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t SLODeleteAnEntityUsingIndexDirectly(int32_t s32Handle, int32_t s32Index)
{
	StSLOHandle *pHandle = (StSLOHandle *)s32Handle;
	int32_t s32Err;
	uint16_t u16Entity;
	if (NULL == pHandle)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	s32Err = LockLock(pHandle->s32LockHandle);
	if (s32Err != 0)
	{
		return s32Err;
	}
	u16Entity = s32Index;
	if (u16Entity >= pHandle->pSLO->u16SLOTotalCapacity)
	{
		s32Err = MY_ERR(_Err_SLO_Index);	/* error index */
		goto end;
	}
	if (pHandle->pSLO->u16Head >= pHandle->pSLO->u16SLOTotalCapacity)
	{
		s32Err = MY_ERR(_Err_SLO_NoElement);	/* no element */
		goto end;
	}

	if (!IsFlag(pHandle->pSLO, u16Entity))
	{
		s32Err = MY_ERR(_Err_SLO_IndexNotUsing);	/* error index */
		goto end;
	}

	s32Err = SLODeleteAnEntityUsingIndexInner(pHandle->pSLO, u16Entity);
end:
	LockUnlock(pHandle->s32LockHandle);
	return s32Err;

}


/* delete an entity from the SLO by using the callback function */
static int32_t SLODeleteAnEntityTraversal(StSLO *pSLO, void *pData, PFUN_SLO_Compare pFunCompare)
{
	if ((pSLO == NULL) || (pFunCompare == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}
	if (pSLO->u16Head >= pSLO->u16SLOTotalCapacity)
	{
		return MY_ERR(_Err_SLO_NoElement);	/* no element */
	}
	{
		/* we should compare the data now */
		uint16_t u16End = pSLO->u16SLOTotalCapacity;
		uint16_t u16Entity = pSLO->u16Head;
		while (u16Entity < u16End)
		{
			uint32_t u32EntityAddr = GetEntityAddrInline(pSLO) +
				(uint32_t)u16Entity * pSLO->u32EntityWithIndexSize;
			if (pFunCompare((void *)(u32EntityAddr), GetAdditionDataAddr(pSLO), pData) == 0)
			{
				return SLODeleteAnEntityUsingIndexInner(pSLO, u16Entity);
			}
			else
			{
				StSLOCursor *pEntity = (StSLOCursor *)(u32EntityAddr + pSLO->u32EntityWithIndexSize - sizeof(StSLOCursor));
				u16Entity = pEntity->u16Next;
			}
		}
	}
	return MY_ERR(_Err_SLO_NotExist);
}

/*
 * 函数名      : SLODeleteAnEntity
 * 功能        : 从共享内存链表删除一个实体（pFunCompare返回值为0的情况下）
 * 参数        : s32Handle[in] (int32_t): SLOInit的返回值
 * 			   : pData[in] (void *): 实体数据
 * 			   : pFunCompare[in] (PFUN_SLO_Compare): 详见定义(pData将传递到pFunCompare的pRight参数)
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t SLODeleteAnEntity(int32_t s32Handle, void *pData, PFUN_SLO_Compare pFunCompare)
{
	StSLOHandle *pHandle = (StSLOHandle *)s32Handle;
	int32_t s32Err;
	if (NULL == pHandle)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	s32Err = LockLock(pHandle->s32LockHandle);
	if (s32Err != 0)
	{
		return s32Err;
	}
	s32Err = SLODeleteAnEntityTraversal(pHandle->pSLO, pData, pFunCompare);
	LockUnlock(pHandle->s32LockHandle);
	return s32Err;
}


/*
 *  __________    ______    ______    _______
 * | leftprev |->| left |->| this |->| right |
 * |__________|<-|______|<-|______|<-|_______|
 *
 * ------->
 *  __________    ______    ______    _______
 * | leftprev |->| this |->| left |->| right |
 * |__________|<-|______|<-|______|<-|_______|
 */
/* update(transpose) an entity from the SLO by using the index directly */
static int32_t SLOUpdateAnEntityUsingIndexInner(StSLO *pSLO, uint16_t u16Entity)
{
	StSLOCursor *pEntity;
	StSLOCursor *pEntityLeft;
	/*uint16_t u16EntityLeft;*/


	if (u16Entity == pSLO->u16Head)
	{
		return 0;	/* it has been the topest entity */
	}
	pEntity = GetEntityCursorInline(pSLO, u16Entity);
	pEntityLeft = GetEntityCursorInline(pSLO, pEntity->u16Prev);

	/* the left entity index of this entity */
	/*u16EntityLeft = pEntity->u16Prev;*/

	/* link the right entity to the left entity */
	if (u16Entity == pSLO->u16Tail)
	{
		pEntityLeft->u16Next = ~0;
		pSLO->u16Tail = pEntity->u16Prev;
	}
	else
	{
		StSLOCursor *pEntityRight = GetEntityCursorInline(pSLO, pEntity->u16Next);
		pEntityLeft->u16Next = pEntity->u16Next;
		pEntityRight->u16Prev = pEntity->u16Prev;
	}

	/* link the left entity of this entity to the right side of this entity */
	pEntity->u16Next = pEntity->u16Prev;

	/* link the left entity of the left entity to this entity */
	if (pEntity->u16Prev == pSLO->u16Head)
	{
		pSLO->u16Head = u16Entity;
		pEntity->u16Prev = ~0;
	}
	else
	{
		StSLOCursor *pEntityLeftPrev = GetEntityCursorInline(pSLO, pEntityLeft->u16Prev);
		pEntityLeftPrev->u16Next = u16Entity;
		pEntity->u16Prev = pEntityLeft->u16Prev;
	}
	/* link the left entity of this entity to the right side of this entity */
	pEntityLeft->u16Prev = u16Entity;

	return 0;
}

/* update an entity of the SLO by using the callback function */
static int32_t SLOUpdateAnEntityTraversal(StSLO *pSLO, void *pData, PFUN_SLO_Compare pFunCompare)
{
	if ((pSLO == NULL) || (pFunCompare == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}
	if (pSLO->u16Head >= pSLO->u16SLOTotalCapacity)
	{
		return MY_ERR(_Err_SLO_NoElement);	/* no element */
	}
	{
		/* we should compare the data now */
		uint16_t u16End = pSLO->u16SLOTotalCapacity;
		uint16_t u16Entity = pSLO->u16Head;
		while (u16Entity < u16End)
		{
			uint32_t u32EntityAddr = GetEntityAddrInline(pSLO) +
				(uint32_t)u16Entity * pSLO->u32EntityWithIndexSize;
			if (pFunCompare((void *)(u32EntityAddr), GetAdditionDataAddr(pSLO), pData) == 0)
			{
				return SLOUpdateAnEntityUsingIndexInner(pSLO, u16Entity);
			}
			else
			{
				StSLOCursor *pEntity = (StSLOCursor *)(u32EntityAddr + pSLO->u32EntityWithIndexSize - sizeof(StSLOCursor));
				u16Entity = pEntity->u16Next;
			}
		}
	}
	return  MY_ERR(_Err_SLO_NotExist);

}

/*
 * 函数名      : SLOUpdateAnEntity
 * 功能        : 从共享内存链表更新一个实体（pFunCompare返回值为0的情况下）
 * 参数        : s32Handle[in] (int32_t): SLOInit的返回值
 * 			   : pData[in] (void *): 实体数据
 * 			   : pFunCompare[in] (PFUN_SLO_Compare): 详见定义(pData将传递到pFunCompare的pRight参数)
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t SLOUpdateAnEntity(int32_t s32Handle, void *pData, PFUN_SLO_Compare pFunCompare)
{
	StSLOHandle *pHandle = (StSLOHandle *)s32Handle;
	int32_t s32Err;
	if (NULL == pHandle)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	s32Err = LockLock(pHandle->s32LockHandle);
	if (s32Err != 0)
	{
		return s32Err;
	}
	s32Err = SLOUpdateAnEntityTraversal(pHandle->pSLO, pData, pFunCompare);
	LockUnlock(pHandle->s32LockHandle);
	return s32Err;
}

/* update an entity of the SLO by using the index safely */
static int32_t SLOUpdateAnEntityUsingIndexTraversal(StSLO *pSLO, int32_t s32Index)
{
	uint16_t u16Entity = s32Index;

	if (pSLO == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	if (u16Entity >= pSLO->u16SLOTotalCapacity)
	{
		return MY_ERR(_Err_SLO_Index);	/* error index */
	}
	if (pSLO->u16Head >= pSLO->u16SLOTotalCapacity)
	{
		return MY_ERR(_Err_SLO_NoElement);	/* no element */
	}
	{
		/* we should compare the data now */
		uint16_t u16End = pSLO->u16SLOTotalCapacity;
		uint16_t u16EntityTmp = pSLO->u16Head;
		while (u16EntityTmp < u16End)
		{
			uint32_t u32EntityAddr = GetEntityAddrInline(pSLO) +
				(uint32_t)u16EntityTmp * pSLO->u32EntityWithIndexSize;
			if (u16EntityTmp == u16Entity)
			{
				return SLOUpdateAnEntityUsingIndexInner(pSLO, u16Entity);
			}
			else
			{
				StSLOCursor *pEntity = (StSLOCursor *)(u32EntityAddr + pSLO->u32EntityWithIndexSize - sizeof(StSLOCursor));
				u16EntityTmp = pEntity->u16Next;
			}
		}
	}
	return MY_ERR(_Err_SLO_NotExist);
}

/* 废弃，使用扫描方式和实体的索引删除一个实体 */
int32_t SLOUpdateAnEntityUsingIndex(int32_t s32Handle, int32_t s32Index)
{
	StSLOHandle *pHandle = (StSLOHandle *)s32Handle;
	int32_t s32Err;
	if (NULL == pHandle)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	s32Err = LockLock(pHandle->s32LockHandle);
	if (s32Err != 0)
	{
		return s32Err;
	}
	s32Err = SLOUpdateAnEntityUsingIndexTraversal(pHandle->pSLO, s32Index);
	LockUnlock(pHandle->s32LockHandle);
	return s32Err;
}

/*
 * 函数名      : SLOUpdateAnEntityUsingIndexDirectly
 * 功能        : 通过索引从共享内存链表更新一个实体
 * 参数        : s32Handle[in] (int32_t): SLOInit的返回值
 * 			   : s32Index[in] (int32_t): 实体的索引
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t SLOUpdateAnEntityUsingIndexDirectly(int32_t s32Handle, int32_t s32Index)
{
	StSLOHandle *pHandle = (StSLOHandle *)s32Handle;
	int32_t s32Err;
	uint16_t u16Entity;
	if (NULL == pHandle)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	s32Err = LockLock(pHandle->s32LockHandle);
	if (s32Err != 0)
	{
		return s32Err;
	}
	u16Entity = s32Index;
	if (u16Entity >= pHandle->pSLO->u16SLOTotalCapacity)
	{
		s32Err = MY_ERR(_Err_SLO_Index);	/* error index */
		goto end;
	}
	if (pHandle->pSLO->u16Head >= pHandle->pSLO->u16SLOTotalCapacity)
	{
		s32Err = MY_ERR(_Err_SLO_NoElement);	/* no element */
		goto end;
	}

	if (!IsFlag(pHandle->pSLO, u16Entity))
	{
		s32Err = MY_ERR(_Err_SLO_IndexNotUsing);	/* error index */
		goto end;
	}

	s32Err = SLOUpdateAnEntityUsingIndexInner(pHandle->pSLO, u16Entity);
end:
	LockUnlock(pHandle->s32LockHandle);
	return s32Err;
}

/*
 * 函数名      : SLOTraversal
 * 功能        : 遍历链表
 * 参数        : s32Handle[in] (int32_t): SLOInit的返回值
 * 			   : pFunCallback[in] (PFUN_SLO_Callback): 实体的索引，该函数返回非0的情况下
 * 			     程序会删除该实体(pData将传递到pFunCompare的pRight参数)
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t SLOTraversal(int32_t s32Handle, void *pData, PFUN_SLO_Callback pFunCallback)
{
	StSLOHandle *pHandle = (StSLOHandle *)s32Handle;
	int32_t s32Err = 0;
	if (NULL == pHandle)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	s32Err = LockLock(pHandle->s32LockHandle);
	if (s32Err != 0)
	{
		return s32Err;
	}
	{
		/* we should compare the data now */
		uint16_t u16End = pHandle->pSLO->u16SLOTotalCapacity;
		uint16_t u16EntityTmp = pHandle->pSLO->u16Head;
		while (u16EntityTmp < u16End)
		{
			uint32_t u32EntityAddr = GetEntityAddrInline(pHandle->pSLO) +
				(uint32_t)u16EntityTmp * pHandle->pSLO->u32EntityWithIndexSize;
#if 0
//#ifdef _DEBUG
			{
				StProcessInfo *pInfo = (StProcessInfo *)u32EntityAddr;
				PRINT("The ID of the process is %d\n", pInfo->u32Pid);
			}
#endif
			if (pFunCallback != NULL)
			{
				int32_t s32Return  = pFunCallback((void *)u32EntityAddr, GetAdditionDataAddr(pHandle->pSLO), pData);
				if (s32Return != 0)
				{
					StSLOCursor *pEntity = (StSLOCursor *)(u32EntityAddr +
							pHandle->pSLO->u32EntityWithIndexSize - sizeof(StSLOCursor));
					uint16_t u16EntityNext = pEntity->u16Next;
					SLODeleteAnEntityUsingIndexInner(pHandle->pSLO, u16EntityTmp);
					u16EntityTmp = u16EntityNext;
					continue;
				}
			}
			{
				StSLOCursor *pEntity = (StSLOCursor *)(u32EntityAddr +
						pHandle->pSLO->u32EntityWithIndexSize - sizeof(StSLOCursor));
				u16EntityTmp = pEntity->u16Next;
			}
		}
	}
	LockUnlock(pHandle->s32LockHandle);
	return 0;
}


/*
 * 函数名      : SLOOperateAdditionData
 * 功能        : 操作附加数据
 * 参数        : s32Handle[in] (int32_t): SLOInit的返回值
 * 			   : pContext [in] (void *): 上写文指针
 * 			   : pFunOperate[in] (pFUN_SLO_OperateAdditionData): 具体操作回调函数(pContext将传递到pFunOperate的pContext参数)
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t SLOOperateAdditionData(int32_t s32Handle, void *pContext, pFUN_SLO_OperateAdditionData pFunOperate)
{
	StSLOHandle *pHandle = (StSLOHandle *)s32Handle;
	int32_t s32Err = 0;
	if (NULL == pHandle)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	s32Err = LockLock(pHandle->s32LockHandle);
	if (s32Err != 0)
	{
		return s32Err;
	}
	if (pFunOperate != NULL)
	{
		s32Err = pFunOperate(GetAdditionDataAddr(pHandle->pSLO), pContext);
	}

	LockUnlock(pHandle->s32LockHandle);
	return s32Err;
}

/*
 * 函数名      : SLOSetEntityUsingIndexDirectly
 * 功能        : 重新设置索引s32Index的实体的值
 * 参数        : s32Handle[in] (int32_t): SLOInit的返回值
 * 			   : s32Index[in] (int32_t): 实体的索引
 * 			   : pData [in] (void *): 要设置的数据指针
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t SLOSetEntityUsingIndexDirectly(int32_t s32Handle, int32_t s32Index, void *pData)
{
	StSLOHandle *pHandle = (StSLOHandle *)s32Handle;
	int32_t s32Err = 0;
	uint16_t u16Entity;
	uint32_t u32EntityAddr;
	if (NULL == pHandle)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	s32Err = LockLock(pHandle->s32LockHandle);
	if (s32Err != 0)
	{
		return s32Err;
	}

	u16Entity = s32Index;
	if (u16Entity >= pHandle->pSLO->u16SLOTotalCapacity)
	{
		s32Err = MY_ERR(_Err_SLO_Index);	/* error index */
		goto end;
	}

	if (!IsFlag(pHandle->pSLO, u16Entity))
	{
		s32Err = MY_ERR(_Err_SLO_IndexNotUsing);	/* error index */
		goto end;
	}
	u32EntityAddr = GetEntityElementAddrInline(pHandle->pSLO, u16Entity);
	memcpy((void *)u32EntityAddr, pData, pHandle->pSLO->u32RealEntitySize);

end:
	LockUnlock(pHandle->s32LockHandle);
	return s32Err;
}

/*
 * 函数名      : SLOGetEntityUsingIndexDirectly
 * 功能        : 获取索引s32Index的实体的值
 * 参数        : s32Handle[in] (int32_t): SLOInit的返回值
 * 			   : s32Index[in] (int32_t): 实体的索引
 * 			   : pData [in/out] (void *): 要将数据保存的位值指针
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t SLOGetEntityUsingIndexDirectly(int32_t s32Handle, int32_t s32Index, void *pData)
{
	StSLOHandle *pHandle = (StSLOHandle *)s32Handle;
	int32_t s32Err = 0;
	uint16_t u16Entity;
	uint32_t u32EntityAddr;
	if (NULL == pHandle)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	s32Err = LockLock(pHandle->s32LockHandle);
	if (s32Err != 0)
	{
		return s32Err;
	}

	u16Entity = s32Index;
	if (u16Entity >= pHandle->pSLO->u16SLOTotalCapacity)
	{
		s32Err = MY_ERR(_Err_SLO_Index);			/* error index */
		goto end;
	}

	if (!IsFlag(pHandle->pSLO, u16Entity))
	{
		s32Err = MY_ERR(_Err_SLO_IndexNotUsing);	/* error index */
		goto end;
	}
	u32EntityAddr = GetEntityElementAddrInline(pHandle->pSLO, u16Entity);
	memcpy(pData, (void *)u32EntityAddr, pHandle->pSLO->u32RealEntitySize);

end:
	LockUnlock(pHandle->s32LockHandle);
	return s32Err;
}

/*
 * 函数名      : SLOGetEntityUsingIndexDirectly
 * 功能        : 通过索引s32Index的操作实体的值
 * 参数        : s32Handle[in] (int32_t): SLOInit的返回值
 * 			   : s32Index[in] (int32_t): 实体的索引
 * 			   : pContext [in] (void *): 上写文指针
 * 			   : pFunOperate[in] (pFUN_SLO_OperateEntityElementData):
 * 			     具体操作回调函数(pContext将传递到pFunOperate的pContext参数)
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t SLOOperateEntityUsingIndexDirectly(int32_t s32Handle, int32_t s32Index,
		void *pContext, pFUN_SLO_OperateEntityElementData pFunOperate)
{
	StSLOHandle *pHandle = (StSLOHandle *)s32Handle;
	int32_t s32Err = 0;
	uint16_t u16Entity;
	uint32_t u32EntityAddr;
	if (NULL == pHandle)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	s32Err = LockLock(pHandle->s32LockHandle);
	if (s32Err != 0)
	{
		return s32Err;
	}

	u16Entity = s32Index;
	if (u16Entity >= pHandle->pSLO->u16SLOTotalCapacity)
	{
		s32Err = MY_ERR(_Err_SLO_Index);			/* error index */
		goto end;
	}

	if (!IsFlag(pHandle->pSLO, u16Entity))
	{
		s32Err = MY_ERR(_Err_SLO_IndexNotUsing);	/* error index */
		goto end;
	}
	u32EntityAddr = GetEntityElementAddrInline(pHandle->pSLO, u16Entity);
	if (pFunOperate != NULL)
	{
		s32Err = pFunOperate((void *)u32EntityAddr, pContext);
	}

end:
	LockUnlock(pHandle->s32LockHandle);
	return s32Err;
}

/*
 * 函数名      : SLODestroy
 * 功能        : 释放该进程空间中的句柄占用的资源
 * 参数        : s32Handle[in] (int32_t): SLOInit的返回值
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void SLODestroy(int32_t s32Handle)
{
	StSLOHandle *pHandle = (StSLOHandle *)s32Handle;

	if (NULL == pHandle)
	{
		return;
	}

	shmdt(pHandle->pSLO);
	free(pHandle);
}

/*
 * 函数名      : SLODestroy
 * 功能        : 释放该进程空间中的句柄占用的资源，并从该系统中销毁资源
 * 参数        : s32Handle[in] (int32_t): SLOInit的返回值
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void SLOTombDestroy(int32_t s32Handle)
{
	StSLOHandle *pHandle = (StSLOHandle *)s32Handle;

	if (NULL == pHandle)
	{
		return;
	}
	shmdt(pHandle->pSLO);
	ReleaseAShmId(pHandle->s32SHMHandle);
	LockClose(pHandle->s32LockHandle);
	free(pHandle);
}
