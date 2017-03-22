/****************************************************************************
 * Copyright(c), 2001-2060, ******************************* 版权所有
 ****************************************************************************
 * 文件名称             : sundries.c
 * 版本                 : 0.0.1
 * 作者                 : 许龙杰
 * 创建日期             : 2014年5月7日
 * 描述                 :
 ****************************************************************************/
//#define NOT_USING_PRINT
#include <common.h>
#include "common_define.h"

#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <linux/sockios.h>
#include <fcntl.h>
#include <sys/ioctl.h>


#if 0
#define MAXLINE 4096            /* max line length */
/*
 * 函数名      : ErrDoit
 * 功能        : 打印错误信息
 * 参数        : boIsPrintErr [in] (bool类型): 是否打印错误码
 *             : s32Err [in] (int32_t类型): 错误码
 *             : pFmt [in] (const char * 类型): 格式
 *             : ap [in] (va_list 类型): 可变参数
 * 返回值      : 无
 * 作者        : 许龙杰
 */
static void ErrDoit(bool boIsPrintErr, int32_t s32Err, const char *pFmt,
        va_list ap)
{
    char    c8Buf[MAXLINE];

    vsnprintf(c8Buf, MAXLINE, pFmt, ap);

    PRINT("%s", c8Buf);

    if (boIsPrintErr)
    {
        PRINT("%s", strerror(s32Err));
    }
    PRINT("\n");
}
/*
 * 函数名      : ErrQuit
 * 功能        : 打印错误信息并退出
 * 参数        : pFmt [in] (const char * 类型): 格式
 *             : ... [in] : 可变参数
 * 返回值      : 无
 * 作者        : 许龙杰
 */
static  void ErrQuit(const  char *pFmt, ...)
{
    va_list     ap;

    va_start(ap, pFmt);
    ErrDoit(0, 0, pFmt, ap);
    va_end(ap);
    exit(1);
}
#else
#define ErrQuit		err_quit
#endif

/*
 * 函数名      : Daemonize
 * 功能        : 创建守护进程
 * 参数        : boIsFork [in] (bool类型): 是否创建进程
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void Daemonize(bool boIsFork)
{
    int32_t s32Fd0, s32Fd1, s32Fd2;
    int32_t s32PID;
    struct rlimit       stRlimit;
    struct sigaction    stSigaction;

    umask(0);

    /*
     * 得到文件描述符的最大限制
     */
    if (getrlimit(RLIMIT_NOFILE, &stRlimit) < 0)
    {
        ErrQuit("can't get file limit");
    }

    if(boIsFork)
    {
        /*
         * 成为会话终端header
         */
        if ((s32PID = fork()) < 0)
        {
            ErrQuit("can't fork");
        }
        else if (s32PID != 0)
        {
            exit(0);
        }
        setsid();
        /*
         * 保证将来的打开操作不会申请控制终端
         */
        stSigaction.sa_handler = SIG_IGN;
        sigemptyset(&stSigaction.sa_mask);
        stSigaction.sa_flags = 0;
        if (sigaction(SIGHUP, &stSigaction, NULL) < 0)
        {
             ErrQuit("can't ignore SIGHUP");
        }

        if ((s32PID = fork()) < 0)
        {
            ErrQuit("can't fork");
        }
		else if (s32PID != 0)
		{
			exit(0);
		}
    }



    /*
     * 改变当前工作目录到根目录, 以保证文件系统不会被卸载
     */
    if (chdir(WORK_DIR) < 0)
    {
        ErrQuit("can't change directory to "WORK_DIR);
    }

    /*
     * 关闭所有打开的描述符
     */
    if (stRlimit.rlim_max == RLIM_INFINITY)
    {
        stRlimit.rlim_max = 1024;
    }
    {
    	uint32_t i;
		for (i = 0; i < stRlimit.rlim_max; i++)
		{
			close(i);
		}
    }

    /*
     * 绑定标准输入, 输出, 错误到 /dev/null.
     */
    s32Fd0 = open("/dev/null", O_RDWR);
    s32Fd1 = dup(0);
    s32Fd2 = dup(0);

    if (s32Fd0 != 0 || s32Fd1 != 1 || s32Fd2 != 2)
    {
        ErrQuit("unexpected file descriptors %d %d %d", s32Fd0, s32Fd1, s32Fd2);
    }
}

#if 0
/*
 * 函数名      : LockFile
 * 功能        : 尝试对文件加锁
 * 参数        : s32Fd [in] (int32_t类型): 文件描述符
 * 返回值      : int32_t fcntl的返回值
 * 作者        : 许龙杰
 */
static int32_t LockFile(int32_t s32Fd)
{

    struct flock stFlock;

    stFlock.l_type = F_WRLCK;
    stFlock.l_start = 0;
    stFlock.l_whence = SEEK_SET;
    stFlock.l_len = 0;
    return(fcntl(s32Fd, F_SETLK, &stFlock));

}
#endif
/*
 * 函数名      : AlreadyRunningUsingLockFile
 * 功能        : 检测程序是否已经运行
 * 参数        : 无
 * 返回值      : bool类型数据, 正在运行返回true, 否则返回false
 * 作者        : 许龙杰
 */
bool AlreadyRunningUsingLockFile(const char *pLockFile)
{
    int32_t s32Fd;
    char c8Buf[16];

    if (pLockFile == NULL)
    {
    	return false;
    }
    s32Fd = open(pLockFile, O_RDWR | O_CREAT, LOCK_MODE);
    if (s32Fd < 0)
    {
        syslog(LOG_ERR, "can't open %s: %s", pLockFile, strerror(errno));
        exit(1);
    }
    if (LockFile(s32Fd) < 0)
    {
        if (errno == EACCES || errno == EAGAIN)
        {
            close (s32Fd);
            return true;
        }
        syslog(LOG_ERR, "can't lock %s: %s", pLockFile, strerror(errno));
        exit(1);
    }
    ftruncate(s32Fd, 0);
    lseek(s32Fd, 0, SEEK_SET);
    sprintf(c8Buf, "%ld\n", (long)getpid());
    write(s32Fd, c8Buf, strlen(c8Buf)+1);
    return false;
}


