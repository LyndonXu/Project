/****************************************************************************
 * Copyright(c), 2001-2060, ******************************* 版权所有
 ****************************************************************************
 * 文件名称             : upgrade.h
 * 版本                 : 0.0.1
 * 作者                 : 许龙杰
 * 创建日期             : 2014年4月18日
 * 描述                 :
 ****************************************************************************/
#ifndef COMMON_H_
#define COMMON_H_
#include <stdbool.h>
#include <sys/types.h>
#include <dirent.h>
#include <netinet/in.h>

#pragma pack(4)

#ifdef __cplusplus
extern "C" {
#endif

/*错误码定义
 * -------------------------------------------------------------------
 * | 31~28  | 27~26 |    25   |  24  |     23 ~ 16    |    15 ~ 0    |
 * |--------|-------|---------|------|----------------|--------------|
 * | 标识域  |  保留  | 0：保留  | 保留  |        保留     | 各模块错误码   |
 * |  0x0E  |       | 1：用户  |      | 暂用于模块分配    |              |
 * -------------------------------------------------------------------
 */
#define FULL_ERROR_CODE(Identification, UserOrReserved, Module, Code) \
        ((((Identification) & 0x0F) << 28) | \
        (((UserOrReserved) & 0x01) << 25) | \
        (((Module) & 0xFF) << 16) | \
        ((Code) & 0xFFFF))

#define ERROR_CODE(Module, Code) 			FULL_ERROR_CODE(0x0E, 0, (Module), (Code))

#define MY_ERR(code)       					ERROR_CODE(0x15, (code))
#define COMMON_ERR(code)       				MY_ERR(code)


enum
{
	_Err_Identification = 0x01,
	_Err_Handle,
	_Err_InvalidParam,
	_Err_CmdLen,
	_Err_CmdType,
	_Err_NULLPtr,
	_Err_Time,
	_Err_Mem,
	_Err_NoneCmdData,
	_Err_TimeOut,

	_Err_SLO_NoSpace = 0x100,
	_Err_SLO_NoElement,
	_Err_SLO_Exist,
	_Err_SLO_NotExist,
	_Err_SLO_Index,
	_Err_SLO_IndexNotUsing,

	_Err_LOG_NoText = 0x110,
	_Err_LOG_Time_FMT,
	_Err_NotADir,
	_Err_JSON,
	_Err_Authentication,
	_Err_IDPS,

	_Err_Cloud_IsNotOnline = 0x120,
	_Err_Cloud_Body,
	_Err_Cloud_JSON,
	_Err_Cloud_CMD,
	_Err_Cloud_Data,
	_Err_Cloud_Authentication,
	_Err_Cloud_Save_Domain,
	_Err_Cloud_Get_Domain,

	_Err_Cloud_Result = 0x130,

	_Err_Unkown_Host = 0x140,

	_Err_File_CRC = 0x150,
	_Err_No_New_Version,

	_Err_Hash_NoSpace = 0x160,
	_Err_Hash_NoElement,
	_Err_Hash_Exist,
	_Err_Hash_NotExist,
	_Err_Hash_Index,
	_Err_Hash_IndexNotUsing,
	_Err_Hash_ToLarge,
	_Err_Hash_NotAProcess,

	_Err_WPA_Open = 0x170,
	_Err_WPA_TimeOut,
	_Err_WPA_Send,

	_Err_HTTP = 0x1000,

	_Err_SSL = 0x2000,

	_Err_SYS = 0xFF00,

	_Err_Common = 0xFFFF,
};

#if ((defined _DEBUG) && (!(defined NOT_USING_PRINT)))
#define PRINT(x, ...) printf("[%s:%d]: " x, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define PRINT(x, ...)
#endif


#if HAS_CROSS
#if defined _DEBUG
#define WORK_DIR							"/tmp/workdir/"
#else
#define WORK_DIR							"/var/workdir/"
#endif
#else
#define WORK_DIR							"/tmp/workdir/"
#endif


#define UDP_SERVER_PORT				(('Y'<< 8) | 'A')
#define TCP_UPDATE_SERVER_PORT		UDP_SERVER_PORT + 1
#define TCP_CMD_SERVER_PORT			UDP_SERVER_PORT + 2


/*
 * 函数名      : MakeASpecialKey
 * 功能        : 创建一个IPC key
 * 参数        : pName [in] (char * 类型): 名字
 * 返回值      : 正确返回key_t, 否则返回-1
 * 作者        : 许龙杰
 */
key_t MakeASpecialKey(const char *pName);

/*
 * 函数名      : LockOpen
 * 功能        : 创建一个新的锁 与 LockClose成对使用
 * 参数        : pName[in] (char * 类型): 记录锁的名字(不包含目录), 为NULL时使用默认名字
 * 返回值      : (int32_t) 成功返回非负数, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t LockOpen(const char *pName);

/*
 * 函数名      : LockClose
 * 功能        : 销毁一个锁, 并释放相关的资源 与 LockOpen成对使用
 * 参数        : s32Handle[in] (JA_SI32类型): LockOpen返回的值
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void LockClose(int32_t s32Handle);

/*
 * 函数名      : LockLock
 * 功能        : 加锁 与 LockUnlock成对使用
 * 参数        : s32Handle[in] (JA_SI32类型): LockOpen返回的值
 * 返回值      : 成功返回0, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t LockLock(int32_t s32Handle);

/*
 * 函数名      : LockUnlock
 * 功能        : 解锁 与 LockLock 成对使用
 * 参数        : s32Handle[in] (JA_SI32类型): LockOpen返回的值
 * 返回值      : 成功返回0, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t LockUnlock(int32_t s32Handle);

/*
 * 函数名      : GetTheShmId
 * 功能        : 得到一个共享内存ID 与 ReleaseAShmId 成对使用
 * 参数        : pName [in] (char * 类型): 名字
 *             : u32Size [in] (JA_UI32类型): 共享内存的大小
 * 返回值      : 正确返回非负ID号, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t GetTheShmId(const char * pName, uint32_t u32Size);

/*
 * 函数名      : ReleaseAShmId
 * 功能        : 释放一个共享内存ID 与 GetTheShmId成对使用, 所有进程只需要一个进程释放
 * 参数        : s32Id [in] (JA_SI32类型): ID号
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void ReleaseAShmId(int32_t s32Id);

/*
 * 函数名      : GetTheMsgId
 * 功能        : 得到一个MSG IPCID
 * 参数        : pName [in] (char * 类型): 名字
 *             : u32Size [in] (JA_UI32类型): 共享内存的大小
 * 返回值      : 正确返回非负ID号, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t GetTheMsgId(const char * pName);

/*
 * 函数名      : ReleaseAMsgId
 * 功能        : 从系统中释放一个MSG ID 所有进程只需要一个进程释放
 * 参数        : s32Id [in] (JA_SI32类型): ID号
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void ReleaseAMsgId(int32_t s32Id);

/*
 * 比较pLeft和pRight(通过上下文指针pData传递)数据，操作附加数据(pAdditionData)
 * 返回 0 表示两块数据关键子相同，负数 表示pLeft > pRight关键字，否则返回 正数
 */
typedef int32_t (*PFUN_SLO_Compare)(void *pLeft, void *pAdditionData, void *pRight);

/*
 * 遍历链表，操作附加数据(pAdditionData)
 * 返回值以具体调用函数而定
 */
typedef PFUN_SLO_Compare PFUN_SLO_Callback;


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
int32_t SLOInit(const char *pName, uint16_t u16Capacity, uint32_t u32EntitySize,
		uint32_t u32AdditionDataSize, int32_t *pErr);


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
int32_t SLOInsertAnEntity(int32_t s32Handle, void *pData, PFUN_SLO_Compare pFunCompare);

/*
 * 函数名      : SLODeleteAnEntity
 * 功能        : 从共享内存链表删除一个实体（pFunCompare返回值为0的情况下）
 * 参数        : s32Handle[in] (int32_t): SLOInit的返回值
 * 			   : pData[in] (void *): 实体数据
 * 			   : pFunCompare[in] (PFUN_SLO_Compare): 详见定义(pData将传递到pFunCompare的pRight参数)
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t SLODeleteAnEntity(int32_t s32Handle, void *pData, PFUN_SLO_Compare pFunCompare);

/* 废弃，使用扫描方式和实体的索引删除一个实体 */
int32_t SLODeleteAnEntityUsingIndex(int32_t s32Handle, int32_t s32Index);


/*
 * 函数名      : SLODeleteAnEntityUsingIndexDirectly
 * 功能        : 通过索引从共享内存链表删除一个实体
 * 参数        : s32Handle[in] (int32_t): SLOInit的返回值
 * 			   : s32Index[in] (int32_t): 实体的索引
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t SLODeleteAnEntityUsingIndexDirectly(int32_t s32Handle, int32_t s32Index);

/*
 * 函数名      : SLOUpdateAnEntity
 * 功能        : 从共享内存链表更新一个实体（pFunCompare返回值为0的情况下）
 * 参数        : s32Handle[in] (int32_t): SLOInit的返回值
 * 			   : pData[in] (void *): 实体数据
 * 			   : pFunCompare[in] (PFUN_SLO_Compare): 详见定义(pData将传递到pFunCompare的pRight参数)
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t SLOUpdateAnEntity(int32_t s32Handle, void *pData, PFUN_SLO_Compare pFunCompare);

/* 废弃，使用扫描方式和实体的索引删除一个实体 */
int32_t SLOUpdateAnEntityUsingIndex(int32_t s32Handle, int32_t s32Index);


/*
 * 函数名      : SLOUpdateAnEntityUsingIndexDirectly
 * 功能        : 通过索引从共享内存链表更新一个实体
 * 参数        : s32Handle[in] (int32_t): SLOInit的返回值
 * 			   : s32Index[in] (int32_t): 实体的索引
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t SLOUpdateAnEntityUsingIndexDirectly(int32_t s32Handle, int32_t s32Index);

/*
 * 函数名      : SLOTraversal
 * 功能        : 遍历链表
 * 参数        : s32Handle[in] (int32_t): SLOInit的返回值
 * 			   : pFunCallback[in] (PFUN_SLO_Callback): 实体的索引，该函数返回非0的情况下
 * 			     程序会删除该实体(pData将传递到pFunCompare的pRight参数)
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t SLOTraversal(int32_t s32Handle, void *pData, PFUN_SLO_Callback pFunCallback);


/*
 * 功能        : 操作附加数据
 * 参数        : pData[in/out] (int32_t): 要操作数据的指针
 * 			   : pContext [in/out] (void *): 上写文指针
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
typedef int32_t (*pFUN_SLO_OperateData)(void *pData, void *pContext);

typedef pFUN_SLO_OperateData pFUN_SLO_OperateAdditionData;
typedef pFUN_SLO_OperateData pFUN_SLO_OperateEntityElementData;

/*
 * 函数名      : SLOOperateAdditionData
 * 功能        : 操作附加数据
 * 参数        : s32Handle[in] (int32_t): SLOInit的返回值
 * 			   : pContext [in] (void *): 上写文指针
 * 			   : pFunOperate[in] (pFUN_SLO_OperateAdditionData): 具体操作回调函数(pContext将传递到pFunOperate的pContext参数)
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t SLOOperateAdditionData(int32_t s32Handle, void *pContext, pFUN_SLO_OperateAdditionData pFunOperate);

/*
 * 函数名      : SLOSetEntityUsingIndexDirectly
 * 功能        : 重新设置索引s32Index的实体的值
 * 参数        : s32Handle[in] (int32_t): SLOInit的返回值
 * 			   : s32Index[in] (int32_t): 实体的索引
 * 			   : pData [in] (void *): 要设置的数据指针
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t SLOSetEntityUsingIndexDirectly(int32_t s32Handle, int32_t s32Index, void *pData);

/*
 * 函数名      : SLOGetEntityUsingIndexDirectly
 * 功能        : 获取索引s32Index的实体的值
 * 参数        : s32Handle[in] (int32_t): SLOInit的返回值
 * 			   : s32Index[in] (int32_t): 实体的索引
 * 			   : pData [in/out] (void *): 要将数据保存的位值指针
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t SLOGetEntityUsingIndexDirectly(int32_t s32Handle, int32_t s32Index, void *pData);

/*
 * 函数名      : SLOGetEntityUsingIndexDirectly
 * 功能        : 通过索引s32Index的操作实体的值
 * 参数        : s32Handle[in] (int32_t): SLOInit的返回值
 * 			   : s32Index[in] (int32_t): 实体的索引
 * 			   : pContext [in] (void *): 上下文指针
 * 			   : pFunOperate[in] (pFUN_SLO_OperateEntityElementData):
 * 			     具体操作回调函数(pContext将传递到pFunOperate的pContext参数)
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t SLOOperateEntityUsingIndexDirectly(int32_t s32Handle, int32_t s32Index,
		void *pContext, pFUN_SLO_OperateEntityElementData pFunOperate);

/*
 * 函数名      : SLODestroy
 * 功能        : 释放该进程空间中的句柄占用的资源
 * 参数        : s32Handle[in] (int32_t): SLOInit的返回值
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void SLODestroy(int32_t s32Handle);

/*
 * 函数名      : SLODestroy
 * 功能        : 释放该进程空间中的句柄占用的资源，并从该系统中销毁资源
 * 参数        : s32Handle[in] (int32_t): SLOInit的返回值
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void SLOTombDestroy(int32_t s32Handle);



enum
{
	_MCS_Cmd_UartDaemon = 0x00007000,
	_MCS_Cmd_UpdateDaemon = 0x00008000,
	_MCS_Cmd_Cmd_Com = 0x00009000,


	_MCS_Cmd_Echo = 0x08000000,
};

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
void LittleAndBigEndianTransfer(char *pDest, const char *pSrc, uint32_t u32Size);

/*
 * 函数指针
 * 参数        : u32CmdNum[in] (uint32_t类型): 命令号
 *             : u32CmdCnt[in] (uint32_t类型): 此命令号命令的数量
 *             : u32CmdSize[in] (uint32_t类型): 每一个命令的大小
 *             : pCmdData[in] (const char *类型): 命令的数据
 *             : pContext[in] (void *类型): 用户上下文
 *
 */

typedef int32_t (*PFUN_MCS_Resolve_CallBack)
        (uint32_t u32CmdNum, uint32_t u32CmdCnt, uint32_t u32CmdSize,
        const char *pCmdData,
        void *pContext);


typedef struct _tagStSendV
{
    uint32_t u32Size;			/* 数据大小 */
    void *pData;      			/* 数据指针 */
}StSendV;
typedef StSendV StInOut;
/*
 * 函数名      : MCSOpen
 * 功能        : 得到一个command stream 句柄
 *             必须与MCSClose成对出现
 * 参数        : pErr[out] (int32_t * 类型): 不为NULL时, 错误码, 0正确, 否则返回负数
 * 返回值      : int32_t 型数据, 0失败, 否则成功
 * 作者        : 许龙杰
 */
int32_t MCSOpen(int32_t *pErr);

/*
 * 函数名			: MCSClose
 * 功能			: 销毁MCSOpen返回的句柄中相关资源
 *   			  必须与MCSOpen成对出现
 * 参数			: s32MCSHandle[in] (int32_t类型): MCSOpen返回的句柄
 * 				: boIsReleaseStream [in] (bool类型): 是否释放构建好的流，如果不释放，用户必须使用MCSFree释放该流
 * 返回值			: 无
 * 作者			: 许龙杰
 */
void MCSCloseNoReleaseStream(int32_t s32MCSHandle, bool boIsReleaseStream);

/*
 * 函数名      : MCSClose
 * 功能        : 销毁MCSOpen返回的句柄中相关资源
 *             必须与MCSOpen成对出现
 * 参数        : s32YSCHandle[in] (int32_t类型): MCSOpen返回的句柄
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void MCSClose(int32_t s32YSCHandle);

/*
 * 函数名      : MCSInsertACmd
 * 功能        : 在句柄中插入一条命令
 * 参数        : s32YSCHandle[in] (int32_t类型): MCSOpen返回的句柄
 *             : u32CmdNum[in] (uint32_t类型): 命令号
 *             : u32CmdCnt[in] (uint32_t类型): 命令对应的命令数量
 *             : u32CmdSize[in] (uint32_t类型): 每一个命令的大小
 *             : pCmdData[in] (void *类型): 命令数据
 *             : boIsRepeatCmd[in] (bool 类型): 是否连接到以前命令
 * 返回值      : int32_t 型数据, 0成功, 否则失败
 * 作者        : 许龙杰
 */
int32_t MCSInsertACmd(int32_t s32YSCHandle,
        uint32_t u32CmdNum, uint32_t u32CmdCnt, uint32_t u32CmdSize,
        const void *pCmdData,
        bool boIsRepeatCmd);

/*
 * 函数名      : MCSInsertAMCS
 * 功能        : 在句柄中插入一条MCS流
 * 参数        : s32MCSHandle[in] (int32_t类型): MCSOpen返回的句柄
 *             : pMCS[in] (const char * 类型): 指向命令流
 *             : u32MCSSize[in] (uint32_t类型): 不为 0 的时候指定命令流的大小
 * 返回值      : int32_t 型数据, 0成功, 否则失败
 * 作者        : 许龙杰
 */
int32_t MCSInsertAMCS(int32_t s32MCSHandle, const char *pMCS, uint32_t u32MCSSize);
/*
 * 函数名      : MCSGetStreamBuf
 * 功能        : 得到句柄中命令流
 * 参数        : s32MCSHandle[in] (int32_t类型): MCSOpen返回的句柄
 *             : pSize[out] (uint32_t类型): 不为 NULL 的时候保存命令流的大小
 *             : pErr[out] (int32_t * 类型): 不为NULL时, 错误码, 0正确, 否则返回负数
 * 返回值      : int32_t 型数据, 0成功, 否则失败
 * 作者        : 许龙杰
 */
const char *MCSGetStreamBuf(int32_t s32MCSHandle, uint32_t *pSize, int32_t *pErr);


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
	uint32_t u32CmdCnt, uint32_t u32CmdSize, uint32_t *pCmdLength);
