/****************************************************************************
 * Copyright(c), 2001-2060, ************************** 版权所有
 ****************************************************************************
 * 文件名称             : common_define.h
 * 版本                 : 0.0.1
 * 作者                 : 许龙杰
 * 创建日期             : 2014年04月08日
 * 描述                 :
 ****************************************************************************/

#ifndef COMMON_DEFINE_H_
#define COMMON_DEFINE_H_


#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include <signal.h>

#include <dirent.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

/*错误码定义
 * -------------------------------------------------------------------
 * | 31~28  | 27~26 |    25   |  24  |     23 ~ 16    |    15 ~ 0    |
 * |--------|-------|---------|------|----------------|--------------|
 * | 标识域 |  保留 | 0：保留 | 保留 |        保留    | 各模块错误码 |
 * |  0x0E  |       | 1：用户 |      | 暂用于模块分配 |              |
 * -------------------------------------------------------------------
 */
#define FULL_ERROR_CODE(Identification, UserOrReserved, Module, Code) \
        ((((Identification) & 0x0F) << 28) | \
        (((UserOrReserved) & 0x01) << 25) | \
        (((Module) & 0xFF) << 16) | \
        ((Code) & 0xFFFF))

#define ERROR_CODE(Module, Code) 			FULL_ERROR_CODE(0x0E, 0, (Module), (Code))

#define MY_ERR(code)       					ERROR_CODE(0x15, (code))

#pragma pack(4)