static int32_t TraversalDirCallback(const char *pCurPath, struct dirent *pInfo, void *pContext)
{
	/* not a directory */
#if !HAS_CROSS
	if ((pInfo->d_type & DT_DIR) == 0)
	{
		return 0;
	}
#endif
	/* not a process directory */
	if ((pInfo->d_name[0] > '9') || (pInfo->d_name[0] < '0'))
	{
		return 0;
	}

	{
		char c8Name[_POSIX_PATH_MAX];
		int32_t s32Err;
		int32_t s32Pid = atoi(pInfo->d_name);
		s32Err = GetProcessNameFromPID(c8Name, _POSIX_PATH_MAX, s32Pid);
		if (s32Err != 0)
		{
			/* PRINT("s32Err: 0x%08x\n", s32Err);*/
			return 0;
		}
		/* PRINT("pInfo->d_name: %s, c8Name: %s\n", pInfo->d_name, c8Name);*/
		if (strstr(c8Name, (const char *)pContext) != NULL)
		{
			return s32Pid;
		}
	}

	return 0;
}

/*
 * 函数名      : AlreadyRunningUsingName
 * 功能        : 使用进程名字判断进程是否已经运行
 * 参数        : pLockFile[in] (const char *): 进程名
 * 返回值      : int32_t, 返回非负数表明PID，以说明该进程整改运行; 返回0表明改进程名没有运行; 否则表示错误
 * 作者        : 许龙杰
 */
int32_t AlreadyRunningUsingName(const char *pName)
{

	if (pName == NULL)
	{
		return false;
	}

	return TraversalDir("/proc/", false, TraversalDirCallback, (void *)pName);
}

static int32_t IsADir(const char *pPathName)
{
	int32_t s32Tmp1 = -1, s32Tmp2 = -1, s32Tmp3 = -1, s32Tmp4 = -1;

	struct stat stStat =
	{ 0 };

	(void) s32Tmp1;
	(void) s32Tmp2;
	(void) s32Tmp3;
	(void) s32Tmp4;

	if (NULL == pPathName)
	{
		return MY_ERR(_Err_NULLPtr);
	}

	if (lstat(pPathName, &stStat) < 0)
	{
		return MY_ERR(_Err_SYS + errno);
	}

	if (0 == S_ISDIR(stStat.st_mode))
	{
		PRINT("the path is: %s", pPathName);
		return MY_ERR(_Err_NotADir);
	}
	return 0;
}

/*
 * 函数名      : TraversalDir
 * 功能        : 遍历目录
 * 参数        : pPathName [in] (const char * 类型): 要查询的目录
 *             : boIsRecursion [in] (bool类型): 是否递归子目录
 *             : pFunCallback [in] (PFUN_TraversalDir_Callback类型): 详见类型定义
 *             : pContext [in] (void *类型): 回调函数的上下文指针
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t TraversalDir(const char *pPathName, bool boIsRecursion,
		PFUN_TraversalDir_Callback pFunCallback, void *pContext)
{
	DIR *pDirHandle = NULL;
	struct dirent stDirent = { 0 }, *pDirent = NULL;
	int32_t s32Err = 0;

	if (NULL == pPathName)
	{
		return MY_ERR(_Err_NULLPtr);
	}

	s32Err = IsADir(pPathName);
	if (s32Err != 0)
	{
		return s32Err;
	}

	pDirHandle = opendir(pPathName);
	if (NULL == pDirHandle)
	{
		return MY_ERR(_Err_SYS + errno);
	}
	while (NULL != (pDirent = readdir(pDirHandle)))
	{
		stDirent = *pDirent;
		s32Err = 0;
		if ((strcmp(stDirent.d_name, ".")) == 0)
		{
			continue;
		}
		if ((strcmp(stDirent.d_name, "..")) == 0)
		{
			continue;
		}
		if (pFunCallback != NULL)
		{
			if ((s32Err = pFunCallback(pPathName, &stDirent, pContext)) != 0)
			{
				return s32Err;
			}
		}
		if (
#if !HAS_CROSS
				((stDirent.d_type & DT_DIR) != 0) &&
#endif
				boIsRecursion)
		{
			char *pPathInner = (char *) malloc(_POSIX_PATH_MAX + 1);
			if (NULL == pPathInner)
			{
				return MY_ERR(_Err_Mem);
			}
			strcpy(pPathInner, pPathName);
			if (pPathInner[strlen(pPathInner) - 1] != '/')
			{
				PRINT("%s\n", pPathInner);
				strcat(pPathInner, "/");
			}
			strcat(pPathInner, stDirent.d_name);
			TraversalDir(pPathInner, boIsRecursion, pFunCallback, pContext);
			free(pPathInner);
		}
	}
	closedir(pDirHandle);
	return 0;
}



#define	 MX ((((z >> 5) ^ (y << 2)) + ((y >> 3) ^ (z << 4))) ^ ((sum ^ y) + (k[(p & 3) ^ e] ^ z)))
//#define MX		(((z >> 5 ^ y << 2) + (y >> 3 ^ z << 4)) ^ ((sum ^ y) + (k[(p & 3) ^ e] ^ z)))

/*
 * 函数名      : btea
 * 功能        : XXTEA加密/解密
 * 参数        : v [in/out] (int32_t * 类型): 需要加密/解密的数据指针
 *             : n [in] (int32_t 类型): 需要加密/解密的数据长度，正值表示加密，负值表示解密
 *             : k [in] (int32_t *): 128位的Key值
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t btea(int32_t *v, int32_t n, int32_t *k)
{
	uint32_t z = 0, y = v[0], sum = 0, e, DELTA = 0x9e3779b9;
	int32_t p, q;
	if (n > 1)
	{ /* Coding Part */
		z = v[n - 1];
		q = 6 + 52 / n;
		while (q-- > 0)
		{
			sum += DELTA;
			e = (sum >> 2) & 3;
			for (p = 0; p < n - 1; p++)
			{
				y = v[p + 1];
				v[p] += MX;
				z = v[p];
			}
			y = v[0];
			v[n - 1] += MX;
			z = v[n - 1];
		}
		return 0;
	}
	else if (n < -1)
	{ /* Decoding Part */
		n = -n;
		z = v[n - 1];
		q = 6 + 52 / n;
		sum = q * DELTA;
		while (sum != 0)
		{
			e = (sum >> 2) & 3;
			for (p = n - 1; p > 0; p--)
			{
				z = v[p - 1];
				v[p] -= MX;
				y = v[p];
			}
			z = v[n - 1];
			v[0] -= MX;
			y = v[0];
			sum -= DELTA;
		}
		return 0;
	}
	return 1;
}


