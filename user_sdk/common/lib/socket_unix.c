/****************************************************************************
 * Copyright(c), 2001-2060, ******************************* 版权所有
 ****************************************************************************
 * 文件名称             : server_unix.c
 * 版本                 : 0.0.1
 * 作者                 : 许龙杰
 * 创建日期             : 2014年4月18日
 * 描述                 :
 ****************************************************************************/
#include "common.h"
#include "common_define.h"
/*
 * 函数名      : ServerListen
 * 功能        : 监听一个UNIX域套接字
 * 参数        : pName[in] (const char * 类型): 不为NULL时, 指定关联名字
 * 返回值      : int32_t 型数据, 正数返回socket, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t ServerListen(const char *pName)
{
    int32_t s32Fd, s32Rval;
    uint32_t u32Len;
    struct sockaddr_un stUn;
    if (pName == NULL)
    {
    	return MY_ERR(_Err_InvalidParam);
    }
    /* 创建UNIX域套接字 */
    if ((s32Fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
        PRINT("Create a socket error: %s", strerror(errno));
        return MY_ERR(_Err_SYS + errno);
    }
    unlink(pName);

    /* 填写套接字结构体 */
    memset(&stUn, 0, sizeof(stUn));
    stUn.sun_family = AF_UNIX;
    strcpy(stUn.sun_path, pName);
    u32Len = offsetof(struct sockaddr_un, sun_path)+ strlen(pName);
    /* 绑定关联文件到套接字 */
    if (bind(s32Fd, (struct sockaddr*) (&stUn), u32Len) < 0)
    {
        PRINT("Bind the socket error: %s\n", strerror(errno));
        s32Rval = MY_ERR(_Err_SYS + errno);
        goto err;
    }
    if (listen(s32Fd, MAX_LISTEN_QUEUE) < 0)
    {
        PRINT("Listen the socket error: %s\n", strerror(errno));
        s32Rval = MY_ERR(_Err_SYS + errno);
        goto err;
    }
    return s32Fd;
err:
    close(s32Fd);
    return s32Rval;
}

/*
 * 函数名      : ServerAccept
 * 功能        : 获取要连接UNIX域套接字
 * 参数        : s32ListenFd[in] (int32_t类型): ServerListen的返回值
 * 返回值      : int32_t 型数据, 正数失败, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t ServerAccept(int32_t s32ListenFd)
{
    int32_t s32ClientFd;
    socklen_t u32Len;
    struct sockaddr_un stUn;
    u32Len = sizeof(stUn);
    if ((s32ClientFd = accept(s32ListenFd, (struct sockaddr *) (&stUn), &u32Len)) < 0)
    {
        PRINT("Accept the socket error: %s\n", strerror(errno));
        return MY_ERR(_Err_SYS + errno);
    }
    return s32ClientFd;
}

/*
 * 函数名      : ServerRemove
 * 功能        : 删除UNIX域套接字
 * 参数        : s32Socket[in] (int32_t 类型): ServerListen的返回值
 *             : pName[in] (const char * 类型): 服务器的名字
 * 返回值      : 无
 * 作者        : 许龙杰
 */
void ServerRemove(int32_t s32Socket, const char *pName)
{
    char c8Str[_POSIX_PATH_MAX] = {0};
    if (s32Socket >= 0)
    {
        close(s32Socket);
    }
    if (pName == NULL)
    {
        return;
    }
    strcat(c8Str, "rm -rf ");
    strcat(c8Str, pName);
    system(c8Str);

}


/*
 * 函数名      : ClientConnect
 * 功能        : 连接到log server
 * 参数        : pName[in] (const char * 类型): 不为NULL时, 指定关联名字
 * 返回值      : int32_t 型数据, 正数返回socket, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t ClientConnect(const char *pName)
{
	int32_t s32Fd, s32Len, s32Rval;
	struct sockaddr_un stUn;
	if (pName == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}

	/* 创建UNIX域套接字 */
	if ((s32Fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		return MY_ERR(_Err_SYS + errno);
	}
	memset(&stUn, 0, sizeof(stUn));
	stUn.sun_family = AF_UNIX;
	strcpy(stUn.sun_path, pName);
	s32Len = offsetof(struct sockaddr_un, sun_path) + strlen(pName);

	if (connect(s32Fd, (struct sockaddr*) (&stUn), s32Len) < 0)
	{
		s32Rval = MY_ERR(_Err_SYS + errno);
		goto err;
	}
	return s32Fd;
err:
	close(s32Fd);
	return s32Rval;
}

#define PROCESS_FD			"ProcessFD"
#define CONTROLLEN			CMSG_LEN(sizeof(int))

int32_t SendFd(int32_t s32Socket, int32_t s32Fd)
{

	if ((s32Fd < 0) || (s32Socket < 0))
	{
		return MY_ERR(_Err_InvalidParam);
	}
	else
	{
		struct iovec	stIOV[1];
		struct msghdr	stMsg;
		struct cmsghdr	*pCmptr;
		const char *pStr = PROCESS_FD;
		char c8Buf[CONTROLLEN] = {0};

		stIOV[0].iov_base = (void *)pStr;
		stIOV[0].iov_len  = sizeof(PROCESS_FD);
		stMsg.msg_iov     = stIOV;
		stMsg.msg_iovlen  = 1;
		stMsg.msg_name    = NULL;
		stMsg.msg_namelen = 0;

		pCmptr = (struct cmsghdr *)c8Buf;

		pCmptr->cmsg_level  = SOL_SOCKET;
		pCmptr->cmsg_type   = SCM_RIGHTS;
		pCmptr->cmsg_len    = CONTROLLEN;
		{
		int32_t *pTmp = (int32_t *)(CMSG_DATA(pCmptr));
		*pTmp = s32Fd;		/* the fd to pass */
		}

		stMsg.msg_control    = pCmptr;
		stMsg.msg_controllen = CONTROLLEN;
		if (sendmsg(s32Socket, &stMsg, MSG_NOSIGNAL) != sizeof(PROCESS_FD))
		{
			PRINT("sendmsg error: %s\n", strerror(errno));
			return MY_ERR(_Err_SYS + errno);
		}
	}
	return 0;
}


int32_t ReceiveFd(int32_t s32Socket)
{

	if (s32Socket < 0)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	else
	{
		struct iovec	stIOV[1];
		struct msghdr	stMsg;
		struct cmsghdr	*pCmptr;
		char c8Str[sizeof(PROCESS_FD)] = {0};
		char c8Buf[CONTROLLEN] = {0};

		stIOV[0].iov_base = (void *)c8Str;
		stIOV[0].iov_len  = sizeof(PROCESS_FD);
		stMsg.msg_iov     = stIOV;
		stMsg.msg_iovlen  = 1;
		stMsg.msg_name    = NULL;
		stMsg.msg_namelen = 0;

		pCmptr = (struct cmsghdr *)c8Buf;

		stMsg.msg_control    = pCmptr;
		stMsg.msg_controllen = CONTROLLEN;
		if (recvmsg(s32Socket, &stMsg, 0) < 0)
		{
			PRINT("recvmsg error: %s\n", strerror(errno));
			return MY_ERR(_Err_SYS + errno);
		}

		if (strcmp(c8Str, PROCESS_FD) != 0)
		{
			PRINT("recvmsg str error: %s\n", c8Str);
			return MY_ERR(_Err_CmdType);
		}
		else
		{
			int32_t *pTmp = (int32_t *)(CMSG_DATA(pCmptr));
			int32_t s32FD = *pTmp;
			lseek(s32FD, 0, SEEK_SET);
			return s32FD;
		}
	}
}