#ifdef __cplusplus
extern "C" {
#endif

#define PROGRAM_VERSION						"V20150212-001"

//#define UPGRADE_LIB_COMPLINE
//#define USING_SELF_DNS_RESOLVE

#define LOG_DIR								WORK_DIR"log/"

#define PROCESS_DIR							WORK_DIR


#define LOG_SOCKET							WORK_DIR"logserver.socket" /* 套接字关联文件 */
#define LOG_LOCK_FILE						WORK_DIR"logserver.lock"

#define LOG_FILE							LOG_DIR"logserver.log"

#define DB_NAME								WORK_DIR"db"

#if HAS_CROSS
#define SDCARD_DIR							"/media/sdcard/"
#else
#define SDCARD_DIR							"/home/lyndon/workspace/nfsboot/SDCard/"
#endif

#define UPDATE_DIR							SDCARD_DIR"Update/"
#define RUN_DIR								SDCARD_DIR"Run/"
#define BACKUP_DIR							SDCARD_DIR"Backup/"
#define LIST_FILE							"FileList.json"
#define CONFIG_FILE							RUN_DIR"Config.json"

#if HAS_CROSS
#define GATEWAY_INFO_FILE					WORK_DIR"GatewayInfo.info"
#else
#define GATEWAY_INFO_FILE					RUN_DIR"GatewayInfo.info"
#endif


#define CLI_PERM							S_IRWXU
#define MAX_LINE							(1024)
#define MAX_LISTEN_QUEUE					(10)
#define PAGE_SIZE							(4096)


#define KEY_FILE							"key.key"
#define SEM_FILE							"sem.sem"
#define SHM_FILE							"shm.shm"
#define MSG_FILE							"msg.msg"

#define MODE_RW								(S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
#define LOCK_MODE							(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

#define SEM_MODE							(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)/* 0600 */
#define KEY_MODE							(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)/* 0600 (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) */

#define SHM_SIZE							(PAGE_SIZE * 256)
#define SHM_MODE							(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)/* 0600 */

#define PEOCESS_LIST_NAME					"ProcessList"
#define PEOCESS_LIST_CNT					64

#define CLOUD_NAME							"CloudStat"
#define SUNDRIES_SOCKET						WORK_DIR"SundriesSocket.socket"


#define XXTEA_KEYCNT						(128)
#define XXTEA_KEY_CNT_CHAR					(XXTEA_KEYCNT / (sizeof(char) * 8))

#define RAND_NUM_CNT						(16)
#define PRODUCT_ID_CNT						(16)


#if 0
#define COORDINATION_DOMAINA				"203.195.202.228:443"
//#define COORDINATION_DOMAINA				"192.168.1.22:443"
//#define COORDINATION_DOMAINA				"10.10.10.151:8443"
//#define COORDINATION_DOMAINA				"EuGateWay.iHealthLabs.com:8443"
#define AUTHENTICATION_SC					"001cfe2fe7044aa691d4e6eff9bfb56c"
#define AUTHENTICATION_SV					"56435ce5601f40c59b1db14405578f60"
#define AUTHENTICATION_ADDR					"gateway/auth.ashx"
#define AUTHENTICATION_SECOND_DOMAIN		NULL

#define GET_SELF_REGION_SC					AUTHENTICATION_SC
#define GET_SELF_REGION_SV					"8e8512b786604848a672865fdcdb7b47"
#define GET_SELF_REGION_ADDR				"gateway/gateway_region.ashx"
#define GET_SELF_REGION_SECOND_DOMAIN		NULL

#define GET_REGION_MAPPING_SC				AUTHENTICATION_SC
#define GET_REGION_MAPPING_SV				"620dff9a725345828a052d00d9969a48"
#define GET_REGION_MAPPING_ADDR				"gateway/get_mapping.ashx"
#define GET_REGION_MAPPING_SECOND_DOMAIN	NULL

#if 1
#define GET_LAST_VERSION_SC					AUTHENTICATION_SC
#define GET_LAST_VERSION_SV					"c79ebaffbde2477082112d62b7c7763b"
#define GET_LAST_VERSION_ADDR				"gateway/last_version.ashx"
#else

#define GET_LAST_VERSION_SC					"d52aa0612e9108a934051b4f5070920d"
#define GET_LAST_VERSION_SV					"085a48d31f479f350ecda31084909f67"
#define GET_LAST_VERSION_ADDR				"api/multifwarecheckversion.ashx"
#endif
#define GET_LAST_VERSION_SECOND_DOMAIN		NULL
#else
#define COORDINATION_DOMAINA				"203.195.202.228:443"
//#define COORDINATION_DOMAINA				"192.168.1.22:443"
//#define COORDINATION_DOMAINA				"10.10.10.151:8443"
//#define COORDINATION_DOMAINA				"EuGateWay.iHealthLabs.com:8443"
#define AUTHENTICATION_SC					"001cfe2fe7044aa691d4e6eff9bfb56c"
#define AUTHENTICATION_SV					"56435ce5601f40c59b1db14405578f60"
#define AUTHENTICATION_ADDR					"gateway/auth.htm"
#define AUTHENTICATION_SECOND_DOMAIN		NULL

#define GET_SELF_REGION_SC					AUTHENTICATION_SC
#define GET_SELF_REGION_SV					"8e8512b786604848a672865fdcdb7b47"
#define GET_SELF_REGION_ADDR				"gateway/gateway_region.htm"
#define GET_SELF_REGION_SECOND_DOMAIN		NULL

#define GET_REGION_MAPPING_SC				AUTHENTICATION_SC
#define GET_REGION_MAPPING_SV				"620dff9a725345828a052d00d9969a48"
#define GET_REGION_MAPPING_ADDR				"gateway/get_mapping.htm"
#define GET_REGION_MAPPING_SECOND_DOMAIN	NULL

#if 1
#define GET_LAST_VERSION_SC					AUTHENTICATION_SC
#define GET_LAST_VERSION_SV					"c79ebaffbde2477082112d62b7c7763b"
#define GET_LAST_VERSION_ADDR				"gateway/last_version.htm"
#else

#define GET_LAST_VERSION_SC					"d52aa0612e9108a934051b4f5070920d"
#define GET_LAST_VERSION_SV					"085a48d31f479f350ecda31084909f67"
#define GET_LAST_VERSION_ADDR				"api/multifwarecheckversion.htm"
#endif
#define GET_LAST_VERSION_SECOND_DOMAIN		NULL

#endif

#define DEFAULT_SN							"00000001"
#define DEFAULT_REGION						"SN"

/* 单位: S */
#define CLOUD_KEEPALIVE_TIME				30
#define CLOUD_UPDATE_TIME					(24 * 3600)

#define UDP_MSG_HEAD_NAME					"ANGW"
#define UDP_MSG_HEAD_NAME_LENGTH			4


#if HAS_CROSS
#define ETH_LAN_NAME						"eth2"		/* <TODO> */
#define ETH_WIFI_NAME						"ra0"		/* <TODO> */
#else
#define ETH_LAN_NAME						"p8p1"
#define ETH_WIFI_NAME						"wlp2s0"
#endif

#define UDP_SETWIFI_PORT	(((uint32_t)('H') << 8) | 'G')

typedef struct _tagStUDPMsgHead
{
	char c8HeadName[UDP_MSG_HEAD_NAME_LENGTH];
	char c8Var[3];
	char c8Reserved;
	uint64_t u64TimeStamp;
	uint16_t u16Cmd;
	uint16_t u16CmdLength;
}StUDPMsgHead;
typedef struct _tagStUDPHeartBeat
{
	uint16_t u16QueueNum;
	char c8SN[16];
}StUDPHeartBeat;
typedef struct _tagStUDPConfig
{
	char c8GatewaySN[16];
	char c8ProductSN[16];
}StUDPConfig;
typedef union _tagUnUDPMsg
{
	char c8Buf[64];							/* 占位 */
	struct
	{
		StUDPMsgHead stHead;				/* UDP头 */
		void *pData;						/* UDP数据 */
	};
}UnUDPMsg;



typedef struct _tagStMCSCmdHeader
{
    uint32_t u32CmdNum;      /* 命令号 */
    uint32_t u32CmdCnt;      /* 命令数量 */
    uint32_t u32CmdSize;     /* 命令大小 */
}StMCSCmdHeader;

typedef struct _tagStMSCCmdTag
{
    StMCSCmdHeader stMCSCmdHeader;
    uint32_t u32TotalCnt;
    void *pData;                                  	/* 指向命令数据 */
    struct _tagStMSCCmdTag *pFrontmostPart;         /* 指向本命令的下一部分 */
    struct _tagStMSCCmdTag *pLastPart;              /* 指向本命令的最后一部分 */
    struct _tagStMSCCmdTag *pPrevCmd;               /* 指向上一条命令 */
    struct _tagStMSCCmdTag *pNextCmd;               /* 指向下一条命令 */
}StMCSCmdTag;

typedef struct _tagStMCSHeader
{
    uint8_t u8MixArr[4];                 /* 混合参数 */
    uint32_t u32CmdCnt;                  /* 命令包的数量 */
    uint32_t u32CmdTotalSize;            /* 命令总大小 */
    uint8_t u8CheckSumArr[4];            /* 校验和 */
}StMCSHeader;

typedef struct _tagStMCS
{
    StMCSHeader stMCSHeader;        /* 协议头 */
    StMCSCmdTag *pFrontmostCmd;     /* 指向最前面的命令 */
    StMCSCmdTag *pLastCmd;          /* 指向最后面的命令 */
    uint32_t u32TotalSize;          /* 整个命令流的大小, 在getbuf的时候更新 */
    void *pBuf;                  	/* 用户获取命令数据的时候用到的指针 */
}StMCS;


void LittleAndBigEndianTransfer(char *pDest, const char *pSrc, uint32_t u32Size);


typedef struct _tagStMemCheck
{
	int32_t s32FlagHead; 		/* 0x01234567 */
	int16_t s16RandArray[8];	/* 随机数 */
	int32_t s32FlagTail; 		/* 0x89ABCDEF */
	int32_t s32CheckSum;		/* 校验码 */
} StMemCheck;

typedef struct _tagStSLO
{
	StMemCheck stMemCheck;				/* 校验标志 */
	uint32_t u32SLOTotalSize;			/* 链表的总大小 */
	uint32_t u32RealEntitySize;			/* 链表中实体实际总大小 */
	uint32_t u32EntityWithIndexSize;	/* 链表中实体和游标的总大小 */
	uint32_t u32FlagDataSize;			/* 标志区域的大小  */
	uint32_t u32FlagDataOffset;			/* 标志区域的偏移量 */
	uint32_t u32AdditionDataSize;		/* 附加数据大小 */
	uint32_t u32AdditionDataOffset;		/* 附加数据的偏移量 */
	uint32_t u32EntityOffset;			/* 实体的偏移量 */

	uint16_t u16SLOTotalCapacity;		/* 链表的元素容量 */
	uint16_t u16SLOCurrentCapacity;		/* 链表中当前使用量 */

	uint16_t u16HeadIdle;				/* 空闲链表的头索引 */
	uint16_t u16TailIdle;				/* 空闲链表的尾索引 */

	uint16_t u16Head;					/* 当前正在使用链表的头索引 */
	uint16_t u16Tail;					/* 当前正在使用链表的尾索引 */
} StSLO;

typedef struct _tagStProcessInfo
{
	uint32_t u32Pid;					/* 进程ID */
	char c8Name[64];					/* 进程名字 */

	uint32_t u32UserTime;				/* 进程用户时间 */
	uint32_t u32SysTime;				/* 进程系统时间 */

	uint32_t u32VSize;					/* 进程虚拟内存 */
	uint32_t u32RSS;					/* 进程实际内存 */

	uint16_t u16CPUUsage;				/* 当前进程CPU使用率 */
	uint16_t u16MemUsage;				/* 当前进程MEM使用率 */

	uint16_t u16CPUAverageUsage;		/* 平均进程CPU使用率 */
	uint16_t u16MemAverageUsage;		/* 平均进程MEM使用率 */

	uint32_t u32StatisticTimes;			/* 统计的计数 */

} StProcessInfo;						/* 进程信息 */


typedef struct _tagStProcessInfoNoName
{
	uint32_t u32Pid;					/* 进程ID */

	uint32_t u32UserTime;				/* 进程用户时间 */
	uint32_t u32SysTime;				/* 进程系统时间 */

	uint32_t u32VSize;					/* 进程虚拟内存 */
	uint32_t u32RSS;					/* 进程实际内存 */

	uint16_t u16CPUUsage;				/* 当前进程CPU使用率 */
	uint16_t u16MemUsage;				/* 当前进程MEM使用率 */

	uint16_t u16CPUAverageUsage;		/* 平均进程CPU使用率 */
	uint16_t u16MemAverageUsage;		/* 平均进程MEM使用率 */

} StProcessInfoNoName;						/* 进程信息 */

typedef struct _tagStSLOIndex
{
	uint16_t u16Prev;					/* 前一个内存块 */
	uint16_t u16Next;					/* 后一个内存块 */
	uint16_t u16SelfIndex;				/* 当前索引 */
	uint16_t u16Reserved;				/* 保留，内存对齐 */
} StSLOCursor;

typedef struct _tagStSLOEntity
{
	StProcessInfo stProcessInfo;		/* 自组织链表的实体 */
	StSLOCursor stSLOCursor;			/* 自组织链表的游标 */
} StSLOEntityWithIndex;

typedef struct _tagStSLOHandle
{
	int32_t s32LockHandle;				/* 锁句柄 */
	int32_t s32SHMHandle;				/* 共享内存句柄 */
	StSLO *pSLO;						/* 该进程空间的共享内存影射地址 */
} StSLOHandle;							/* 共享内存操作句柄 */


#define MEM_INFO_CNT		(4)			/* 需要读取MEM信息的数量 */
#define CPU_INFO_CNT		(8)			/* 需要读取CPU信息的数量 */
#define AVERAGE_WEIGHT		(12)		/* 一节滤波权重值 */




#define LOG_DATE_FORMAT		"[%04d-%02d-%02d %02d:%02d:%02d.%03d]" 	/* 日志时间的格式 */
typedef struct _tagStFileCtrlElement StFileCtrlElement;
struct _tagStFileCtrlElement			/* 日志控制中单个文件的控制信息 */
{
	uint64_t u64StartTime;				/* 日志控制中单个文件的开始时间us */
	uint64_t u64EndTime;				/* 日志控制中单个文件的结束时间us */
	uint32_t u32FileSize;				/* 日志控制中单个文件的文件大小 */
	uint32_t u32ValidSize;				/* 日志控制中单个文件的有效内容大小 */
	StFileCtrlElement *pNext;
	StFileCtrlElement *pPrev;
	FILE *pFile;
#ifdef _DEBUG
	char c8Name[_POSIX_PATH_MAX];
#endif
};

typedef struct _tagStFileCtrl StFileCtrl;
struct _tagStFileCtrl								/* 日志控制 */
{
	uint32_t u32MaxCnt;								/* 日志控制包含的最大文件数 */
	uint32_t u32CurUsing;							/* 已经使用的文件数 */
	StFileCtrlElement *pOldestFile;					/* 指向最旧的文件 */
	StFileCtrlElement *pLatestFile;					/* 指向最新的文件 */
	StFileCtrlElement *pFileCtrlElement;			/* 指向文件控制数组 */
	uint64_t u64GlobalStartTime;					/* 全局日志开始时间 */
	uint64_t u64GlobalEndTime;						/* 全局日志结束时间 */
	uint32_t u32GlobalValidSize;					/* 全局日志有效长度 */

	uint64_t u64BufStartTime;						/* 缓存开始时间 */
	uint64_t u64BufEndTime;							/* 缓存结束时间 */
	uint32_t u32BufUsingCnt;						/* 当前缓存的字节数 */
	char c8Buf[PAGE_SIZE];							/* 缓存 */
};

typedef struct _tagStLogFileCtrl					/* 日志控制句柄 */
{
	StFileCtrl *pFileCtrl;							/* 指向日志控制 */
	pthread_mutex_t stMutex;						/* 互斥量 */
}StLogFileCtrl;

int32_t LogFileCtrlInit(const char *pName, int32_t *pErr);
int32_t LogFileCtrlWriteLog(int32_t s32Handle, const char *pLog, int32_t s32Size);
int32_t LogFileCtrlGetAOldestBlockLog(int32_t s32Handle, char **p2Log, uint32_t *pSize);
int32_t LogFileCtrlDelAOldestBlockLog(int32_t s32Handle);
void LogFileCtrlDestroy(int32_t s32Handle);

int32_t CheckSum(int32_t *pData, uint32_t u32Cnt);

void Daemonize(bool boIsFork);

/* 详见 man /proc */
typedef struct _tagStUpTime
{
	double d64UpTime;
	double d64IdleTime;
}StUpTime;
/* 详见 man /proc */
typedef struct _tagStJiffies
{
	uint64_t u64Usr;
	uint64_t u64Nice;
	uint64_t u64Sys;
	uint64_t u64Idle;
	uint64_t u64IOWait;
	uint64_t u64Irq;
	uint64_t u64SoftIrq;
	uint64_t u64Streal;
	uint64_t u64PrevTotal;
	uint64_t u64Total;
	uint32_t u32Usage;
	uint32_t u32AverageUsage;
}StJiffies;
/* 详见 man /proc */
typedef struct _tagStMemInfo
{
	uint32_t u32MemTotal;			/* 可用RAM的总量 */
	uint32_t u32MemFree;			/* 空闲的RAM */
	uint32_t u32Buffers;			/*  */
	uint32_t u32Cached;
	uint32_t u32Usage;
	uint32_t u32AverageUsage;
}StMemInfo;

typedef struct _tagStSYSInfo
{
	StJiffies stCPU;
	StMemInfo stMem;
}StSYSInfo;		/* 系统CPU和MEM信息 */


typedef struct _tagStProcessList
{
	int32_t s32SLOHandle;			/* 自组织链表控制句柄 */
	int32_t s32EntityIndex;			/* 进程申请到的实体索引 */
}StProcessList;


int32_t MemInfo(StMemInfo *pMemInfo);
int32_t CpuInfo(StJiffies *pJiffies);
int32_t GetUpTime(StUpTime *pUpTime);
int32_t GetProcessStat(pid_t s32Pid, StProcessStat *pProcessStat);



extern bool g_boIsExit;
extern char g_c8ID [PRODUCT_ID_CNT];
extern char g_c8Key [XXTEA_KEY_CNT_CHAR];

void *ThreadLogServer(void *pArg);
void *ThreadSundries(void *pArg);
void *ThreadStat(void *pArg);
void *ThreadCloud(void *pArg);


typedef enum _tagEmCloudStat
{
	_Cloud_IsNotOnline = 0,
	_Cloud_IsOnline,
}EmCloudStat;

typedef struct _tagStCloudStat
{
	EmCloudStat emStat;					/* 云在线状态 */
	char c8ClientIPV4[16]; 				/* 可用的IP地址 192.168.100.100\0 */
}StCloudStat;							/* 云状态 */


typedef struct _tagStCloudDomain
{
	StCloudStat stStat;
	char c8Domain[64];					/* 服务器一级域名 */
	int32_t s32Port;					/* 端口 */
}StCloudDomain;							/* 通讯信息 */

typedef struct _tagStCloud
{
	StMemCheck stMemCheck;				/* */
	StCloudStat stStat;					/*  */
}StCloud;								/* 云状态共享内存实体 */

typedef struct _tagStCloudHandle		/* 与StSLOHandle类似 */
{
	int32_t s32LockHandle;
	int32_t s32SHMHandle;
	StCloud *pCloud;
	void *pDBHandle;					/* 数据库句柄 */
}StCloudHandle;


int32_t LockReg(int32_t fd, int32_t cmd, int32_t type, off_t offset, int32_t whence, off_t len); /* {Prog lockreg} */

#define		read_lock(fd, offset, whence, len) \
			LockReg((fd), F_SETLK, F_RDLCK, (offset), (whence), (len))
#define		readw_lock(fd, offset, whence, len) \
			LockReg((fd), F_SETLKW, F_RDLCK, (offset), (whence), (len))
#define		write_lock(fd, offset, whence, len) \
			LockReg((fd), F_SETLK, F_WRLCK, (offset), (whence), (len))
#define		writew_lock(fd, offset, whence, len) \
			LockReg((fd), F_SETLKW, F_WRLCK, (offset), (whence), (len))
#define		un_lock(fd, offset, whence, len) \
			LockReg((fd), F_SETLK, F_UNLCK, (offset), (whence), (len))
#define 	LockFile(fd) \
			LockReg(fd, F_SETLK, F_WRLCK, 0, SEEK_SET, 0)
pid_t	lock_test(int, int, off_t, int, off_t);		/* {Prog locktest} */

#define		is_read_lockable(fd, offset, whence, len) \
			(lock_test((fd), F_RDLCK, (offset), (whence), (len)) == 0)
#define		is_write_lockable(fd, offset, whence, len) \
			(lock_test((fd), F_WRLCK, (offset), (whence), (len)) == 0)

void err_msg(const char *, ...);			/* {App misc_source} */
void err_dump(const char *, ...) __attribute__((noreturn));
void err_quit(const char *, ...) __attribute__((noreturn));
void err_cont(int, const char *, ...);
void err_exit(int, const char *, ...) __attribute__((noreturn));
void err_ret(const char *, ...);
void err_sys(const char *, ...) __attribute__((noreturn));



typedef struct _tagStMMap
{
	FILE *pFile;					/* 指向打开的临时文件 */
	void *pMap;						/* 通过mmap函数得到该进程空间的影射地址 */
	uint32_t u32MapSize;			/* 影射内存的大小 */
}StMMap;							/* 影射文件信息 */

typedef struct _tagStSendInfo
{
	bool boIsGet;					/* 通过GET方法发送?, 否则通过POST方法发送 */
	const char *pSecondDomain;		/* 二级域名 */
	const char *pFile;				/* URL地址(后半部分) */
	const char *pSendBody;			/* 发送的实体 */
	int32_t s32BodySize;			/* 发送的实体的长度 */
}StSendInfo;						/* 发送信息 */

void SSLInit(void);
void SSLDestory(void);
int32_t CloudSendAndGetReturn(StCloudDomain *pStat, StSendInfo *pSendInfo, StMMap *pMap);
int32_t CloudSendAndGetReturnNoSSL(StCloudDomain *pStat, StSendInfo *pSendInfo, StMMap *pMap);
void CloudMapRelease(StMMap *pMap);
int32_t CloudAuthentication(StCloudDomain *pStat, bool boIsCoordination,
		const char c8ID[PRODUCT_ID_CNT], const char c8Key[XXTEA_KEY_CNT_CHAR]);
int32_t CloudGetSelfRegion(StCloudDomain *pStat, char *pRegion, uint32_t u32Size);

typedef struct _tagStRegionMapping
{
	char c8Region[16];							/* 区域名 */
	char c8Cloud[64];							/* 云服务器 */
	char c8Heartbeat[64];						/* 保活服务器 */
}StRegionMapping;
int32_t CloudGetRegionMapping(StCloudDomain *pStat, StRegionMapping **p2Mapping, uint32_t *pCnt);
int32_t CloudKeepAlive(StCloudDomain *pStat, const char c8ID[PRODUCT_ID_CNT]);




#define MAX_REPEAT_CNT			5
#define TIMEOUT_CNT				(6 * MAX_REPEAT_CNT)
#define CMD_CNT					(8 * MAX_REPEAT_CNT)
#define MIN_COMPUTE_CNT			8

typedef struct _tagStUDPInfo
{
	uint16_t u16QueueNum;				/* 该条信息的序号 */
	uint64_t u64SendTime;				/* 本地发送时间 */
	uint64_t u64ReceivedTime;			/* 本地接收到恢复的时间 */
	uint64_t u64ServerTime;				/* 服务器的回复时间 */
	struct _tagStUDPInfo *pPrev;		/* 链表上一个 */
	struct _tagStUDPInfo *pNext;		/* 链表下一个 */
}StUDPInfo;

typedef struct _tagStUDPKeepalive
{

	uint16_t u16OldestSendNum;			/* 最旧的发送序号 */
	uint16_t u16LatestSendNum;			/* 最新的发送序号 */
	uint16_t u16SendCnt;				/* 缓冲中存放信息包的数量, 最大为CMD_CNT */
	uint16_t u16RecvCnt;				/* 当前得到恢复的信息保的数量*/

	uint16_t u16LatestReceivedNum;		/* 最新的接收到的恢复包的序号 */

	StUDPInfo stUDPInfo[CMD_CNT];		/* 信息包数组 */
	StUDPInfo *pCur;					/* 当前可使用的信息包 */
	StUDPInfo *pOldest;					/* 当前最旧的信息包 */
}StUDPKeepalive;

StUDPKeepalive *UDPKAInit(void);
int32_t UDPKAAddASendTime(StUDPKeepalive *pUDP, uint16_t u16SendNum,
	uint64_t u64SendTime);
int32_t UDPKAAddAReceivedTime(StUDPKeepalive *pUDP, uint16_t u16ReceivedNum,
	uint64_t u64ReceivedTime, uint64_t u64ServerTime);
int32_t UDPKAGetTimeDiff(StUDPKeepalive *pUDP, int64_t *pTimeDiff);
int32_t UDPKAClearTimeDiff(StUDPKeepalive *pUDP);
bool UPDKAIsTimeOut(StUDPKeepalive *pUDP);
void UDPKAReset(StUDPKeepalive *pUDP);
void UDPKADestroy(StUDPKeepalive *pUDP);

#define UPDATE_KERNEL		0x00000001
#define UPDATE_REBOOT		(UPDATE_KERNEL << 1)
#define UPDATE_CONFIG		(UPDATE_REBOOT)
#define UPDATE_DRIVER		(UPDATE_REBOOT)
#define UPDATE_SHELL		(UPDATE_REBOOT)
#define UPDATE_LIB			(UPDATE_REBOOT)
#define UPDATE_NORMAL		(UPDATE_REBOOT << 1)
#define KILL_REPEAT_CNT		5


enum
{
	_File_Type_Kernel = 0x02,
	_File_Type_App,
	_File_Type_Driver,
	_File_Type_Peripheral,
	_File_Type_Config,
	_File_Type_Shell,
	_File_Type_Lib,
};


typedef struct _tagStPtlIDPS
{
	char c8Protocol[16];
	char c8Name[16];
	char c8FirmwareVersion[3];
	char c8HardwareVersion[3];
	char c8Manufacturer[16];
	char c8ModelNumber[16];
	char c8SN[16];
	char c8ProductName[32];
}StPtlIDPS;

int32_t GetIDPSStruct(StPtlIDPS *pIDPS);
int32_t GetRegionOfGateway(char *pRegion, uint32_t u32Size);
int32_t GetSNOfGateway(char *pSN, uint32_t u32Size);
int32_t GetServerDomainForGateway(char *pServerDomain, uint32_t u32Size);

typedef struct _tagStUpdateFileInfo
{
	char c8Name[64];
	char c8Url[256];
	uint32_t u32FileType;
	uint32_t u32Version;
	uint32_t u32OldVersion;
	uint32_t u32CRC32;
}StUpdateFileInfo;

typedef struct _tagStMovingFileInfo
{
	StUpdateFileInfo *pFileNew;
	StUpdateFileInfo *pFileDelete;
}StMovingFileInfo;

int32_t UpdateFileCopyToRun(StMovingFileInfo **p2Info);
void UpdateFileInfoRelease(StUpdateFileInfo *pInfo);
void MovingFileInfoRelease(StMovingFileInfo *pInfo);
int32_t UpdateTheNewFile(StMovingFileInfo *pInfo, bool boIsJustForUpdate);



#define HASH_INVALID_INDEX	(0xFFFF)

/**
* The fraction of filled hash buckets until an insert will cause the table
* to be resized.
* This can range from just above 0 up to 1.0.
*/
#define HASH_LOAD_FACTOR	2 / 3

#define HASH_MAX_CAPACITY	4096

/**
* sentinel pointer value for empty slots
*/
#define HASH_EMPTY ((uint16_t)(-1))

/**
* sentinel pointer value for freed slots
*/
#define HASH_FREED ((uint16_t)(-2))

/**
* sentinel pointer value for using slots
*/
#define HASH_USING 				((uint16_t)(0))

#define HASH_KEY_MAX_LENGTH		 (64)
#define HASH_NAME_MAX_LENGTH	 (256)
#define HASH_NAME_PING			"hash_ping"
#define HASH_NAME_PONG			"hash_pong"
#define HASH_INIT_SIZE			128


typedef struct _tagStHashCursor
{
	char c8Key[HASH_KEY_MAX_LENGTH];
	uint16_t u16Index;
	uint16_t u16Next;
	uint16_t u16Prev;
	uint16_t u16SIL;
}StHashCursor;

typedef enum _tagEmPingPong
{
	_Ping_Pong_Unused,
	_Ping_Pong_Using,
	_Ping_Pong_Transfer,
}EmPingPong;

typedef struct _tagStHash
{
	StMemCheck stMemCheck;				/* 校验标志 */
	char c8Name[HASH_NAME_MAX_LENGTH];
	uint32_t u32HashTotalSize;			/* 哈希表的总大小 */
	uint32_t u32RealEntitySize;			/* 哈希表中实体实际总大小 */
	uint32_t u32EntityWithIndexSize;	/* 哈希表中实体和游标的总大小 */
	uint32_t u32FlagDataSize;			/* 标志区域的大小 */
	uint32_t u32FlagDataOffset;			/* 标志区域的偏移量 */
	uint32_t u32AdditionDataSize;		/* 附加数据大小 */
	uint32_t u32AdditionDataOffset;		/* 附加数据的偏移量 */
	uint32_t u32EntityOffset;			/* 实体的偏移量 */
	uint16_t u16HashTotalCapacity;		/* 哈希表的元素容量 */
	uint16_t u16HashCurrentCapacity;	/* 哈希表中当前使用量 */
	uint16_t u16HeadIdle;				/* 空闲哈希表的头索引 */
	uint16_t u16TailIdle;				/* 空闲哈希表的尾索引 */
	uint16_t u16Head;					/* 当前正在使用哈希表的头索引 */
	uint16_t u16Tail;					/* 当前正在使用哈希表的尾索引 */
	uint32_t u32Initval;				/* 哈希表随机种子 */
	uint32_t u32Collisions;				/* 哈希表中索引发生的碰撞 */
	EmPingPong emPingPong;				/* 哈希表PING-PONG机制标志 */
	int32_t s32File;					/* 哈希表影射的文件描述符 */
} StHash;


typedef struct _tagStHashHandle
{
	bool boNeedLock;
	int32_t s32LockHandle;				/* 锁句柄 */
	int32_t s32SHMHandle;				/* 共享内存句柄 */
	StHash *pHash;						/* 该进程空间的共享内存影射地址 */
} StHashHandle;							/* 共享内存操作句柄 */


void HashDestroy(StHashHandle *pHandle);
void HashTombDestroy(StHashHandle *pHandle);
int32_t HashReset(StHashHandle *pHandle);
StHashHandle *HashInit(const char *pName, uint16_t u16Capacity, uint32_t u32EntitySize,
	uint32_t u32AdditionDataSize, bool boNeedLock, int32_t *pErr);
uint16_t HashInsert(StHashHandle *pHandle, const char *pKey, int32_t s32Length,
		void *pData, int32_t *pErr);
void *HashSearch(StHashHandle *pHandle, const char *pKey, int32_t s32Length, int32_t *pErr);
int32_t HashDelete(StHashHandle *pHandle, const char *pKey, int32_t s32Length);
enum
{
	_Hash_Traversal_Process = 0,
	_Hash_Traversal_Thread,
	_Hash_Traversal_Process_Out,
	_Hash_Traversal_Thread_New,
	_Hash_Traversal_Thread_Update,
	_Hash_Traversal_Process_Update,
};

typedef union _tagUnProcrssInfo
{
	StProcessInfo stThreadInfo;
	struct _tagStMainProcess
	{
		uint64_t u64LatestUpdateTime;
		uint16_t u16NotExistCount;
		uint32_t u32StatisticTimes;
	}stMainProcess;

}UnProcessInfo;

#if HAS_CROSS
#define PROCESS_STATISTIC_BASE_PID				150
#else
#define PROCESS_STATISTIC_BASE_PID				2000
#endif
#define PROCESS_STATISTIC_SOCKET				WORK_DIR"statistic.socket" /* 套接字关联文件 */

#define	 PROCESS_STATISTIC_TIME					2000

enum
{
	_Process_Statistic_Add = 0x00006001,
	_Process_Statistic_Delete,
	_Process_Statistic_Update,
};


/*
*             : s32Flag[in] (int32_t): 指明什么时刻调用的该函数
*             : pCursor[in] (const StHashCursor *): 指向游标
*             : pData (void *): 指向数据区域
*             : pContext[in] (void *): 上下文指针
*/
typedef int32_t (*PFUN_Hash_OperateData)(int32_t s32Flag, const StHashCursor *pCursor, void *pData, void *pContext);

StHashHandle *ProcessStatisticInit(int32_t *pErr);
int32_t ProcessStatisticInsertAProcess(StHashHandle **p2Handle, const char *pName);
int32_t ProcessStatisticUpdateAThread(StHashHandle **p2Handle, const char *pProcessName,
		int32_t s32Tid, PFUN_Hash_OperateData pFun, void *pContext);
int32_t ProcessStatisticUpdateAProcess(StHashHandle *pHandle, const char *pName,
		PFUN_Hash_OperateData pFun, void *pContext);
int32_t ProcessStatisticDeleteAProcess(StHashHandle *pHandle, const char *pName);
int32_t ProcessStatisticDeleteAThread(StHashHandle *pHandle, const char *pProcessName, int32_t s32Tid);
void *ProcessStatisticSearchAProcess(StHashHandle *pHandle, const char *pName, int32_t *pErr);

int32_t ProcessStatisticTraversal(StHashHandle *pHandle, PFUN_Hash_OperateData pFun, void *pContext);
int32_t ProcessStatisticPrint(StHashHandle *pHandle);


#define DEVNAME "/dev/HG3_gpio"

#define HG3_LED_Cld_Rd_Lt          0x44  //0b01000100   云红长亮
#define HG3_LED_Cld_Rd_Fl          0x84  //0b10000100   云红闪烁
#define HG3_LED_Cld_Gn_Lt          0x48  //0b01001000   云绿长亮
#define HG3_LED_Cld_Gn_Fl          0x88  //0b10001000   云绿闪烁
#define HG3_LED_Cld_Shut           0x0c  //0b00001100   云全灭



typedef struct _TagStDNSHead
{
	uint16_t u16ID;
	uint16_t u16FLag;
	uint16_t u16Questions;
	uint16_t u16AnswerRRs;
	uint16_t u16AuthorityRRs;
	uint16_t u16AdditionalRRs;
}StDNSHead;


#define DNS_QR			(0x0001 << 15)
#define DNS_OPCODE		(0x000F << 11)
#define DNS_AA			(0x0001 << 10)
#define DNS_TC			(0x0001 << 9)
#define DNS_RD			(0x0001 << 8)
#define DNS_RA			(0x0001 << 7)
#define DNS_ZERO		(0x0007 << 4)
#define DNS_RCODE		(0x000F)

#define DNS_TYPE_A		(0x0001)
#define DNS_CLASS_IN	(0x0001)

#define DNS_PORT		(53)



typedef struct _tagStNetInterfaceConfig
{
	bool boIsDHCP;
	char c8IPV4[IPV4_ADDR_LENGTH];
	char c8Mask[IPV4_ADDR_LENGTH];
	char c8Gateway[IPV4_ADDR_LENGTH];
	char c8DNS[IPV4_ADDR_LENGTH];
	char c8ReserveDNS[IPV4_ADDR_LENGTH];
	int s32DHCPPid;
}StNetInterfaceConfig;

typedef struct _tagStGlobeConfig
{
	StNetInterfaceConfig stETHAddr;
	struct
	{
		char c8SSID[64];
		char c8PSK[64];
		int32_t s32WPAPid;
		StNetInterfaceConfig stWifiAddr;
	}stWifi;
	struct
	{
		int8_t s8TimeZone;
		bool boIsDST;
	}stTimeZoneDST;
	uint64_t u64TS;
	struct
	{
		void *pContext;
	};
}StGlobeConfig;

typedef struct _tagStBaseCommProtocol
{
	uint8_t u8CommandHead;
	uint8_t u8DataLength;
	uint8_t u8StatID;
	uint8_t u8SequenceID;
	uint8_t u8ProductType;
	uint8_t u8CommandID;
}StBaseCommProtocol;

typedef enum _tagEmUDPSetWIFIStat
{
	_UDP_SET_WIFI_Scan,
	_UDP_SET_WIFI_Auth_FA,
	_UDP_SET_WIFI_Auth_FC,
	_UDP_SET_WIFI_QA,
	_UDP_SET_WIFI_Reserved,
}EmUDPSetWIFIStat;



typedef enum _tagEmWPACIC
{
	_WPA_CIC_Ping,
	_WPA_CIC_Status,
	_WPA_CIC_Status_Verbose,
	_WPA_CIC_MIB,
	_WPA_CIC_Interface,
	_WPA_CIC_Level,
	_WPA_CIC_Log_OFF,
	_WPA_CIC_Log_ON,
	_WPA_CIC_Set,
	_WPA_CIC_PMKSA,
	_WPA_CIC_Reassociate,
	_WPA_CIC_Reconfigure,
	_WPA_CIC_Preauthenticate,
	_WPA_CIC_Identity,
	_WPA_CIC_Password,
	_WPA_CIC_New_password,
	_WPA_CIC_Pin,
	_WPA_CIC_Otp,
	_WPA_CIC_Passphrase,
	_WPA_CIC_Bssid,
	_WPA_CIC_List_Networks,
	_WPA_CIC_Select_Network,
	_WPA_CIC_Enable_Network,
	_WPA_CIC_Disable_Network,
	_WPA_CIC_Add_Network,
	_WPA_CIC_Remove_Network,
	_WPA_CIC_Set_Network,
	_WPA_CIC_Get_Network,
	_WPA_CIC_Disconnect,
	_WPA_CIC_Save_Config,
	_WPA_CIC_Scan,
	_WPA_CIC_Scan_Results,
	_WPA_CIC_Get_Capability,
	_WPA_CIC_Ap_Scan,
	_WPA_CIC_Stkstart,
	_WPA_CIC_Terminate,
	_WPA_CIC_PSK_SHA,

	_WPA_CIC_Reserved,
}EmWPACIC;


typedef struct _tagStArgv
{
	uint32_t u32Cnt;
	const char **p2Argv;
}StArgv;

#define CTRL_INTERFACE_DIR			"/var/run/wpa_supplicant/"
#define WIFI_INTERFACE				ETH_WIFI_NAME

const char *WPARequest(EmWPACIC emCmd, const char *pInterface, StArgv *pArgv, int32_t *pResult);


typedef struct _tagStMCSCmdInnerWIFISSIDAndPSK
{
	const char *pSSID;
	const char *pPSK;
}StMCIWIFISSIDAndPSK;

enum
{
	_MCS_Cmd_Inner = 0x00006000,
	_MCS_Cmd_Inner_SetWifiSSIDAndPSK = 0x00006001,			/* StMCIWIFISSIDAndPSK, int32_t */
	_MCS_Cmd_Inner_GetWifiConnectStatus,					/* NULL, int32_t */
	_MCS_Cmd_Inner_LoadEthConfig,
	_MCS_Cmd_Inner_ClearEth,
	_MCS_Cmd_Inner_LoadWifiConfig_Connect,
	_MCS_Cmd_Inner_LoadWifiConfig_SetIp,
	_MCS_Cmd_Inner_ClearWifi,

};

typedef struct _tagStSelfUpdateProtocol
{
	uint8_t u8CommandHead;
	uint8_t u8PortNum;
	uint8_t u8DataLengthMSB;
	uint8_t u8DataLengthLSB;
	uint8_t u8UUID[6];
	uint8_t u8CommandID;
}StSUP;

#define SUP_LENGTH_BYTES_CNT	(offsetof(StSUP, u8UUID) - offsetof(StSUP, u8DataLengthMSB))



typedef struct _tagStTimeTask
{
	char c8Str[64];					/* 定时任务的名字 */
	uint32_t u32Cycle;				/* 循环周期基数, 单位ms */
	pthread_t u32ThreadId;			/* 定时任务的线程ID号 */
	bool boIsExit;					/* 定时任务的终止标志 */
}StTimeTask;

/* 定时任务的函数定义形式, 回调函数返回非0时, 中断循环 */
typedef int32_t (*pFUN_TimeTask)(void *pArg);


typedef struct _tagStTimeTaskInfo StTimeTaskInfo;

typedef struct _tagStTimeTaskBaseInfo
{
	uint32_t u32Cycle;								/* 定时任务的定时时长, 最终时间为该数 * 循环周期基数  */
	EmTimeTask emType;								/* 定时任务的类型 */
	pFUN_TimeTask pFunTask;							/* 回调函数指针 */
	void *pArg;										/* 传递给回调函数的实参 */
}StTimeTaskBaseInfo;
struct _tagStTimeTaskInfo
{
	StTimeTaskBaseInfo stInfo;						/* 定时任务的基本信息 */
	StTimeTaskInfo *pNext;
	StTimeTaskInfo *pPrev;
	uint64_t u64TimeMatch;							/* 匹配计数 */
};

#define TIME_TASK_WHEEL_SIZE						64	/* 定时任务HASH表数量 */



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
#define PRINT(x, ...) printf("[%s:%d]: "x, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define PRINT(x, ...)
#endif

#ifdef __cplusplus
}
#endif

#endif /* COMMON_DEFINE_H_ */