/* get the check sum */
int32_t CheckSum(int32_t *pData, uint32_t u32Cnt)
{
	uint32_t i;
	int32_t s32CheckSum = 0;
	for (i = 0; i < u32Cnt; i++)
	{
		s32CheckSum += pData[i];
	}
	s32CheckSum = ~s32CheckSum;
	s32CheckSum -= 1;
	return s32CheckSum;
}

/*
 * 函数名      : GetInterfaceIPV4Addr
 * 功能        : 得到指定网卡的IPV4信息
 * 参数        : pInterfaceName [in] (const char * 类型): 网卡名字
 *			   : pAddrOut [out] (StIPV4Addr * 类型): 详见定义
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t GetInterfaceIPV4Addr(const char *pInterfaceName, StIPV4Addr *pAddrOut)
{
	struct ifreq stIfreq;
	int32_t s32FD;

	if ((pInterfaceName == NULL) || (pAddrOut == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}

	s32FD = socket(AF_INET, SOCK_DGRAM, 0);
	if (s32FD < 0)
	{
		return MY_ERR(_Err_SYS + errno);
	}

	strncpy(stIfreq.ifr_name, pInterfaceName, sizeof(stIfreq.ifr_name));
	memset(pAddrOut[0].c8MacAddr, 0, MAC_ADDR_LENGTH);
	if (ioctl(s32FD, SIOCGIFHWADDR, &stIfreq) >= 0)
	{
		memcpy(pAddrOut[0].c8MacAddr, stIfreq.ifr_hwaddr.sa_data, 6);
	}
	stIfreq.ifr_addr.sa_family = AF_INET;
	if (ioctl(s32FD, SIOCGIFADDR, &stIfreq) == -1)
	{
		close(s32FD);
		return MY_ERR(_Err_SYS + errno);
	}
	close(s32FD);
	{
	int32_t s32Rslt = getnameinfo(&stIfreq.ifr_addr, sizeof(struct sockaddr),
			pAddrOut[0].c8IPAddr, 16, NULL,	0, NI_NUMERICHOST);
	if (s32Rslt != 0)
	{
		PRINT("getnameinfo error: %s\n", strerror(errno));
		return MY_ERR(_Err_SYS + errno);
	}
	}
	strncpy(pAddrOut[0].c8Name, pInterfaceName, IPV4_ADDR_LENGTH);
	PRINT("network :%s address: %s\n", pInterfaceName, pAddrOut[0].c8IPAddr);
#if defined _DEBUG
	{
		uint64_t *pAddr = (uint64_t *)pAddrOut[0].c8MacAddr;
		PRINT("and hardware address is %016llX\n", *pAddr);
	}
#endif

	return 0;
}

/*
 * 函数名      : GetIPV4Addr
 * 功能        : 得到支持IPV4的已经连接的网卡的信息
 * 参数        : pAddrOut [out] (StIPV4Addr * 类型): 详见定义
 *             : pCnt [in/out] (uint32_t * 类型): 作为输入指明pAddrOut数组的数量，
 *               作为输出指明查询到的可用网卡的数量
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t GetIPV4Addr(StIPV4Addr *pAddrOut, uint32_t *pCnt)
{
#if 1
	if ((pAddrOut == NULL) || (pCnt == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}
	{
	uint32_t u32Cnt = 0, i, u32NetworkCnt = 0;
	char c8NewWork[16][16];
	{
	FILE *pFile;
	char c8Buf[256];
	c8Buf[0] = 0;

	pFile = fopen("/proc/net/dev", "rb");
	if (pFile == NULL)
	{
		return MY_ERR(_Err_SYS + errno);
	}
	fgets(c8Buf, 256, pFile);
	fgets(c8Buf, 256, pFile);
	while (fgets(c8Buf, 256, pFile) != NULL)
	{
		char *pTmp = strchr(c8Buf, ':');
		if (pTmp == NULL)
		{
			continue;
		}
		*pTmp = 0;
		sscanf(c8Buf, "%s", c8NewWork[u32Cnt]);
		/*PRINT("network: %s\n", c8NewWork[u32Cnt]);*/
		u32Cnt++;
		if (u32Cnt >= 16)
		{
			break;
		}
	}
	fclose(pFile);
	}
	for (i = 0; i < u32Cnt; i++)
	{
		struct ifreq stIfreq;
		int32_t s32FD;
		s32FD = socket(AF_INET, SOCK_DGRAM, 0);
		if (s32FD < 0)
		{
			continue;
		}

		strncpy(stIfreq.ifr_name, c8NewWork[i], sizeof(stIfreq.ifr_name));
		memset(pAddrOut[u32NetworkCnt].c8MacAddr, 0, MAC_ADDR_LENGTH);
		if (ioctl(s32FD, SIOCGIFHWADDR, &stIfreq) >= 0)
		{
			memcpy(pAddrOut[u32NetworkCnt].c8MacAddr, stIfreq.ifr_hwaddr.sa_data, 6);
		}
		stIfreq.ifr_addr.sa_family = AF_INET;
		if (ioctl(s32FD, SIOCGIFADDR, &stIfreq) == -1)
		{
			close(s32FD);
			continue;
		}
		close(s32FD);
		{
		int32_t s32Rslt = getnameinfo(&stIfreq.ifr_addr, sizeof(struct sockaddr),
				pAddrOut[u32NetworkCnt].c8IPAddr, 16, NULL,	0, NI_NUMERICHOST);
		if (s32Rslt != 0)
		{
			PRINT("getnameinfo error: %s\n", strerror(errno));
			continue;
		}
		}
		strncpy(pAddrOut[u32NetworkCnt].c8Name, c8NewWork[i], IPV4_ADDR_LENGTH);
		PRINT("network :%s address: %s\n", c8NewWork[i], pAddrOut[u32NetworkCnt].c8IPAddr);
#if defined _DEBUG
		{
			uint64_t *pAddr = (uint64_t *)pAddrOut[u32NetworkCnt].c8MacAddr;
			PRINT("and hardware address is %016llX\n", *pAddr);
		}
#endif
		u32NetworkCnt++;
		if (u32NetworkCnt >= (*pCnt))
		{
			return 0;
		}
	}
	*pCnt = u32NetworkCnt;
	return 0;
	}