/*
 * 函数名      : MCSMalloc
 * 功能        : 得到一些内存
 * 参数        : u32Size[in] (uint32_t类型): 要申请内存的大小
 * 返回值      : void * 型数据, NULL失败，否则成功
 * 作者        : 许龙杰
 */
void *MCSMalloc(uint32_t u32Size);
/*
 * 函数名      : MCSFree
 * 功能        : 释放MCSMalloc得到的内存
 * 参数        : pBuf[in] (void *类型): MCSMalloc的返回值
 * 返回值      : void * 型数据, NULL失败，否则成功
 * 作者        : 许龙杰
 */
void MCSFree(void *pBuf);
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
        bool boIsRepeatCmd);
/*
 * 函数名      : MCSGetCmdLength
 * 功能        : 从MCS流中得到负载的命令大小
 * 参数        : pMCS[in] (const char * 类型): 指向命令流
 *             : pMCSCmdSize[out] (uint32_t *类型): 成功在*pMCSCmdSize中保存大小
 * 返回值      : int32_t 型数据, 0成功, 否则失败
 * 作者        : 许龙杰
 */
int32_t MCSGetCmdLength(const char *pMCS, uint32_t *pMCSCmdSize);
/*
 * 函数名      : MCSGetCmdCnt
 * 功能        : 从MCS流中得到负载的命令数量
 * 参数        : pMCS[in] (const char * 类型): 指向命令流
 *             : pMCSCmdCnt[out] (uint32_t *类型): 成功在*pMCSCmdCnt中保存大小
 * 返回值      : int32_t 型数据, 0成功, 否则失败
 * 作者        : 许龙杰
 */
