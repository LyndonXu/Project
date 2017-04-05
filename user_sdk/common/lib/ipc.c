/****************************************************************************
 * Copyright(c), 2001-2060, ************************** 版权所有
 ****************************************************************************
 * 文件名称             : ipc.c
 * 版本                 : 0.0.1
 * 作者                 : 许龙杰
 * 创建日期             : 2014年04月08日
 * 描述                 :
 ****************************************************************************/

//#define NOT_USING_PRINT

#include "common.h"
#include "common_define.h"



/*
 * 函数名      : MakeASpecialKey
 * 功能        : 创建一个IPC key
 * 参数        : pName [in] (char * 类型): 名字
 * 返回值      : 正确返回key_t, 否则返回-1
 * 作者        : 许龙杰
 */
key_t MakeASpecialKey(const char *pName)
{
	int32_t s32Fd = 0;
	char c8Str[128];

	system("mkdir -p " WORK_DIR);
	strcpy(c8Str, WORK_DIR);
	if (NULL == pName)
	{
		strcat(c8Str, KEY_FILE);
	}
	else
	{
		strcat(c8Str, pName);
	}
	/* 如果文件不存在, 创建它 */
	s32Fd = open(c8Str, O_CREAT | O_EXCL | O_RDWR, KEY_MODE);
	if (s32Fd >= 0)
	{
		close(s32Fd);
	}

	return ftok(c8Str, 0);
}