#else	/* mips dos't supported */
#include <ifaddrs.h>
	struct ifaddrs *pInterfaceAddr, *pI;
	int32_t s32Family;
	uint32_t u32NetworkCnt = 0;
	char c8Host[16];

	if ((pAddrOut == NULL) || (pCnt == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}
	if (getifaddrs(&pInterfaceAddr) == -1)
	{
		PRINT("getifaddrs error: %s\n", strerror(errno));
		return MY_ERR(_Err_SYS + errno);
	}

	/* Walk through linked list, maintaining head pointer so we
	 can free list later */

	for (pI = pInterfaceAddr; pI != NULL; pI = pI->ifa_next)
	{
		if (pI->ifa_addr == NULL)
			continue;

		s32Family = pI->ifa_addr->sa_family;

		if (s32Family == AF_INET )
		{
			int32_t s32Rslt = getnameinfo(pI->ifa_addr, sizeof(struct sockaddr_in), c8Host, 16, NULL,
					0, NI_NUMERICHOST);
			if (s32Rslt != 0)
			{
				PRINT("getnameinfo error: %s\n", strerror(errno));
				freeifaddrs(pInterfaceAddr);
				return MY_ERR(_Err_SYS + errno);
			}
			PRINT("network :%s address: %s\n", pI->ifa_name, c8Host);
			strncpy(pAddrOut[u32NetworkCnt].c8Name, pI->ifa_name, 16);
			strncpy(pAddrOut[u32NetworkCnt].c8IPAddr, c8Host, IPV4_ADDR_LENGTH);
			u32NetworkCnt++;
			if (u32NetworkCnt >= (*pCnt))
			{
				freeifaddrs(pInterfaceAddr);
				return 0;
			}
		}
	}
	*pCnt = u32NetworkCnt;
	freeifaddrs(pInterfaceAddr);
#endif
	return 0;
}

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
int32_t GetHostIPV4Addr(const char *pHost, char c8IPV4Addr[IPV4_ADDR_LENGTH], struct in_addr *pInternetAddr)
{
	if (pHost == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	else
	{
		struct addrinfo stFilter = {0};
		struct addrinfo *pRslt = NULL;
		int32_t s32Err = 0;
		stFilter.ai_family = AF_INET;
		stFilter.ai_socktype = SOCK_STREAM;
		s32Err = getaddrinfo(pHost, NULL, &stFilter, &pRslt);
		if (s32Err == 0)
		{
			struct sockaddr_in *pTmp =  (struct sockaddr_in *)(pRslt->ai_addr);
			PRINT("server ip: %s\n", inet_ntoa(pTmp->sin_addr));
			if (pInternetAddr != NULL)
			{
				*pInternetAddr = pTmp->sin_addr;
			}
			if (c8IPV4Addr != NULL)
			{
				inet_ntop(AF_INET, (void *)(&(pTmp->sin_addr)), c8IPV4Addr, IPV4_ADDR_LENGTH);
				PRINT("after inet_ntoa: %s\n", c8IPV4Addr);		/* just get the first address */
			}
			freeaddrinfo(pRslt);
			return 0;
		}
		else
		{
			PRINT("getaddrinfo s32Err = %d, error: %s\n", s32Err, gai_strerror(s32Err));
			s32Err = 0 - s32Err;
			return MY_ERR(_Err_Unkown_Host + s32Err);
		}
	}
}

/* 构建DNS查询 */
int32_t DNSBuildQuery(const char *pHost, char *pBuf)
{
	int32_t s32Result = -1;
	uint32_t u32Tmp = 0;
	uint32_t u32Lenght;

	StDNSHead *pHead = (StDNSHead *)pBuf;
	const char *pTmp;
	char *pCharTmp, *pTmp1;
	int16_t *pShortTmp;

	pHead->u16ID = rand();
	pHead->u16FLag = htons(0x0100);
	pHead->u16Questions = htons(0x0001);
	pHead->u16AnswerRRs = 0;
	pHead->u16AuthorityRRs = 0;
	pHead->u16AdditionalRRs = 0;
	u32Tmp += sizeof(StDNSHead);

	pCharTmp = pBuf + u32Tmp;
	pTmp = pHost;
	while((pTmp1 = strchr(pTmp, '.')) != NULL)
	{
		u32Lenght = pTmp1 - pTmp;
		if (u32Lenght > 64)
		{
			return MY_ERR(_Err_Common);
		}
		*pCharTmp = u32Lenght;
		memcpy(pCharTmp + 1, pTmp, u32Lenght);
		u32Tmp += (u32Lenght + 1);
		pCharTmp = pBuf + u32Tmp;

		pTmp = pTmp1 + 1;
	}
	if (pTmp[0] != 0)
	{
		u32Lenght = strlen(pTmp);
		*pCharTmp = u32Lenght;
		memcpy(pCharTmp + 1, pTmp, u32Lenght);
		u32Tmp += (u32Lenght + 1);
		pCharTmp = pBuf + u32Tmp;
	}
	pCharTmp[0] = 0;
	u32Tmp += 1;
	pShortTmp = (int16_t *)(pBuf + u32Tmp);

	*pShortTmp = htons(DNS_TYPE_A);		/* type A, host addr */
	pShortTmp++;
	u32Tmp += sizeof(int16_t);

	*pShortTmp = htons(DNS_CLASS_IN);		/* class IN */
	pShortTmp++;
	u32Tmp += sizeof(int16_t);

	s32Result = u32Tmp;
	return s32Result;
}

/* 解析DNS 反馈 得到IP地址 */
int32_t ResolveDNSAnswer(const char *pSendBuf, const uint32_t u32SendLength,
		const char *pRecvBuf, const uint32_t u32RecvLength,
		char c8IPV4Addr[IPV4_ADDR_LENGTH], struct in_addr *pInternetAddr)
{
	StDNSHead stDNSHead = *((StDNSHead *)pRecvBuf);
	if (((StDNSHead *)pRecvBuf)->u16ID != stDNSHead.u16ID)
	{
		PRINT("it's not my ID\n");
		return MY_ERR(_Err_Common);
	}
	stDNSHead.u16FLag = ntohs(stDNSHead.u16FLag);

	if ((stDNSHead.u16FLag & DNS_QR) != DNS_QR)
	{
		PRINT("it's not an answer\n");
		return MY_ERR(_Err_Common);
	}
	if ((stDNSHead.u16FLag & DNS_OPCODE) == 0)
	{
		uint32_t u32Read = 0;
		int16_t u16Type, u16Class, u16DataLength;
		int32_t s32LiveTime;
		const signed char *pCharTmp;
		const int16_t *pShortTmp;
		const int32_t *pLongTmp;
		(void)u16Type, (void)u16Class, (void)u16DataLength, (void)s32LiveTime;
		if ((stDNSHead.u16FLag & DNS_RCODE) != 0)
		{
			PRINT("host name error\n");
			return MY_ERR(_Err_Common);
		}
		stDNSHead.u16Questions = ntohs(stDNSHead.u16Questions);
		stDNSHead.u16AnswerRRs = ntohs(stDNSHead.u16AnswerRRs);
		stDNSHead.u16AuthorityRRs = ntohs(stDNSHead.u16AuthorityRRs);
		stDNSHead.u16AdditionalRRs = ntohs(stDNSHead.u16AdditionalRRs);
		if (stDNSHead.u16Questions != 1)
		{
			PRINT("DNS error, it's not my queries\n");
			return MY_ERR(_Err_Common);
		}
		if (memcmp(pSendBuf + sizeof(StDNSHead),
				pRecvBuf + sizeof(StDNSHead),
				u32SendLength - sizeof(StDNSHead)) != 0)
		{
			PRINT("DNS error, it's not my queries\n");
			return MY_ERR(_Err_Common);
		}
		u32Read = u32SendLength;
		while(u32Read < u32RecvLength)
		{
			pCharTmp = (const signed char *)(pRecvBuf + u32Read);
			while ((pCharTmp[0] != 0) && (u32Read < u32RecvLength))
			{
				if (pCharTmp[0] < 0)
				{
					pCharTmp += 2;
					u32Read +=2;
				}
				else
				{
					u32Read += (pCharTmp[0] + 1);
					pCharTmp += (pCharTmp[0] + 1);
				}
			}	/* host name */
			if ((pCharTmp[0] == 0) && (pCharTmp[1] == 0))
			{
				u32Read += 1;
			}
			if (u32Read >= u32RecvLength)
			{
				PRINT("DNS error\n");
				return MY_ERR(_Err_Common);
			}
			pShortTmp = (const int16_t *)(pRecvBuf + u32Read);
			u16Type = ntohs(*pShortTmp);
			u32Read += sizeof(const int16_t);

			pShortTmp++;
			u16Class = ntohs(*pShortTmp);
			u32Read += sizeof(const int16_t);

			pLongTmp = (const int32_t *)(pRecvBuf + u32Read);
			s32LiveTime = ntohl(*pLongTmp);
			u32Read += sizeof(const int32_t);

			pShortTmp = (const int16_t *)(pRecvBuf + u32Read);
			u16DataLength = ntohs(*pShortTmp);
			u32Read += sizeof(const int16_t);
			if (u16Type == DNS_TYPE_A)
			{
				struct in_addr stInternetAddr;
				pLongTmp = (const int32_t *)(pRecvBuf + u32Read);
				/* stInternetAddr.s_addr = ntohl(*pLongTmp); */
				stInternetAddr.s_addr = *pLongTmp;
				if (c8IPV4Addr != NULL)
				{
					inet_ntop(AF_INET, (void *)(&stInternetAddr), c8IPV4Addr, IPV4_ADDR_LENGTH);
					PRINT("after inet_ntoa: %s\n", c8IPV4Addr);		/* just get the first address */
				}
				if (pInternetAddr != NULL)
				{
					*pInternetAddr = stInternetAddr;
				}
				return 0;
			}
		}
	}
	else
	{
		PRINT("do not support, 0x%04x\n", (stDNSHead.u16FLag & DNS_RCODE));
		return MY_ERR(_Err_Common);
	}
	return MY_ERR(_Err_Common);
}
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
		char c8IPV4Addr[IPV4_ADDR_LENGTH],	struct in_addr *pInternetAddr)
{
	int32_t s32Result;
	if (pHost == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	if (inet_pton(AF_INET, pHost, &s32Result) > 0)
	{
		if (c8IPV4Addr != NULL)
		{
			strncpy(c8IPV4Addr, pHost, IPV4_ADDR_LENGTH);
		}
		if (pInternetAddr != NULL)
		{
			pInternetAddr->s_addr = s32Result;
		}
		PRINT("IPV4 is %s %08X\n", pHost, pInternetAddr->s_addr);
		return 0;
	}
	else
	{
		char c8Buf[512];
		uint32_t u32Lenght;
		s32Result = DNSBuildQuery(pHost, c8Buf);
		if (s32Result < 0)
		{
			return s32Result;
		}
		u32Lenght = s32Result;
#if 0
		{
			uint32_t i;
			for (i = 0; i < u32Lenght; i++)
			{
				if ((i & 0x0F) == 0)
				{
					printf("\n");
				}
				printf("%02hhX ", c8Buf[i]);
			}
			printf("\n");
		}
#endif
		do
		{
			FILE *pFile = fopen("/etc/resolv.conf", "rb");
			char c8ReadBuf[512];
			if (pFile == NULL)
			{
				s32Result = MY_ERR(_Err_SYS + errno);
				PRINT("open file /etc/resolv.conf error: %s\n", strerror(errno));
				return s32Result;
			}
			while (feof(pFile) == 0)
			{
				char *pRead = fgets(c8ReadBuf, 512, pFile);
				if (pRead == NULL)
				{
					fclose(pFile);
					s32Result = MY_ERR(_Err_SYS + errno);
					PRINT("read file /etc/resolv.conf error: %s\n", strerror(errno));
					return s32Result;
				}
				if (pRead[0] == '#')
				{
					continue;
				}
				if (pRead[strlen(pRead) - 1] == '\n')
				{
					pRead[strlen(pRead) - 1] = 0;
				}

				if (inet_pton(AF_INET, c8ReadBuf + 11, &s32Result) <= 0)
				{
					PRINT("unknown string: %s\n", c8ReadBuf + 11);
					continue;
				}
				else
				{
					struct sockaddr_in stServerAddr = {0}, stAddr = {0};
					struct timeval stTimeout;

					int32_t s32Socket = socket(AF_INET, SOCK_DGRAM, 0);
					if (s32Socket < 0)
					{
						fclose(pFile);
						s32Result = MY_ERR(_Err_SYS + errno);
						PRINT("socket error: %s\n", strerror(errno));
						return s32Result;
					}

					stServerAddr.sin_family = AF_INET;
					stServerAddr.sin_port = htons(DNS_PORT);
					stServerAddr.sin_addr.s_addr = s32Result;

					stAddr.sin_family = AF_INET;
					stAddr.sin_port = htons(0);
					stAddr.sin_addr.s_addr = htonl(INADDR_ANY);
					if (bind(s32Socket, (struct sockaddr *)(&stAddr), sizeof(struct sockaddr)))
					{
						fclose(pFile);
						close(s32Socket);
						s32Result = MY_ERR(_Err_SYS + errno);
						PRINT("socket error: %s\n", strerror(errno));
						return s32Result;
					}

				    stTimeout.tv_sec  = u32Time / 1000;
				    stTimeout.tv_usec = (u32Time % 1000) * 1000;
				    if(setsockopt(s32Socket, SOL_SOCKET, SO_RCVTIMEO, &stTimeout, sizeof(struct timeval)) < 0)
				    {
				        close(s32Socket);
				        fclose(pFile);
				        PRINT("socket error: %s\n", strerror(errno));
				        return MY_ERR(_Err_SYS + errno);
				    }

				    if(setsockopt(s32Socket, SOL_SOCKET, SO_SNDTIMEO, &stTimeout, sizeof(struct timeval)) < 0)
				    {
				        close(s32Socket);
				        fclose(pFile);
				        PRINT("socket error: %s\n", strerror(errno));
				        return MY_ERR(_Err_SYS + errno);
				    }

					sendto(s32Socket, c8Buf, u32Lenght, MSG_NOSIGNAL,
							(struct sockaddr *)(&stServerAddr), sizeof(struct sockaddr));

					{
						socklen_t s32Len = sizeof(struct sockaddr_in);
						s32Result = recvfrom(s32Socket, c8ReadBuf, 512,
												0, (struct sockaddr *)(&stAddr), &s32Len);
						if (s32Result <= 0 || (s32Len != sizeof (struct sockaddr_in)))
						{
							fclose(pFile);
							close(s32Socket);
							s32Result = MY_ERR(_Err_SYS + errno);
							PRINT("recvfrom error: %s\n", strerror(errno));
							return s32Result;
						}
						if(memcmp(&stServerAddr, &stAddr, sizeof(struct sockaddr_in)) != 0)
						{
							fclose(pFile);
							close(s32Socket);
							s32Result = MY_ERR(_Err_SYS + errno);
							PRINT("unknown server error: %s\n", strerror(errno));
							return s32Result;
						}
#if 0
						{
							uint32_t i;
							for (i = 0; i < s32Result; i++)
							{
								if ((i & 0x0F) == 0)
								{
									printf("\n");
								}
								printf("%02hhX ", c8ReadBuf[i]);
							}
							printf("\n");
						}
#endif
						PRINT("begin to resolve the DNS answer\n");
						s32Result = ResolveDNSAnswer(c8Buf, u32Lenght, c8ReadBuf, (uint32_t)s32Result, c8IPV4Addr, pInternetAddr);
						if (s32Result == 0)
						{
							fclose(pFile);
							close(s32Socket);
							return 0;
						}
						close(s32Socket);
					}
				}
			}
			fclose(pFile);
		}while(0);
		return MY_ERR(_Err_Common);
	}
}


/*
 * 函数名      : TimeGetTime
 * 功能        : 得到当前系统时间 (MS级)
 * 参数        : 无
 * 返回值      : 当前系统时间ms
 * 作者        : 许龙杰
 */
uint64_t TimeGetTime(void)
{
    struct timeval stTime;

    gettimeofday(&stTime, NULL);

    uint64_t u64Time;
    u64Time = stTime.tv_sec;
    u64Time *= 1000; /* ms */
    u64Time += (stTime.tv_usec / 1000);
    return u64Time;
}

/*
 * 函数名      : TimeGetSetupTime
 * 功能        : 得到当前系统启动时间 (MS级)
 * 参数        : 无
 * 返回值      : 当前系统启动了多长时间
 * 作者        : 许龙杰
 */
uint64_t TimeGetSetupTime(void)
{
    uint64_t u64Time;
	struct timespec stTime;

	clock_gettime(CLOCK_MONOTONIC, &stTime);
    u64Time = stTime.tv_sec;
    u64Time *= 1000; /* ms */
    u64Time += (stTime.tv_nsec / 1000000);
    return u64Time;
}



#if 0

void CreateCRC32Table(void)
{
	FILE *pFile = fopen("CRC32_table", "wb+");
	int32_t i, j;
	uint32_t u32CRC32;
	fprintf(pFile, "static uint32_t s_u32CRC32Table[] = {\n");

	for (i = 0; i < 256; i++)
	{
		u32CRC32 = i;
		for (j = 0; j < 8; j++)
		{
			if ((u32CRC32 & 0x01) != 0)
			{
				u32CRC32 = (u32CRC32 >> 1) ^ 0xEDB88320;
			}
			else
			{
				u32CRC32 >>= 1;
			}
		}
		if ((i & 0x07) == 0)
		{
			fprintf(pFile, "\t");
		}
		fprintf(pFile, "0x%08X, ", u32CRC32);
		if ((i & 0x07) == 0x07)
		{
			fprintf(pFile, "\n");
		}
	}

	fprintf(pFile, "};\n");
	fclose(pFile);
}

#endif

const uint32_t c_u32CRC32Table[] = {
	0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
	0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
	0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
	0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
	0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
	0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
	0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
	0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
	0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
	0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
	0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
	0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
	0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
	0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
	0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
	0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
	0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
	0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
	0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
	0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
	0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
	0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
	0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
	0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
	0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
	0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
	0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
	0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
	0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
	0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
	0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
	0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D,
};

/*
 * 函数名      : CRC32Buf
 * 功能        : 计算一段缓存的CRC32值
 * 参数        : pBuf [in] (uint8_t *)缓存指针
 *             : u32Length [in] (uint32_t)缓存的大小
 * 返回值      : uint32_t 类型, 表示CRC32值
 * 作者        : 许龙杰
 */
uint32_t CRC32Buf(uint8_t *pBuf, uint32_t u32Length)
{
	uint32_t u32CRC32 = ~0, i;

	if ((pBuf == NULL) || (u32Length == 0))
	{
		return u32CRC32;
	}

	for (i = 0; i < u32Length; ++i)
	{
		u32CRC32 = c_u32CRC32Table[((u32CRC32 & 0xFF) ^ pBuf[i] )] ^ (u32CRC32 >> 8);
	}
	u32CRC32 = ~u32CRC32;
	return u32CRC32;
}

/*
 * 函数名      : CRC32Buf
 * 功能        : 计算一个文件的CRC32值
 * 参数        : pName [in] (uint8_t *)文件名
 *             : pCRC32 [in/out] (uint32_t *)正确的话, 在指针中保存CRC值
 * 返回值      : 计算正确返回0, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t CRC32File(const char *pName, uint32_t *pCRC32)
{
	int32_t s32FD, s32Read;
	uint8_t u8Buf[512];
	uint32_t u32CRC32 = ~0;

	if ((pName == NULL) || (pCRC32 == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}

	s32FD = open(pName, O_RDONLY);
	if (s32FD < 0)
	{
		return MY_ERR(_Err_SYS + errno);
	}

	while ((s32Read = read(s32FD, u8Buf, 512)) > 0)
	{
		int32_t i;
		for (i = 0; i < s32Read; i++)
		{
			u32CRC32 = c_u32CRC32Table[((u32CRC32 & 0xFF) ^ u8Buf[i] )] ^ (u32CRC32 >> 8);
		}
	}
	close(s32FD);
	if (s32Read < 0)
	{
		return MY_ERR(_Err_SYS + errno);
	}

	*pCRC32 = ~u32CRC32;

	return 0;
}

const uint8_t c_u8CRCHigh[] =
{
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
	0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40
};

const uint8_t c_u8CRCLow[] =
{
	0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7,
	0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E,
	0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9,
	0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC,
	0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
	0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32,
	0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D,
	0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A, 0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38,
	0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF,
	0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
	0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1,
	0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4,
	0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB,
	0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA,
	0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
	0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0,
	0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97,
	0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C, 0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E,
	0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89,
	0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
	0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83,
	0x41, 0x81, 0x80, 0x40
};

/*
 * 函数名      : CRC16
 * 功能        : 计算一段缓存的CRC16值
 * 参数        : pBuf [in] (uint8_t *)缓存指针
 *             : u32Length [in] (uint32_t)缓存的大小
 * 返回值      : uint16_t 类型, 表示CRC16值
 * 作者        : 许龙杰
 */
uint16_t CRC16(const uint8_t *pFrame, uint16_t u16Len)
{
	uint8_t u8CRCHigh = 0xFF;
	uint8_t u8CRCLow = 0xFF;
	int32_t u32Index;
	if (pFrame == NULL)
	{
		return ~0;
	}

	while (u16Len--)
	{
		u32Index = u8CRCLow ^ (*( pFrame++ ));
		u8CRCLow = ( uint8_t )( u8CRCHigh ^ c_u8CRCHigh[u32Index] );
		u8CRCHigh = c_u8CRCLow[u32Index];
	}
	return (uint16_t)(((uint16_t)u8CRCHigh) << 8 | u8CRCLow );
}



/*
 * 函数名      : RunAProcess
 * 功能        : 运行一个进程
 * 参数        : pName [in] (uint8_t *)进程的名字(完整路径)
 *             : pArgv [in] (const char *[])传递给进程的参数数组, 没有的话传入NULL,
 *               有的话数组必须以NULL为最后一个成员
 * 返回值      : 计算正确返回非负值(子进程号), 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t RunAProcess(const char *pName, const char *pArgv[])
{
	/* fork and run it */
	pid_t s32Pid;
	if (pName == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	s32Pid = fork();
	if (s32Pid < 0)	/* error happened */
	{
		PRINT("fork %s error %d\n", pName, errno);
		PrintLog("fork %s error %d\n", pName, errno);
		s32Pid = MY_ERR(_Err_SYS + errno);
	}
	else if (s32Pid == 0) /* child process */
	{
		char c8Name[64], *pTmp, **p2Argv;
		uint32_t u32Cnt = 0;
		pTmp = strrchr(pName, '/');
		if (pTmp == NULL)
		{
			pTmp = (char *)pName;
		}
		else
		{
			pTmp += 1;
		}
		strncpy(c8Name, pTmp, 63);
		if (pArgv != NULL)
		{
			while (pArgv[u32Cnt] != NULL)
			{
				u32Cnt++;
			}
		}
		u32Cnt += 2;
		p2Argv = malloc(sizeof(char *) * u32Cnt);
		p2Argv[0] = c8Name;
		if (pArgv != NULL)
		{
			memcpy(p2Argv + 1, pArgv, sizeof(char *) * (u32Cnt - 2));
		}
		p2Argv[u32Cnt - 1] = NULL;
		if (execv(pName, p2Argv) < 0)
		{
			PRINT("execv %s error %d\n", pName, errno);
			PrintLog("execv %s error %d\n", pName, errno);
			free(p2Argv);
			exit(-1);
		}
		else
		{
			PRINT("execv %s OK!n", pName);
		}
	}
	return s32Pid;
}

/*
 * 函数名      : GetProcessNameFromPID
 * 功能        : 通过进程号得到进程名字
 * 参数        : pNameSave [out]: (char * 类型) 将得到的名字保存到的位置
 *             : u32Size [in]: (uint32_t类型) 指示pNameSave的长度
 *             : s32ID [in]: (int32_t类型) 进程号
 * 返回值      : (int32_t类型) 0表示成功, 否则错误
 * 作者        : 许龙杰
 */
int32_t GetProcessNameFromPID(char *pNameSave, uint32_t u32Size, int32_t s32ID)
{
	char c8ProcPath[32];
	char c8ProcessName[_POSIX_PATH_MAX];
	int32_t s32Length;
	char *pTmp;
	snprintf(c8ProcPath, 32, "/proc/%d/exe", s32ID);

	s32Length = readlink(c8ProcPath, c8ProcessName, _POSIX_PATH_MAX);
	if (s32Length < 0)
	{
		return MY_ERR(_Err_SYS +errno);
	}
	c8ProcessName[s32Length] = 0;
	pTmp = strrchr(c8ProcessName, '/');
	pTmp += 1;
	s32Length = strlen(pTmp) + 1;
	if (u32Size < ((uint32_t) s32Length))
	{
		return MY_ERR(_Err_InvalidParam);
	}
	strcpy(pNameSave, pTmp);
	return 0;
}


static int32_t MCSCallBack(uint32_t u32CmdNum, uint32_t u32CmdCnt, uint32_t u32CmdSize,
        const char *pCmdData, void *pContext)
{
	StInOut *pBuf = (StInOut *)pContext;
	memcpy(pBuf->pData, pCmdData, pBuf->u32Size);
	return 0;
}

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
		uint32_t u32SendDataLength, void *pReceiveBuf, uint32_t u32BufLength)
{
	int32_t s32Err = 0;
	int32_t s32Client = -1;
	if (pServer == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}

	s32Client = ClientConnect(pServer);
	if (s32Client < 0)
	{
		return s32Client;
	}

	s32Err = MCSSyncSend(s32Client, 1000, u32CmdNum, u32SendDataLength, pSendData);
	if (s32Err < 0)
	{
		close(s32Client);
		return s32Err;
	}

	if (pReceiveBuf != NULL)
	{
		uint32_t u32Size = 0;
		void *pMCS = MCSSyncReceive(s32Client, true, 1000, &u32Size, &s32Err);
		if (pMCS != NULL)
		{
			StInOut stRcvBuf = {u32BufLength, pReceiveBuf};
			s32Err = MCSResolve((const char *)pMCS, u32Size, MCSCallBack, (void *)(&stRcvBuf));
			MCSSyncFree(pMCS);
		}
	}
	close(s32Client);

	return s32Err;
}


/*
 * 函数名      : GetNetLinkStatus
 * 功能        : 检测有线网卡的网线是否连接
 * 参数        : pInterfaceName [in]: (const char * 类型) 网卡的名字
 * 返回值      : (int32_t类型) 0表示已经连接, 否则表示没有连接或者错误
 * 作者        : 许龙杰
 */
int32_t GetNetLinkStatus(const char *pInterfaceName)
{
	int32_t s32Sock, s32Err = 0;
	struct ifreq stIFR;

	s32Sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (s32Sock < 0)
	{
		return MY_ERR(_Err_SYS + errno);
	}

	memset(&stIFR, 0, sizeof(struct ifreq));

	strncpy(stIFR.ifr_name, pInterfaceName, sizeof(stIFR.ifr_name));

	s32Err = ioctl(s32Sock, SIOCGIFFLAGS, &stIFR);
	close(s32Sock);
	if (s32Err < 0)
	{
		return MY_ERR(_Err_SYS + errno);
	}
	if ((stIFR.ifr_flags & IFF_RUNNING) != 0)
	{
		return 0;
	}

	return MY_ERR(_Err_Common);
}