int32_t MCSGetCmdCnt(const char *pMCS, uint32_t *pMCSCmdCnt);
/*
 * 函数名      : MCSResolve
 * 功能        : 解析命令流
 * 参数        : pMCS[in] (const char * 类型): 指向命令流
 *             : u32MCSSize[in] (uint32_t类型): 不为 0 的时候指定命令流的大小
 *             : pFunCallBack[in] (PFUN_MCS_Resolve_CallBack类型): 命令解析回调函数
 *             : pContext[in] (void *类型): 用户上下文
 * 返回值      : int32_t 型数据, 0成功, 否则失败
 * 作者        : 许龙杰
 */
int32_t MCSResolve(const char *pMCS, uint32_t u32MCSSize, PFUN_MCS_Resolve_CallBack pFunCallBack, void *pContext);

/*
 * 函数名		: MCSSyncReceiveWithLimit
 * 功能			: 以MCS头作为同步头从SOCKET中接收数据, 与 MCSSyncFree成对使用, 负载长度不超u32MaxLength
 * 参数			: s32Socket[in] (int32_t类型): 要接收的SOCKET
 *				: boWantSyncHead [in] (bool类型): 是否希望在数据前端增加同步头
 *				：u32MaxLength [in]  (uint32_t): 最大负载长度
 *				: u32TimeOut[in] (uint32_t类型): 超时时间(ms)
 *				: pSize[out] (uint32_t * 类型): 保存数据的长度
 *				: pErr[out] (int32_t * 类型): 不为NULL时, *pErr中保存错误码
 * 返回值		: int32_t 型数据, 0成功, 否则失败
 * 作者			: 许龙杰
 */