/*
 * 函数名      : LockOpen
 * 功能        : 创建一个新的锁 与 LockClose成对使用
 * 参数        : pName[in] (char * 类型): 记录锁的名字(不包含目录), 为NULL时使用默认名字
 * 返回值      : (int32_t) 成功返回非负数, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t LockOpen(const char *pName)
{
	int32_t s32Err = 0;
	int32_t s32SemId = 0;
	key_t keySem = 0;
	if (NULL == pName)
	{
		pName = SEM_FILE;
	}
	keySem = MakeASpecialKey(pName);

	if (-1 == keySem)
	{
		PRINT("MakeASpecialKey error: %s", strerror(errno));
		s32Err = MY_ERR(_Err_SYS + errno);
		return s32Err;
	}
	PRINT("the key is 0x%08x\n", keySem);
	s32SemId = semget(keySem, 1, IPC_CREAT | IPC_EXCL | SEM_MODE);
	if (s32SemId >= 0)
	{
		if (semctl(s32SemId, 0, SETVAL, 1) == -1)
		{
			semctl(s32SemId, 0, IPC_RMID);
			s32Err = MY_ERR(_Err_SYS + errno);
			return s32Err;
		}
		return s32SemId;
	}
	PRINT("I can't create the semaphore ID, and I will get it\n");
	s32SemId = semget(keySem, 0, SEM_MODE);
	if (s32SemId < 0)
	{
		PRINT("semget error: %s\n", strerror(errno));
		s32Err = MY_ERR(_Err_SYS + errno);
		return s32Err;
	}
	return s32SemId;
}

/*
 * 函数名      : LockClose
 * 功能        : 销毁一个锁, 并释放相关的资源 与 LockOpen成对使用
 * 参数        : s32Handle[in] (JA_SI32类型): LockOpen返回的值
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void LockClose(int32_t s32Handle)
{
	if (s32Handle < 0)
	{
		return;
	}
	if (semctl(s32Handle, 0, IPC_RMID) == -1)
	{
		PRINT("Failed to delete semaphore: %s\n", strerror(errno));
	}

}


/*
 * 函数名      : LockLock
 * 功能        : 加锁 与 LockUnlock成对使用
 * 参数        : s32Handle[in] (JA_SI32类型): LockOpen返回的值
 * 返回值      : 成功返回0, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t LockLock(int32_t s32Handle)
{
	struct sembuf stSem;

	stSem.sem_num = 0;
	stSem.sem_op = -1;
	stSem.sem_flg = SEM_UNDO;
	if (semop(s32Handle, &stSem, 1) == -1)
	{
		PRINT("Failed to lock: %s \n", strerror(errno));
		return MY_ERR(_Err_SYS + errno);
	}
	PRINT("Lock OK\n");
	return 0;
}

/*
 * 函数名      : LockUnlock
 * 功能        : 解锁 与 LockLock 成对使用
 * 参数        : s32Handle[in] (JA_SI32类型): LockOpen返回的值
 * 返回值      : 成功返回0, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t LockUnlock(int32_t s32Handle)
{
	struct sembuf stSem;

	stSem.sem_num = 0;
	stSem.sem_op = 1;
	stSem.sem_flg = SEM_UNDO;
	if (semop(s32Handle, &stSem, 1) == -1)
	{
		PRINT("Failed to unlock: %s \n", strerror(errno));
		return MY_ERR(_Err_SYS + errno);
	}
	PRINT("Unlock OK\n");
	return 0;
}



/*
 * 函数名      : GetTheShmId
 * 功能        : 得到一个共享内存ID 与 ReleaseAShmId 成对使用
 * 参数        : pName [in] (char * 类型): 名字
 *             : u32Size [in] (JA_UI32类型): 共享内存的大小
 * 返回值      : 正确返回非负ID号, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t GetTheShmId(const char * pName, uint32_t u32Size)
{
    key_t keyShm;
    int32_t s32ShmId = 0;

	if (NULL == pName)
	{
		pName = SHM_FILE;
	}
	keyShm = MakeASpecialKey(pName);

    if (keyShm < 0)
    {
        PRINT("MakeASpecialKey error: %s", strerror(errno));
        return MY_ERR(_Err_SYS + errno);
    }
    PRINT("the key is 0x%08x\n", keyShm);
    s32ShmId = shmget(keyShm, u32Size, IPC_CREAT | IPC_EXCL | SHM_MODE);
    if (s32ShmId != -1)
    {
        return s32ShmId;
    }
    PRINT("I can't create the shared memory ID, and I will get it\n");
    s32ShmId = shmget(keyShm, 0, SHM_MODE);
    if (s32ShmId < 0)
    {
        PRINT("shmget error: %s", strerror(errno));
        return MY_ERR(_Err_SYS + errno);
    }
    return s32ShmId;
}

/*
 * 函数名      : ReleaseAShmId
 * 功能        : 释放一个共享内存ID 与 GetTheShmId成对使用, 所有进程只需要一个进程释放
 * 参数        : s32Id [in] (JA_SI32类型): ID号
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void ReleaseAShmId(int32_t s32Id)
{
	if (s32Id < 0)
	{
		return;
	}
	shmctl(s32Id, IPC_RMID, NULL);
}

/*
 * 函数名      : GetTheMsgId
 * 功能        : 得到一个MSG IPCID
 * 参数        : pName [in] (char * 类型): 名字
 *             : u32Size [in] (JA_UI32类型): 共享内存的大小
 * 返回值      : 正确返回非负ID号, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t GetTheMsgId(const char * pName)
{
    key_t keyMsg;
    int32_t s32MsgId = 0;

	if (NULL == pName)
	{
		pName = MSG_FILE;
	}
	keyMsg = MakeASpecialKey(pName);

    if (keyMsg < 0)
    {
        PRINT("MakeASpecialKey error: %s", strerror(errno));
        return MY_ERR(_Err_SYS + errno);
    }
    PRINT("the key is 0x%08x\n", keyMsg);
    s32MsgId = msgget(keyMsg, IPC_CREAT | IPC_EXCL | MODE_RW);
    if (s32MsgId != -1)
    {
        return s32MsgId;
    }
    PRINT("I can't create the message ID, and I will get it\n");
    s32MsgId = msgget(keyMsg, MODE_RW);
    if (s32MsgId < 0)
    {
        PRINT("msgget error: %s", strerror(errno));
        return MY_ERR(_Err_SYS + errno);
    }

    return s32MsgId;
}

/*
 * 函数名      : ReleaseAMsgId
 * 功能        : 从系统中释放一个MSG ID 所有进程只需要一个进程释放
 * 参数        : s32Id [in] (JA_SI32类型): ID号
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void ReleaseAMsgId(int32_t s32Id)
{
	if (s32Id < 0)
	{
		return;
	}
	msgctl(s32Id, IPC_RMID, NULL);
}