void *MCSSyncReceiveWithLimit(int32_t s32Socket, bool boWantSyncHead,
		uint32_t u32MaxLength, uint32_t u32TimeOut, uint32_t *pSize, int32_t *pErr);

typedef int32_t (*PFUN_MCSRecvCMDCB)(void *pData, uint32_t u32Length, void *pContext);

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
		PFUN_MCSRecvCMDCB pFun, void *pContext);
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
void *MCSSyncReceive(int32_t s32Socket, bool boWantSyncHead, uint32_t u32TimeOut, uint32_t *pSize, int32_t *pErr);

/*
 * 函数名      : MCSSyncFree
 * 功能        : 释放数据, 与 MCSSyncReceive成对使用
 * 参数        : 无
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void MCSSyncFree(void *pData);

/*
 * 函数名      : MCSSyncSend
 * 功能        : 以MCS头作为同步头向SOCKET中发送数据
 * 参数        : s32Socket[in] (int32_t类型): 要放送到的SOCKET
 *             : u32TimeOut[in] (uint32_t类型): 超时(ms)
 *             : u32CommandNum[in] (uint32_t 类型): 数据对应的命令号(根据情况可以添0)
 *             : u32Size[in] (uint32_t 类型): 数据的长度
 *             : pData[in] (const void * 类型): 数据
 * 返回值      : int32_t 型数据, 0成功, 否则失败
 * 作者        : 许龙杰
 */
int32_t MCSSyncSend(int32_t s32Socket,  uint32_t u32TimeOut, uint32_t u32CommandNum,
		uint32_t u32Size, const void *pData);

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
		uint32_t u32Cnt, uint32_t u32ElementSize, const void *pData);

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
		uint32_t u32CommandNum, const char *pFileName);
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
int32_t MCSSyncSendData(int32_t s32Socket,  uint32_t u32TimeOut, uint32_t u32Size, const void *pData);

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
int32_t MCSSyncSendV(int32_t s32Socket, uint32_t u32TimeOut, uint32_t u32CommandNum, const StSendV *pSendV, uint32_t u32SendVCnt);
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
int32_t MCSSyncSendAMCS(int32_t s32Socket, uint32_t u32TimeOut, int32_t s32MCSHandle, uint32_t u32Size);


/*
 * 函数名      : AlreadyRunningUsingLockFile
 * 功能        : 使用文件所判断进程是否已经运行
 * 参数        : pLockFile[in] (const char *): 指定的加锁文件
 * 返回值      : bool, 已经运行返回true, 否则false
 * 作者        : 许龙杰
 */
bool AlreadyRunningUsingLockFile(const char *pLockFile);

/*
 * 函数名      : AlreadyRunningUsingName
 * 功能        : 使用进程名字判断进程是否已经运行
 * 参数        : pLockFile[in] (const char *): 进程名
 * 返回值      : int32_t, 返回非负数表明PID，以说明该进程整改运行; 返回0表明改进程名没有运行; 否则表示错误
 * 作者        : 许龙杰
 */
int32_t AlreadyRunningUsingName(const char *pName);

/*
 * 函数名      : PrintLog
 * 功能        : 保存日志信息, 使用方法和 C99 printf一致
 * 参数        : pFmt [in]: (const char * 类型) 打印的格式
 *             : ...: [in]: 可变参数
 * 返回值      : (int32_t类型) 0表示成功, 否则错误
 * 作者        : 许龙杰
 */
int32_t PrintLog(const char *pFmt, ...);


/* 线程地址形式重定义 */
typedef void *(*PFUN_THREAD)(void *pArg);

/*
 * 函数名      : MakeThread
 * 功能        : 创建线程
 * 参数        : pThread [in] (PFUN_THREAD): 线程地址
 *             : pArg [in] (void *): 传递给线程的参数
 *             : boIsDetach [in] (bool): 是否是分离线程
 *             : pThreadId [in] (pthread_t *): 不为NULL时用于保存线程ID
 *             : boIsFIFO [in] (bool): 是否打开线程FIFO属性(需要root权限)
 * 返回值      : (int32_t类型) 0表示成功, 否则错误
 * 作者        : 许龙杰
 */
int32_t MakeThread(PFUN_THREAD pThread, void *pArg,
        bool boIsDetach, pthread_t *pThreadId, bool boIsFIFO);

/*
 * 函数名      : ServerListen
 * 功能        : 监听一个UNIX域套接字
 * 参数        : pName[in] (const char * 类型): 不为NULL时, 指定关联名字
 * 返回值      : int32_t 型数据, 正数返回socket, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t ServerListen(const char *pName);

/*
 * 函数名      : ServerAccept
 * 功能        : 获取要连接UNIX域套接字
 * 参数        : s32ListenFd[in] (int32_t类型): ServerListen的返回值
 * 返回值      : int32_t 型数据, 正数失败, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t ServerAccept(int32_t s32ListenFd);
/*
 * 函数名      : ServerRemove
 * 功能        : 删除UNIX域套接字
 * 参数        : s32Socket[in] (int32_t 类型): ServerListen的返回值
 *             : pName[in] (const char * 类型): 服务器的名字
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void ServerRemove(int32_t s32Socket, const char *pName);

/*
 * 函数名      : ClientConnect
 * 功能        : 连接到UNIX域server
 * 参数        : pName[in] (const char * 类型): 不为NULL时, 指定关联名字
 * 返回值      : int32_t 型数据, 正数返回socket, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t ClientConnect(const char *pName);

int32_t SendFd(int32_t s32Socket, int32_t s32Fd);
int32_t ReceiveFd(int32_t s32Socket);


/*
 * 函数名      : ProcessListInit
 * 功能        : 初始化进程维护表在该进程空间的资源，并将该进程信息插入到表中
 * 参数        : pErr[in] (int32_t * 类型): 不为NULL时, 用于保存错误码
 * 返回值      : int32_t 型数据, 错误返回0, 否则返回控制句柄
 * 作者        : 许龙杰
 * 注          : 此函数不再使用
 */
int32_t ProcessListInit(int32_t *pErr);


/*
 * 函数名      : ProcessListUpdate
 * 功能        : 更新进程表，指明当前进程在仍在有效运行
 * 参数        : s32Handle[in] (int32_t): ProcessListInit的返回值
 * 返回值      : int32_t 型数据, 正数返回0, 否则返回错误码
 * 作者        : 许龙杰
 * 注          : 此函数不再使用
 */
int32_t ProcessListUpdate(int32_t s32Handle);


/*
 * 函数名      : ProcessListDestroy
 * 功能        : 从进程表中删除该进程信息，并销毁句柄在该进程空间的资源
 * 参数        : s32Handle[in] (int32_t): ProcessListInit的返回值
 * 返回值      : 无
 * 作者        : 许龙杰
 * 注          : 此函数不再使用
 */
void ProcessListDestroy(int32_t s32Handle);

/*
 * 函数名      : GetProcessNameFromPID
 * 功能        : 通过进程号得到进程名字
 * 参数        : pNameSave [out]: (char * 类型) 将得到的名字保存到的位置
 *             : u32Size [in]: (uint32_t类型) 指示pNameSave的长度
 *             : s32ID [in]: (int32_t类型) 进程号
 * 返回值      : (int32_t类型) 0表示成功, 否则错误
 * 作者        : 许龙杰
 */
int32_t GetProcessNameFromPID(char *pNameSave, uint32_t u32Size, int32_t s32ID);


/*
 * 功能        : 遍历目录的回调函数指针定义
 * 参数        : pCurPath [in] (const char * 类型): 要查询的父目录
 *             : pInfo [in] (struct dirent *类型): 详见定义
 *             : pContext [in] (void *类型): 回调函数的上下文指针
 *             : 返回0为正确，否则为错误码，可导致到用函数退出
 * 作者        : 许龙杰
 */
typedef int32_t (*PFUN_TraversalDir_Callback)(const char *pCurPath, struct dirent *pInfo, void *pContext);

/*
 * 函数名      : TraversalDir
 * 功能        : 遍历目录
 * 参数        : pPathName [in] (const char * 类型): 要查询的目录
 *             : boIsRecursion [in] (bool类型): 是否递归子目录
 *             : pFunCallback [in] (PFUN_TranversaDir_Callback类型): 详见类型定义
 *             : pContext [in] (void *类型): 回调函数的上下文指针
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t TraversalDir(const char *pPathName, bool boIsRecursion,
		PFUN_TraversalDir_Callback pFunCallback, void *pContext);



typedef union _tagUnBteaKey
{
	int32_t s32Key[4];
	char c8Key[16];
}UnBteaKey;

/*
 * 函数名      : btea
 * 功能        : XXTEA加密/解密
 * 参数        : v [in/out] (int32_t * 类型): 需要加密/解密的数据指针
 *             : n [in] (int32_t 类型): 需要加密/解密的数据长度，正值表示加密，负值表示解密
 *             : k [in] (int32_t *): 128位的Key值
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t btea(int32_t *v, int32_t n, int32_t *k);


/*
 * 函数名      : GetDomainPortFromString
 * 功能        : 得到字符串中domain 和 port的信息
 * 参数        : pStr [in] (const char * 类型): 要解析的符串(以'\0'结尾), 例如"www.jiuan.com[:443]"
 * 			   : pDomain [out] (char * 类型): 成功保存域名
 *             : uint32_t [in] (uint32_t 类型): pDomain指向字符串的长度
 *             : pPort [out] (uint32_t * 类型): 成功保存段口号
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t GetDomainPortFromString(const char *pStr, char *pDomain, uint32_t u32Size, int32_t *pPort);


#define IPV4_ADDR_LENGTH					(16)
#define MAC_ADDR_LENGTH						(32)
#define MAC_HEX_ADDR_LENGTH					(6)
#define IPV4_HEX_ADDR_LENGTH				(4)


typedef struct _tagStIPV4Addr
{
	char c8Name[16];								/* 用于保存网卡的名字 */
	char c8IPAddr[IPV4_ADDR_LENGTH];				/* 用于保存网卡的IP */
	char c8Mask[IPV4_ADDR_LENGTH];
	char c8Gateway[IPV4_ADDR_LENGTH];
	char c8DNS[IPV4_ADDR_LENGTH];
	char c8ReserveDNS[IPV4_ADDR_LENGTH];
	char c8MacAddr[MAC_ADDR_LENGTH];
}StIPV4Addr;										/* IPV4网卡信息 */

/*
 * 函数名      : GetInterfaceIPV4Addr
 * 功能        : 得到指定网卡的IPV4信息
 * 参数        : pInterfaceName [in] (const char * 类型): 网卡名字
 *			   : pAddrOut [out] (StIPV4Addr * 类型): 详见定义
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t GetInterfaceIPV4Addr(const char *pInterfaceName, StIPV4Addr *pAddrOut);

/*
 * 函数名      : GetIPV4Addr
 * 功能        : 得到支持IPV4的已经连接的网卡的信息
 * 参数        : pAddrOut [out] (StIPV4Addr * 类型): 详见定义
 *             : pCnt [in/out] (uint32_t * 类型): 作为输入指明pAddrOut数组的数量，
 *               作为输出指明查询到的可用网卡的数量
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t GetIPV4Addr(StIPV4Addr *pAddrOut, uint32_t *pCnt);
/*
 * 函数名      : GetHostIPV4Addr
 * 功能        : 解析域名或者IPV4的IPV4地址
 * 参数        : pHost [in] (const char * 类型): 有结束符的字符串可以是dotted-decimal型的字符串,
 *               例如"192.168.1.1", "50.18.155.75", 也可以是域名,例如"www.baidu.com", "baidu.com"
 *             : c8IPV4Addr [in/out] (char [IPV4_ADDR_LENGTH] 类型): 用于保存IP地址
 *             : pInternetAddr [in/out] (struct in_addr * 类型): 用户保存Internet类型地址
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t GetHostIPV4Addr(const char *pHost, char c8IPV4Addr[IPV4_ADDR_LENGTH], struct in_addr *pInternetAddr);

/*
 * 函数名      : GetHostIPV4AddrTimeout
 * 功能        : 解析域名或者IPV4的IPV4地址, 可设置超时时间
 * 参数        : pHost [in] (const char * 类型): 有结束符的字符串可以是dotted-decimal型的字符串,
 *               例如"192.168.1.1", "50.18.155.75", 也可以是域名,例如"www.baidu.com", "baidu.com"
 *             : u32Time [in] (uint32_t 类型): 超时时间, 单位ms
 *             : c8IPV4Addr [in/out] (char [IPV4_ADDR_LENGTH] 类型): 用于保存IP地址
 *             : pInternetAddr [in/out] (struct in_addr * 类型): 用户保存Internet类型地址
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t GetHostIPV4AddrTimeout(const char *pHost, uint32_t u32Time,
		char c8IPV4Addr[IPV4_ADDR_LENGTH],	struct in_addr *pInternetAddr);
/*
 * 函数名      : GetNetLinkStatus
 * 功能        : 检测有线网卡的网线是否连接
 * 参数        : pInterfaceName [in]: (const char * 类型) 网卡的名字
 * 返回值      : (int32_t类型) 0表示已经连接, 否则表示没有连接或者错误
 * 作者        : 许龙杰
 */
int32_t GetNetLinkStatus(const char *pInterfaceName);

/*
 * 函数名			: GetHostIPV4AddrTimeout
 * 功能			: 解析域名或者IPV4的IPV4地址, 可设置超时时间
 * 参数			: pSocket [in/out] (int32_t * 类型): 指向TCP或者UDP的指针
 *				: s32SockType [in] (int32_t 类型): 该socket的类型
 *				: u16Port [in] (uint16_t 类型): 该socket要绑定的端口号
 *				: pETHName [in] (const char * 类型): 网卡的名称
 * 返回			: 正确返回0，并在*pSocket中赋予新的socket, 错误返回错误码
 * 作者			: 许龙杰
 */
int32_t RebindToEth(int32_t *pSocket, int32_t s32SockType, uint16_t u16Port, const char *pETHName);

/*
 * 函数名      : TimeGetTime
 * 功能        : 得到当前系统时间 (MS级)
 * 参数        : 无
 * 返回值      : 当前系统时间ms
 * 作者        : 许龙杰
 */
uint64_t TimeGetTime(void);


/*
 * 函数名      : TimeGetSetupTime
 * 功能        : 得到当前系统启动时间 (MS级)
 * 参数        : 无
 * 返回值      : 当前系统启动了多长时间
 * 作者        : 许龙杰
 */
uint64_t TimeGetSetupTime(void);

/*
 * DBStore的旗帜定义
 */
#define DB_INSERT	   1	/* 插入一条新的记录 */
#define DB_REPLACE	   2	/* 替换一条已经存在的记录 */
#define DB_STORE	   3	/* 替换或者插入一条记录 */

/*
 * 函数名      : DBOpen
 * 功能        : 打开一个数据库, 和DBClose成对使用, 参数详情, 详见posix的open函数
 * 参数        : pPathName [in] (const char *)名字
 *             : s32Flag [in] (int32_t)标志
 *             : ...可变参数
 * 返回值      : 成功返回指针句柄, 否则返回NULL指针
 * 作者        : 许龙杰
 */
void *DBOpen(const char *pPathName, int32_t s32Flag, ...);


/*
 * 函数名      : DBClose
 * 功能        : 关闭一个打开的数据库, 和DBOpen成对使用
 * 参数        : pHandle [in] (void *)DBOpen的返回值
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void DBClose(void *pHandle);

/*
 * 函数名      : DBDestroy
 * 功能        : 彻底关闭一个打开的数据库(销毁数据内容), 和DBOpen成对使用
 * 参数        : pHandle [in] (void *)DBOpen的返回值
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void DBDestroy(void *pHandle);

/*
 * 函数名      : DBFetch
 * 功能        : 查找一个关键值的内容
 * 参数        : pHandle [in] (void *)DBOpen的返回值
 *             : pKey [in] (const char *)关键值
 * 返回值      : 成功返回指向内容的指针, 否则返回NULL指针
 *               该指针一定不可以使用free释放
 * 作者        : 许龙杰
 */
char *DBFetch(void *pHandle, const char *pKey);

/*
 * 函数名      : DBStore
 * 功能        : 存储一条内容
 * 参数        : pHandle [in] (void *) DBOpen的返回值
 *             : pKey [in] (const char *) 关键值
 *             : pData [in] (const char *) 内容
 *             : s32Flag [in] (int32_t) 详见DBStore的旗帜定义
 * 返回值      : 成功返回0, 否则表示参数错误或者存储失败
 * 作者        : 许龙杰
 */
int32_t DBStore(void *pHandle, const char *pKey, const char *pData, int32_t s32Flag);

/*
 * 函数名      : DBDelete
 * 功能        : 根据关键删除一条记录
 * 参数        : pHandle [in] (void *)DBOpen的返回值
 *             : pKey [in] (const char *)关键值
 * 返回值      : 成功返回0, 否则表示参数错误或者删除失败
 * 作者        : 许龙杰
 */
int32_t DBDelete(void *pHandle, const char *pKey);


/*
 * 函数名      : DBRewind
 * 功能        : 重绕索引文件, 必须在第一次调用DBNextrec之前被调用
 * 参数        : pHandle [in] (void *)DBOpen的返回值
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void DBRewind(void *pHandle);

/*
 * 函数名      : DBNextrec
 * 功能        : 返回下一个连续的记录
 * 参数        : pHandle [in] (void *)DBOpen的返回值
 *             : pKey [in] (const char *)关键值
 * 返回值      : 成功返回指向内容的指针, 否则返回NULL指针
 *               该指针一定不可以使用free释放
 * 作者        : 许龙杰
 */
char *DBNextrec(void *pHandle, char *pKey);

/*
 * 函数名      : CRC32Buf
 * 功能        : 计算一段缓存的CRC32值
 * 参数        : pBuf [in] (uint8_t *)缓存指针
 *             : u32Length [in] (uint32_t)缓存的大小
 * 返回值      : uint32_t 类型, 表示CRC32值
 * 作者        : 许龙杰
 */
uint32_t CRC32Buf(uint8_t *pBuf, uint32_t u32Length);

/*
 * 函数名      : CRC32Buf
 * 功能        : 计算一个文件的CRC32值
 * 参数        : pName [in] (const char *)文件的名字
 *             : pCRC32 [in/out] (uint32_t *)成功在指针指向的内容中保存CRC32值
 * 返回值      : int32_t 类型成功返回0, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t CRC32File(const char *pName, uint32_t *pCRC32);

/*
 * 函数名      : CRC16
 * 功能        : 计算一段缓存的CRC16值
 * 参数        : pBuf [in] (uint8_t *)缓存指针
 *             : u32Length [in] (uint32_t)缓存的大小
 * 返回值      : uint16_t 类型, 表示CRC16值
 * 作者        : 许龙杰
 */
uint16_t CRC16(const uint8_t *pFrame, uint16_t u16Len);

/*
 * 函数名      : RunAProcess
 * 功能        : 运行一个进程
 * 参数        : pName [in] (uint8_t *)进程的名字(完整路径)
 *             : pArgv [in] (const char *)传递给进程的参数, 没有的话传入NULL,
 *               有的话数组必须以NULL为最后一个成员
 * 返回值      : 计算正确返回非负值(子进程号), 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t RunAProcess(const char *pName, const char *pArgv[]);



#ifndef UPGRADE_LIB_COMPLINE

/* 详见 man /proc */
typedef struct _tagStProcessStat
{
	int32_t s32Pid;
/*	char c8Name[256];*/
	char c8State;
/*
	int32_t s32Ppid;
	int32_t s32Pgrp;
	int32_t s32Session;
	int32_t s32Tty_nr;
	int32_t s32Tpgid;

	uint32_t u32Flags;

	uint32_t u32Minflt;
	uint32_t u32Cminflt;
	uint32_t u32Majflt;
	uint32_t u32Cmajflt;
*/
	uint32_t u32Utime;
	uint32_t u32Stime;

/*
	int32_t s32Cutime;
	int32_t s32Cstime;
	int32_t s32Priority;
	int32_t s32Nice;
	int32_t s32Threads;
	int32_t s32Iterealvalue;

	uint64_t u64Starttime;
*/
	uint32_t u32Vsize;
	int32_t s32Rss;
}StProcessStat;
#endif

/*
 * 函数名      : GetStatFileInfo
 * 功能        : 得到stat文件的信息
 * 参数        : pStatFile [in] (const char *): stat文件的全路径
 * 			     pProcessStat[in/out] (StProcessStat): 详见定义
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t GetStatFileInfo(const char *pStatFile, StProcessStat *pProcessStat);

/*
 * 函数名      : GetProcessStat
 * 功能        : 得到当前某一个进程信息
 * 参数        : s32Pid [in] (pid_t): 进程PID
 * 			     pProcessStat[in/out] (StProcessStat): 详见定义
 * 返回值      : int32_t 型数据, 0成功, 否则表示错误码
 * 作者        : 许龙杰
 */
int32_t GetProcessStat(pid_t s32Pid, StProcessStat *pProcessStat);





/*
 * 函数名      : ProcessStatisticNew
 * 功能        : 告知统计线程要增加一个新的进程
 * 参数        : 无
 * 返回值      : int32_t 型数据, 正数返回0, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t ProcessStatisticNew(void);

/*
 * 函数名      : ProcessStatisticDelete
 * 功能        : 告知统计线程要删除一个正在统计的进程
 * 参数        : 无
 * 返回值      : int32_t 型数据, 正数返回0, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t ProcessStatisticDelete(void);

/*
 * 函数名      : ProcessStatisticUpdate
 * 功能        : 告知统计线程要更新一个正在统计的进程
 * 参数        : 无
 * 返回值      : int32_t 型数据, 正数返回0, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t ProcessStatisticUpdate(void);

/*
 * 函数名      : MCSSendToServerAndGetEcho
 * 功能        : 通过MCS同步发送命令并得到反馈
 * 参数        : pServer [in]: (const char * 类型) 服务器的名字
 *             : u32CmdNum [in]: (uint32_t 类型) 命令号
 *             : pSendData [in]: (void * 类型) 发送的数据
 *             : u32SendDataLength [in]: (uint32_t 类型) 发送数据的长度
 *             : pReceiveBuf [in/out]: (void * 类型) 接收的数据缓存
 *             : u32BufLength [in]: (uint32_t 类型) 缓存的长度
 * 返回值      : (int32_t类型) 0表示成功, 否则错误
 * 作者        : 许龙杰
 */
int32_t MCSSendToServerAndGetEcho(const char *pServer, uint32_t u32CmdNum, void *pSendData,
		uint32_t u32SendDataLength, void *pReceiveBuf, uint32_t u32BufLength);


#ifndef UPGRADE_LIB_COMPLINE
typedef enum _tagEmTimeTask
{
	_Time_Task_Periodic,			/* 周期性循环 */
	_Time_Task_OneShot,				/* 单次定时任务 */


	_Time_Task_Reserved,
}EmTimeTask;

/* 定时任务的函数定义形式, 回调函数返回非0时, 中断循环 */
typedef int32_t (*pFUN_TimeTask)(void *pArg);
#endif

/*
 * 函数名      : TimeTaskInit
 * 功能        : 初始化定时任务
 * 参数        : pName [in]: (const char * 类型) 定时任务的名字
 *             : u32Cycle [in]: (uint32_t 类型) 周期的颗粒度(ms), 不建议太小, 也不建议太大尽量不要超过秒级
 *             : pErr [in/out]: (int32_t * 类型) 不为NULL时用于保存错误码
 * 返回值      : (int32_t类型) 非0表示成功, 否则错误
 * 作者        : 许龙杰
 */
int32_t TimeTaskInit(const char *pName, uint32_t u32Cycle, int32_t *pErr);

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
		void *pArg, uint32_t u32Cycle, EmTimeTask emType);

/*
 * 函数名      : TimeTaskDestory
 * 功能        : 销毁定时任务
 * 参数        : s32Handle [in]: (int32_t 类型) TimeTaskInit的返回值
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void TimeTaskDestory(int32_t s32Handle);


/*
 * 函数名		: UARTInit
 * 功能			: 串口初始化
 * 参数			: s32FDUart [in]: (int32_t * 类型) 打开的串口文件描述符
 * 				  s32Bandrate [in]: (int32_t * 类型) 波特率，参考文件<termios.h>，B0～B4000000
 * 				  s32Parity[in]: (int32_t * 类型) 奇偶校验，0无校验，1奇校验，2偶校验
 * 				  s32DataBits[in]: (int32_t * 类型) 数据长度，[5, 8]
 * 				  s32StopBits: (int32_t * 类型) 停止位，[1, 2]
 * 				  u8ReadCnt: (uint8_t * 类型) 参考文件<termios.h>
 * 				  u8ReadTime: (uint8_t * 类型) 参考帮助文档c_cc[VMIN]和c_cc[VTIME]
 * 返回值      : (int32_t类型) 0表示成功, 否则表示错误
 * 作者        : 许龙杰
 */
int32_t UARTInit(int32_t s32FDUart, int32_t s32Bandrate, int32_t s32Parity,
		int32_t s32DataBits, int32_t s32StopBits, uint8_t u8ReadCnt, uint8_t u8ReadTime);



enum
{
	_Protocol_YNA,
	_Protocol_PELCO_D,
	_Protocol_PELCO_P,
	_Protocol_VISCA,



	_Protocol_MCS = 0x1000,

	_Protocol_Reserved,
};

#define PROTOCOL_YNA_ENCODE_LENGTH			10
#define PROTOCOL_YNA_DECODE_LENGTH			8
#define PROTOCOL_PELCO_D_LENGTH				7
#define PROTOCOL_PELCO_P_LENGTH				8
#define PROTOCOL_VISCA_MIN_LENGTH			3
#define PROTOCOL_VISCA_MAX_LENGTH			16

enum
{
	_YNA_Sync,
	_YNA_Addr,
	_YNA_Mix,
	_YNA_Cmd,
	_YNA_Data1,
	_YNA_Data2,
	_YNA_Data3,
	_YNA_CheckSum,
};


typedef struct _tagStYNAAuthForOther
{
	int32_t s32Serial;
	uint8_t u8RandData[16];
	uint8_t u8AuthData[16];
}StYNAAuthForOther;

typedef struct _tagStCycleBuf
{
	char *pBuf;						/* 指向的数据指针 */
	uint32_t u32TotalLength;		/* 数据的总长度 */
	uint32_t u32Write;				/* 当前写到的位置 */
	uint32_t u32Read;				/* 当前读到的位置 */
	uint32_t u32Using;				/* 当前BUF使用量 */
	uint32_t u32Flag;				/* 一些标志 */
}StCycleBuf;


/*
 * 函数名		: CycleMsgInit
 * 功能			: 协议解析初始化
 * 参数			: StCycleBuf [in/out]: (StCycleBuf * 类型) 协议解析使用的句柄
 * 				  pBuf [in]: (void * 类型) 协议解析的缓存位置
 * 				  u32Length[in]: (uint32_t 类型) 缓存大小
 * 返回值      : (int32_t类型) 0表示成功, 否则表示错误
 * 作者        : 许龙杰
 */
int32_t CycleMsgInit(StCycleBuf *pCycleBuf, void *pBuf, uint32_t u32Length);

/*
 * 函数名		: CycleGetOneMsg
 * 功能			: 协议解析
 * 参数			: StCycleBuf [in/out]: (StCycleBuf * 类型) 协议解析使用的句柄
 * 				  pData [in]: (const char * 类型) 待解析数据
 * 				  u32DataLength [in]: (uint32_t 类型) 待解析数据长度
 * 				  pProtocolType [out]: (int32_t * 类型) 协议名称(_Protocol_*)
 * 				  pErr [out]: (int32_t * 类型) 错误信息
 * 返回值		: (void * 类型) 非NULL表示成功，指向数据, 否则表示错误
 * 作者			: 许龙杰
 */
void *CycleGetOneMsg(StCycleBuf *pCycleBuf, const char *pData,
	uint32_t u32DataLength, uint32_t *pLength, int32_t *pProtocolType, int32_t *pErr);


/*
 * 函数名		: YNAMakeAnArrayVarialbleCmd
 * 功能			: 组建一个YNA可变长协议
 * 参数			: u16Cmd [in]: (uint16_t 类型) 命令号
 * 				  pData [in]: (void * 类型) 数据
 * 				  u32Count [in]: (uint32_t 类型) 数据数量
 * 				  u32Length [in]: (uint32_t 类型) 单个数组的长度
 * 				  pCmdLength [out]: (uint32_t * 类型) 组建成功的长度
 * 返回值		: (void * 类型) 非NULL表示成功，指向数据, 否则表示错误
 * 作者			: 许龙杰
 */
void *YNAMakeAnArrayVarialbleCmd(uint16_t u16Cmd, void *pData,
	uint32_t u32Count, uint32_t u32Length, uint32_t *pCmdLength);

/*
 * 函数名		: YNAMakeASimpleVarialbleCmd
 * 功能			: 组建一个简单的YNA可变长协议
 * 参数			: u16Cmd [in]: (uint16_t 类型) 命令号
 * 				  pData [in]: (void * 类型) 数据
 * 				  u32DataLength [in]: (uint32_t 类型) 数据长度
 * 				  pCmdLength [out]: (uint32_t * 类型) 组建成功的长度
 * 返回值		: (void * 类型) 非NULL表示成功，指向数据, 否则表示错误
 * 作者			: 许龙杰
 */
void *YNAMakeASimpleVarialbleCmd(uint16_t u16Cmd, void *pData,
	uint32_t u32DataLength, uint32_t *pCmdLength);

/*
 * 函数名		: YNAGetCheckSum
 * 功能			: 得到一个定长YNA协议校验和
 * 参数			: pBuf [in]: (uint8_t * 类型) 长度为PROTOCOL_YNA_DECODE_LENGTH的数据
 * 返回值		: 无
 * 作者			: 许龙杰
 */
void YNAGetCheckSum(uint8_t *pBuf);

#ifdef __cplusplus
}
#endif

#endif /* UPGRADE_H_ */
