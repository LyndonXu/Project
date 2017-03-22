/****************************************************************************
 * Copyright(c), 2001-2060, ******************************* 版权所有
 ****************************************************************************
 * 文件名称             : cloud.c
 * 版本                 : 0.0.1
 * 作者                 : 许龙杰
 * 创建日期             : 2014年8月6日
 * 描述                 :
 ****************************************************************************/
#include "common_define.h"
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ctype.h>

#include "../inc/common.h"
#include "openssl/crypto.h"
#include "openssl/ssl.h"
#include "openssl/err.h"
#include "openssl/rand.h"


#include "json/json.h"

#if defined USING_SELF_DNS_RESOLVE
#undef GetHostIPV4Addr
#define GetHostIPV4Addr(x, y, z)			GetHostIPV4AddrTimeout(x, 5000, y, z)
#endif


/*
 * 函数名      : CloudSendAndGetReturn
 * 功能        : 向云发送数据并得到返回，保存在pMap中
 * 参数        : pStat [in] (StCloudDomain * 类型): 云状态，详见定义
 *             : pSendInfo [in] (StSendInfo 类型): 发送信息，详见定义
 *             : pMap [out] (StMMap *): 保存返回内容，详见定义，使用过之后必须使用CloudMapRelease销毁
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t CloudSendAndGetReturn(StCloudDomain *pStat, StSendInfo *pSendInfo, StMMap *pMap)
{
	if ((pStat == NULL) || (pSendInfo == NULL) || (pMap == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}

	if (pStat->stStat.emStat != _Cloud_IsOnline)
	{
		return MY_ERR(_Err_Cloud_IsNotOnline);
	}

	{
	int32_t s32Err = 0;
	int32_t s32Socket = -1;
	struct sockaddr_in stServerAddr, stClientAddr;

	SSL *pSSL;
	SSL_CTX *pSSLCTX;
	int32_t s32SSL = -1;
	char c8Domain[64];
	c8Domain[0] = 0;
	if (pSendInfo->pSecondDomain != NULL)
	{
		sprintf(c8Domain, "%s.", pSendInfo->pSecondDomain);
	}
	strcat(c8Domain, pStat->c8Domain);
	PRINT("server domain: %s\n", c8Domain);

	bzero(&stServerAddr, sizeof(stServerAddr));
	stServerAddr.sin_family = AF_INET;
	{
	char c8IP[IPV4_ADDR_LENGTH];
	s32Err = GetHostIPV4Addr(c8Domain, c8IP, &(stServerAddr.sin_addr));
	}
	if (s32Err != 0)
	{
		/* get the IP address of the server */
		PRINT("GetHostIPV4Addr error\n");
		return s32Err;
	}
	stServerAddr.sin_port = htons(pStat->s32Port);	/* https */

	bzero(&stClientAddr, sizeof(stClientAddr));
	stClientAddr.sin_family = AF_INET;    /* Internet protocol */
	if (inet_aton(pStat->stStat.c8ClientIPV4, &stClientAddr.sin_addr) == 0)
	{
		PRINT("client IP address error!\n");
		return MY_ERR(_Err_SYS + errno);
	}
	stClientAddr.sin_port = htons(0);    /* the system will allocate a port number for it */

	if ((s32Socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		PRINT("socket error:%s\n", strerror(errno));
		return MY_ERR(_Err_SYS + errno);
	}

	if (bind(s32Socket, (struct sockaddr*) &stClientAddr, sizeof(struct sockaddr_in)) != 0)
	{
		PRINT("bind error:%s\n", strerror(errno));
		s32Err = MY_ERR(_Err_SYS + errno);
		goto err;
	}

	{
		int32_t s32Mode;

		/*First we make the socket nonblocking*/
		s32Mode = fcntl(s32Socket,F_GETFL,0);
		s32Mode |= O_NONBLOCK;
		if (fcntl(s32Socket, F_SETFL, s32Mode) == -1)
		{
			PRINT("bind error:%s\n", strerror(errno));
			s32Err = MY_ERR(_Err_SYS + errno);
			goto err;
		}
	}

	PRINT("begin connect\n");
	if (connect(s32Socket, (struct sockaddr *) (&stServerAddr), sizeof(struct sockaddr_in)) == -1)
	{
		if (errno != EINPROGRESS)
		{
			PRINT("connect error:%s\n", strerror(errno));
			s32Err = MY_ERR(_Err_SYS + errno);
			goto err;
		}
		else
		{
			struct timeval stTimeVal;
			fd_set stFdSet;
			stTimeVal.tv_sec = 10;
			stTimeVal.tv_usec = 0;
			FD_ZERO(&stFdSet);
			FD_SET(s32Socket, &stFdSet);
			s32Err = select(s32Socket + 1, NULL, &stFdSet, NULL, &stTimeVal);
			if (s32Err > 0)
			{
				socklen_t u32Len = sizeof(int32_t);
				/* for firewall */
				getsockopt(s32Socket, SOL_SOCKET, SO_ERROR, &s32Err, &u32Len);
				if (s32Err != 0)
				{
					PRINT("connect error:%s\n", strerror(errno));
					s32Err = MY_ERR(_Err_SYS + errno);
					goto err;
				}
				PRINT("connect ok via select\n");
			}
			else
			{
				PRINT("connect error:%s\n", strerror(errno));
				s32Err = MY_ERR(_Err_SYS + errno);
				goto err;
			}
		}
	}
	PRINT("connect ok\n");
	{
		int32_t s32Mode;

		/*First we make the socket nonblocking*/
		s32Mode = fcntl(s32Socket,F_GETFL,0);
		s32Mode &= (~O_NONBLOCK);
		if (fcntl(s32Socket, F_SETFL, s32Mode) == -1)
		{
			PRINT("bind error:%s\n", strerror(errno));
			s32Err = MY_ERR(_Err_SYS + errno);
			goto err;
		}
	}

	/* 设置套接字选项,接收和发送超时时间 */
	{
	struct timeval stTimeout = {30};
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
	}

	pSSLCTX = SSL_CTX_new(SSLv23_client_method());
	if (pSSLCTX == NULL)
	{
		PRINT("SSL_CTX_new error:%s\n", strerror(errno));
		s32Err = MY_ERR(_Err_SYS + errno);
		goto err;
	}

	pSSL = SSL_new(pSSLCTX);
	if (pSSL == NULL)
	{
		PRINT("SSL_new error:%s\n", strerror(errno));
		s32Err = MY_ERR(_Err_SYS + errno);
		goto err1;
	}

	/* link the socket to the SSL */
	s32SSL = SSL_set_fd(pSSL, s32Socket);
	if (s32SSL == 0)
	{
		ERR_print_errors_fp(stderr);
		s32Err = SSL_get_error(pSSL, s32SSL);
		PRINT("SSL_set_fd error %d\n", s32Err);
		s32Err = MY_ERR(_Err_SSL + s32Err);
		goto err2;
	}

	RAND_poll();
	while (RAND_status() == 0)
	{
		time_t s32Time = time(NULL);
		RAND_seed(&s32Time, sizeof(time_t));
	}

	s32SSL = SSL_connect(pSSL);
	if (s32SSL != 1)
	{
		s32Err = SSL_get_error(pSSL, s32SSL);
		ERR_print_errors_fp(stderr);
		PRINT("SSL_connect error %d\n", s32Err);
		s32Err = MY_ERR(_Err_SSL + s32Err);
		goto err2;
	}

	{
	char c8Request[1024];
	c8Request[0] = 0;
	if (pSendInfo->s32BodySize == -1)
	{
		if (pSendInfo->pSendBody == NULL)
		{
			pSendInfo->s32BodySize = 0;
		}
		else
		{
			pSendInfo->s32BodySize = strlen(pSendInfo->pSendBody);
		}
	}
	sprintf(c8Request,
			"%s /%s HTTP/1.1\r\n"
			"Accept: */*\r\n"
			/* "Accept-Language: zh-cn\r\n"
			"User-Agent: Mozilla/4.0 (compatible; MSIE 5.01; Windows NT 5.0)\r\n" maybe not useful */
			"Host: %s:%d\r\n"
			"Content-Length: %d\r\n"
			"Connection: Close\r\n\r\n",
			pSendInfo->boIsGet ? "GET" : "POST",
			pSendInfo->pFile, c8Domain, pStat->s32Port,
			pSendInfo->s32BodySize);
	PRINT("\n%s\n", c8Request);
	/* send */
	{
	int32_t s32Totalsend = 0;
	int32_t s32Size = strlen(c8Request);
	while (s32Totalsend < s32Size)
	{
		int32_t s32Send = SSL_write(pSSL, c8Request + s32Totalsend, s32Size - s32Totalsend);
		if (s32Send <= 0)
		{
			s32Err = SSL_get_error(pSSL, s32Send);
			PRINT("SSL_set_fd error %d\n", s32Err);
			s32Err = MY_ERR(_Err_SSL + s32Err);
			goto err3;
		}
		s32Totalsend += s32Send;
	}
	if (pSendInfo->pSendBody != NULL)
	{
		const char *pBody = pSendInfo->pSendBody;
		s32Size = pSendInfo->s32BodySize;
		if(s32Size < 0)
		{
			s32Size = strlen(pSendInfo->pSendBody);
		}
		s32Totalsend = 0;
		while (s32Totalsend < s32Size)
		{
			int32_t s32Send = SSL_write(pSSL, pBody + s32Totalsend, s32Size - s32Totalsend);
			if (s32Send <= 0)
			{
				s32Err = SSL_get_error(pSSL, s32Send);
				PRINT("SSL_set_fd error %d\n", s32Err);
				s32Err = MY_ERR(_Err_SSL + s32Err);
				goto err3;
			}
			s32Totalsend += s32Send;
		}
	}
	}
	/* receive */
#if 0
	FILE *pFile = fopen("test.html", "wb+");
	if (pFile == NULL)
	{
		PRINT("fopen error:%s\n", strerror(errno));
		s32Err = MY_ERR(_Err_SYS + errno);
		goto err3;
	}
	while(1)
	{
		int32_t s32RecvTmp = 0;
		char c8Buf[PAGE_SIZE];
		s32RecvTmp = SSL_read(pSSL, c8Buf, PAGE_SIZE);
		if (s32RecvTmp <= 0)
		{
			break;
		}
		fwrite(c8Buf, s32RecvTmp, 1, pFile);
		fflush(pFile);
	}
	fclose(pFile);
#else
	{
	FILE *pFile = tmpfile();
	int32_t s32Fd = fileno(pFile);
	int32_t s32RecvTotal = 0;
	void *pMapAddr = NULL;
	if (pFile == NULL)
	{
		PRINT("fopen error:%s\n", strerror(errno));
		s32Err = MY_ERR(_Err_SYS + errno);
		goto err3;
	}
	s32Err = 0;
	while(1)
	{
		int32_t s32RecvTmp = 0, s32Recv = 0;
		bool boIsFinished = false;

		s32Err = ftruncate(s32Fd, s32RecvTotal + PAGE_SIZE);
		if (s32Err != 0)
		{
			s32Err = MY_ERR(_Err_SYS + errno);
			goto err4;
		}

		if (pMapAddr != NULL)
		{
			munmap(pMapAddr, PAGE_SIZE);
		}
		pMapAddr = mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, s32Fd, s32RecvTotal);
		if (pMapAddr == NULL)
		{
			s32Err = MY_ERR(_Err_SYS + errno);
			goto err4;
		}
		PRINT("mmap ok, begin to read\n");
		while (s32Recv < PAGE_SIZE)
		{
			s32RecvTmp = SSL_read(pSSL, (void *)((uint32_t)pMapAddr + s32Recv), PAGE_SIZE - s32Recv);
			if (s32RecvTmp == 0)
			{
				boIsFinished = true;
				break;
			}
			else if (s32RecvTmp < 0)
			{
				boIsFinished = true;
				s32Err = SSL_get_error(pSSL, s32RecvTmp);
				PRINT("SSL_read error %d\n", s32Err);
				s32Err = MY_ERR(_Err_SSL + s32Err);
				goto err4;
			}
			s32Recv += s32RecvTmp;
		}
		s32RecvTotal += s32Recv;
		PRINT("read ok, total receive is %d\n", s32RecvTotal);
		if (boIsFinished)
		{
			break;
		}
	}

	if (s32RecvTotal == 0)
	{
		s32Err = MY_ERR(_Err_Cloud_Data);
		goto err4;
	}

	pMap->pFile = pFile;
	if (pMapAddr != NULL)
	{
		munmap(pMapAddr, PAGE_SIZE);
	}
	pMapAddr = mmap(0, s32RecvTotal, PROT_READ | PROT_WRITE, MAP_SHARED, s32Fd, 0);
	if (pMapAddr == NULL)
	{
		s32Err = MY_ERR(_Err_SYS + errno);
		goto err4;
	}
	pMap->pMap = pMapAddr;
	pMap->u32MapSize = s32RecvTotal;
	goto err3;
err4:
	if (pMapAddr != NULL)
	{
		munmap(pMapAddr, PAGE_SIZE);
	}

	fclose(pFile);
	}
#endif
	}

err3:
	SSL_shutdown(pSSL);
err2:
	SSL_free(pSSL);
err1:
	SSL_CTX_free(pSSLCTX);
	ERR_remove_state(0);
err:
	close(s32Socket);
	return s32Err;
	}

	return 0;
}



/*
 * 函数名      : CloudSendAndGetReturnNoSSL
 * 功能        : 向云发送数据并得到返回(不通过SSL)，保存在pMap中
 * 参数        : pStat [in] (StCloudDomain * 类型): 云状态，详见定义
 *             : pSendInfo [in] (StSendInfo 类型): 发送信息，详见定义
 *             : pMap [out] (StMMap *): 保存返回内容，详见定义，使用过之后必须使用CloudMapRelease销毁
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t CloudSendAndGetReturnNoSSL(StCloudDomain *pStat, StSendInfo *pSendInfo, StMMap *pMap)
{
	if ((pStat == NULL) || (pSendInfo == NULL) || (pMap == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}

	if (pStat->stStat.emStat != _Cloud_IsOnline)
	{
		return MY_ERR(_Err_Cloud_IsNotOnline);
	}

	{
	int32_t s32Err = 0;
	int32_t s32Socket = -1;
	struct sockaddr_in stServerAddr, stClientAddr;

	char c8Domain[64];
	c8Domain[0] = 0;
	if (pSendInfo->pSecondDomain != NULL)
	{
		sprintf(c8Domain, "%s.", pSendInfo->pSecondDomain);
	}
	strcat(c8Domain, pStat->c8Domain);
	PRINT("server domain: %s\n", c8Domain);

	bzero(&stServerAddr, sizeof(stServerAddr));
	stServerAddr.sin_family = AF_INET;
	s32Err = GetHostIPV4Addr(c8Domain, NULL, &(stServerAddr.sin_addr));
	if (s32Err != 0)
	{
		/* get the IP address of the server */
		PRINT("GetHostIPV4Addr error\n");
		return s32Err;
	}
	stServerAddr.sin_port = htons(pStat->s32Port);	/* https */

	bzero(&stClientAddr, sizeof(stClientAddr));
	stClientAddr.sin_family = AF_INET;    /* Internet protocol */
	if (inet_aton(pStat->stStat.c8ClientIPV4, &stClientAddr.sin_addr) == 0)
	{
		PRINT("client IP address error!\n");
		return MY_ERR(_Err_SYS + errno);
	}
	stClientAddr.sin_port = htons(0);    /* the system will allocate a port number for it */

	if ((s32Socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		PRINT("socket error:%s\n", strerror(errno));
		return MY_ERR(_Err_SYS + errno);
	}

	if (bind(s32Socket, (struct sockaddr*) &stClientAddr, sizeof(struct sockaddr_in)) != 0)
	{
		PRINT("bind error:%s\n", strerror(errno));
		s32Err = MY_ERR(_Err_SYS + errno);
		goto err;
	}

	{
		int32_t s32Mode;

		/*First we make the socket nonblocking*/
		s32Mode = fcntl(s32Socket,F_GETFL,0);
		s32Mode |= O_NONBLOCK;
		if (fcntl(s32Socket, F_SETFL, s32Mode) == -1)
		{
			PRINT("bind error:%s\n", strerror(errno));
			s32Err = MY_ERR(_Err_SYS + errno);
			goto err;
		}
	}

	PRINT("begin connect\n");
	if (connect(s32Socket, (struct sockaddr *) (&stServerAddr), sizeof(struct sockaddr_in)) == -1)
	{
		if (errno != EINPROGRESS)
		{
			PRINT("connect error:%s\n", strerror(errno));
			s32Err = MY_ERR(_Err_SYS + errno);
			goto err;
		}
		else
		{
			struct timeval stTimeVal;
			fd_set stFdSet;
			stTimeVal.tv_sec = 10;
			stTimeVal.tv_usec = 0;
			FD_ZERO(&stFdSet);
			FD_SET(s32Socket, &stFdSet);
			s32Err = select(s32Socket + 1, NULL, &stFdSet, NULL, &stTimeVal);
			if (s32Err > 0)
			{
				socklen_t u32Len = sizeof(int32_t);
				/* for firewall */
				getsockopt(s32Socket, SOL_SOCKET, SO_ERROR, &s32Err, &u32Len);
				if (s32Err != 0)
				{
					PRINT("connect error:%s\n", strerror(errno));
					s32Err = MY_ERR(_Err_SYS + errno);
					goto err;
				}
				PRINT("connect ok via select\n");
			}
			else
			{
				PRINT("connect error:%s\n", strerror(errno));
				s32Err = MY_ERR(_Err_SYS + errno);
				goto err;
			}
		}
	}
	PRINT("connect ok\n");
	{
	int32_t s32Mode;

	/*First we make the socket nonblocking*/
	s32Mode = fcntl(s32Socket,F_GETFL,0);
	s32Mode &= (~O_NONBLOCK);
	if (fcntl(s32Socket, F_SETFL, s32Mode) == -1)
	{
		PRINT("bind error:%s\n", strerror(errno));
		s32Err = MY_ERR(_Err_SYS + errno);
		goto err;
	}
	}
	/* 设置套接字选项,接收和发送超时时间 */
	{
	struct timeval stTimeout = {30};
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
	}



	{
	char c8Request[1024];
	c8Request[0] = 0;

	if (pSendInfo->s32BodySize == -1)
	{
		if (pSendInfo->pSendBody == NULL)
		{
			pSendInfo->s32BodySize = 0;
		}
		else
		{
			pSendInfo->s32BodySize = strlen(pSendInfo->pSendBody);
		}
	}

	sprintf(c8Request,
			"%s /%s HTTP/1.1\r\n"
			"Accept: */*\r\n"
			/* "Accept-Language: zh-cn\r\n"
			"User-Agent: Mozilla/4.0 (compatible; MSIE 5.01; Windows NT 5.0)\r\n" maybe not useful */
			"Host: %s:%d\r\n"
			"Content-Length: %d\r\n"
			"Connection: Close\r\n\r\n",
			pSendInfo->boIsGet ? "GET" : "POST",
			pSendInfo->pFile, c8Domain, pStat->s32Port,
			pSendInfo->s32BodySize);
	PRINT("%s", c8Request);
	/* send */
	{
	int32_t s32Totalsend = 0;
	int32_t s32Size = strlen(c8Request);
	while (s32Totalsend < s32Size)
	{
		int32_t s32Send = send(s32Socket, c8Request + s32Totalsend, s32Size - s32Totalsend, MSG_NOSIGNAL);
		if (s32Send < 0)
		{
			PRINT("send error %s\n", strerror(errno));
			s32Err = MY_ERR(_Err_SYS + errno);
			goto err;
		}
		s32Totalsend += s32Send;
	}
	if (pSendInfo->pSendBody != NULL)
	{
		const char *pBody = pSendInfo->pSendBody;
		s32Size = pSendInfo->s32BodySize;
		if(s32Size < 0)
		{
			s32Size = strlen(pSendInfo->pSendBody);
		}
		s32Totalsend = 0;
		while (s32Totalsend < s32Size)
		{
			int32_t s32Send = send(s32Socket, pBody + s32Totalsend, s32Size - s32Totalsend, MSG_NOSIGNAL);
			if (s32Send <= 0)
			{
				PRINT("send error %s\n", strerror(errno));
				s32Err = MY_ERR(_Err_SYS + errno);
				goto err;
			}
			s32Totalsend += s32Send;
		}
	}
	}
	PRINT("begin to receive\n"); /* receive */
	{
	FILE *pFile = tmpfile();
	int32_t s32Fd = fileno(pFile);
	int32_t s32RecvTotal = 0;
	void *pMapAddr = NULL;
	if (pFile == NULL)
	{
		PRINT("fopen error:%s\n", strerror(errno));
		s32Err = MY_ERR(_Err_SYS + errno);
		goto err;
	}
	while(1)
	{
		int32_t s32RecvTmp = 0, s32Recv = 0;
		bool boIsFinished = false;
		s32Err = ftruncate(s32Fd, s32RecvTotal + PAGE_SIZE);
		if (s32Err != 0)
		{
			s32Err = MY_ERR(_Err_SYS + errno);
			goto err4;
		}
		if (pMapAddr != NULL)
		{
			munmap(pMapAddr, PAGE_SIZE);
		}
		pMapAddr = mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, s32Fd, s32RecvTotal);
		if (pMapAddr == NULL)
		{
			s32Err = MY_ERR(_Err_SYS + errno);
			goto err4;
		}
		/* PRINT("mmap ok, begin to read\n");*/
		while (s32Recv < PAGE_SIZE)
		{
			s32RecvTmp = recv(s32Socket, (void *)((uint32_t)pMapAddr + s32Recv), PAGE_SIZE - s32Recv, 0);
			if (s32RecvTmp == 0)
			{
				boIsFinished = true;
				break;
			}
			else if (s32RecvTmp < 0)
			{
				boIsFinished = true;
				s32Err = MY_ERR(_Err_SYS + errno);
				break;
			}
			s32Recv += s32RecvTmp;
		}
		s32RecvTotal += s32Recv;
		/* PRINT("read ok, total receive is %d\n", s32RecvTotal); */
		if (boIsFinished)
		{
			break;
		}
	}
	pMap->pFile = pFile;
	if (pMapAddr != NULL)
	{
		munmap(pMapAddr, PAGE_SIZE);
	}
	pMapAddr = mmap(0, s32RecvTotal, PROT_READ | PROT_WRITE, MAP_SHARED, s32Fd, 0);
	if (pMapAddr == NULL)
	{
		s32Err = MY_ERR(_Err_SYS + errno);
		goto err4;
	}
	pMap->pMap = pMapAddr;
	pMap->u32MapSize = s32RecvTotal;
	goto err;
err4:
	if (pMapAddr != NULL)
	{
		munmap(pMapAddr, PAGE_SIZE);
	}

	fclose(pFile);
	}
	}

err:
	close(s32Socket);
	return s32Err;
	}
}
/*
 * 函数名      : CloudMapRelease
 * 功能        : 销毁CloudSendAndGetReturn的返回内容
 * 参数        : pMap [out] (StMMap *): 返回的内容，详见定义
 * 返回        : 无
 * 作者        : 许龙杰
 */
void CloudMapRelease(StMMap *pMap)
{
	if (pMap == NULL)
	{
		return;
	}
	if (pMap->pFile != NULL)
	{
		fclose(pMap->pFile);
	}
	if (pMap->pMap != NULL)
	{
		munmap(pMap->pMap, pMap->u32MapSize);
	}
}


/*
 * 函数名      : GetRegionOfGateway
 * 功能        : 得到Gateway的区域信息
 * 参数        : pRegion [out] (char * 类型): 成功保存区域字符串
 *             : uint32_t [in] (uint32_t 类型): pRegion指向字符串的长度
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t GetRegionOfGateway(char *pRegion, uint32_t u32Size)
{
	if ((pRegion == NULL) || (u32Size == 0))
	{
		return MY_ERR(_Err_InvalidParam);
	}
	else
	{
		json_object *pInfo, *pRegionObj;
		pInfo = json_object_from_file(GATEWAY_INFO_FILE);
		if (pInfo == NULL)
		{
			PRINT("json_object_from_file error\n");
			return MY_ERR(_Err_JSON);
		}
		pRegionObj = json_object_object_get(pInfo, "Ptr");
		if (pRegionObj == NULL)
		{
			json_object_put(pInfo);
			return MY_ERR(_Err_JSON);
		}

		pRegionObj = json_object_object_get(pRegionObj, "Region");
		if (pRegionObj == NULL)
		{
			json_object_put(pInfo);
			return MY_ERR(_Err_JSON);
		}
		strncpy(pRegion, json_object_get_string(pRegionObj), u32Size);
		json_object_put(pInfo);
		return 0;
	}
}

/*
 * 函数名      : GetServerDomainForGateway
 * 功能        : 得到服务器的域名信息
 * 参数        : pServerDomain [out] (char * 类型): 成功保存区域名符串
 *             : uint32_t [in] (uint32_t 类型): pServerDomain指向字符串的长度
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t GetServerDomainForGateway(char *pServerDomain, uint32_t u32Size)
{
	if ((pServerDomain == NULL) || (u32Size == 0))
	{
		return MY_ERR(_Err_InvalidParam);
	}
	else
	{
		json_object *pInfo, *pServerObj;
		pInfo = json_object_from_file(GATEWAY_INFO_FILE);
		if (pInfo == NULL)
		{
			PRINT("json_object_from_file error\n");
			return MY_ERR(_Err_JSON);
		}
		pServerObj = json_object_object_get(pInfo, "Ptr");
		if (pServerObj == NULL)
		{
			json_object_put(pInfo);
			return MY_ERR(_Err_JSON);
		}

		pServerObj = json_object_object_get(pServerObj, "ServerDomain");
		if (pServerObj == NULL)
		{
			json_object_put(pInfo);
			return MY_ERR(_Err_JSON);
		}
		strncpy(pServerDomain, json_object_get_string(pServerObj), u32Size);
		json_object_put(pInfo);
		return 0;
	}
}

/*
 * 函数名      : GetSNOfGateway
 * 功能        : 得到Gateway的区域信息
 * 参数        : pSN [out] (char * 类型): 成功保存序列号字符串
 *             : uint32_t [in] (uint32_t 类型): pRegion指向字符串的长度
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t GetSNOfGateway(char *pSN, uint32_t u32Size)
{
	if ((pSN == NULL) || (u32Size == 0))
	{
		return MY_ERR(_Err_InvalidParam);
	}
	else
	{
		json_object *pInfo, *pObj;
		pInfo = json_object_from_file(GATEWAY_INFO_FILE);
		if (pInfo == NULL)
		{
			PRINT("json_object_from_file error\n");
			return MY_ERR(_Err_JSON);
		}

		pObj = json_object_object_get(pInfo, "IDPS");
		if (pObj == NULL)
		{
			json_object_put(pInfo);
			return MY_ERR(_Err_JSON);
		}

		pObj = json_object_object_get(pObj, "SN");
		if (pObj == NULL)
		{
			json_object_put(pInfo);
			return MY_ERR(_Err_JSON);
		}

		strncpy(pSN, json_object_get_string(pObj), u32Size);
		json_object_put(pInfo);
		return 0;
	}
}


/* get http return code HTTP/1.1 200 OK ect. */
int32_t GetHttpReturnCode(StMMap *pMap)
{
	if (pMap != NULL)
	{
		if (pMap->pMap != NULL)
		{
			int32_t s32ReturnCode = -1;
			const char *pTmp = strchr(pMap->pMap, ' ');
			if (pTmp != NULL)
			{
				sscanf(pTmp + 1, "%d", &s32ReturnCode);
			}
			PRINT("HTTP return: %d\n", s32ReturnCode);
			return s32ReturnCode;
		}
	}
	return -1;
}

/* get the content of the return  */
const char *GetHttpBody(StMMap *pMap)
{
	if (pMap != NULL)
	{
		if (pMap->pMap != NULL)
		{
			const char *pTmp = strstr(pMap->pMap, "\r\n\r\n");
			if (pTmp == NULL)
			{
				return NULL;
			}
			return strchr(pTmp, '{');
		}
	}
	return NULL;
}

/* get the json of the return */
json_object *GetHttpJsonBody(StMMap *pMap, int32_t *pErr)
{
	json_object *pReturn = NULL, *pSon;
	int32_t s32Err;
	const char *pBody;
	PRINT("server return: \n%s\n", (char *)pMap->pMap);
	/* analysis the return text */
	s32Err = GetHttpReturnCode(pMap);
	if (s32Err != 200)
	{
		s32Err = MY_ERR(_Err_HTTP + s32Err);
		goto end;
	}
	pBody = GetHttpBody(pMap);
	if (pBody == NULL)
	{
		s32Err =MY_ERR(_Err_Cloud_Body);
		PRINT("cannot find char \'{\'\n");
		goto end;
	}
#if (0)
	{
		uint32_t u32Length = pBody -(char *)pMap->pMap;
		uint32_t i = 0;
		uint32_t j = 0;
		u32Length = pMap->u32MapSize - u32Length;
		PRINT("u32Length: %d\n", u32Length);
		while(i < (u32Length - 1))
		{
			if (pBody[i] == '\\')
			{
				pBody[j++] = pBody[i + 1];
				i += 2;
			}
			else
			{
				pBody[j++] = pBody[i];
				i += 1;
			}
		}
		pBody[j++] = pBody[i];	/* last char */
		if (j != u32Length)
		{
			pBody[j] = 0;
		}
		PRINT("after remove '\\': %s\n", pBody);
	}
#endif
	pReturn = json_tokener_parse(pBody);
	if (pReturn == NULL)
	{
		s32Err = MY_ERR(_Err_Cloud_JSON);
		PRINT("json_tokener_parse error\n");
		goto end;
	}

	/* PRINT("parse json: %s\n", json_object_get_string(pReturn)); */
	pSon = json_object_object_get(pReturn, "Result");
	if (pSon == NULL)
	{
		s32Err = MY_ERR(_Err_Cloud_JSON);
		json_object_put(pReturn);
		PRINT("json cannot get key Result\n");
		pReturn = NULL;
		goto end;
	}
	/* result is ok */
	if (json_object_is_type(pSon, json_type_int))
	{
		s32Err = json_object_get_int(pSon);
		if (s32Err != 1)
		{
			pSon = json_object_object_get(pReturn, "ResultMessage");
			PRINT("ResultMessage: %s\n", json_object_to_json_string(pSon));
			s32Err = MY_ERR(_Err_Cloud_Result + s32Err);
			json_object_put(pReturn);
			pReturn = NULL;
			goto end;
		}
	}
	else
	{
		PRINT("key Result's type is not int\n");

		s32Err = MY_ERR(_Err_Cloud_JSON);
		json_object_put(pReturn);
		pReturn = NULL;
		goto end;
	}
	pSon = json_object_object_get(pReturn, "ReturnValue");
	if (pSon == NULL)
	{
		s32Err = MY_ERR(_Err_Cloud_JSON);
		json_object_put(pReturn);
		pReturn = NULL;
		goto end;
	}
	else
	{
		s32Err = 0;
		json_object_get(pSon);
		json_object_put(pReturn);
		return pSon;
	}

end:
	*pErr = s32Err;
	return pReturn;
}
/*
 * resolve the command of 0xF5 from the cloud
 * 1:
 */
int32_t ResolveCmd_0xF5(json_object *pJsonObj)
{
	char c8Buf[16];
	int s32Rslt = 0xFD;
	char c8Tmp = 0;
	sprintf(c8Buf, "%s", json_object_get_string(pJsonObj));
	sscanf(c8Buf, "%02hhX", &c8Tmp);
	s32Rslt = c8Tmp;
	PRINT("F5 return %s\n", c8Buf);
	return (s32Rslt & 0xFF);
}

/* resolve the command of 0xFA from the cloud, the c8R will save the cloud R */
int32_t ResolveCmd_0xFA(json_object *pJsonObj, char c8R[RAND_NUM_CNT])
{
	/* command data */
	{
	int32_t s32Len;
	s32Len = json_object_get_string_len(pJsonObj);
	if (s32Len != (RAND_NUM_CNT * 2))
	{
		PRINT("command FA R NULL\n");
		return MY_ERR(_Err_Cloud_Data);
	}
	}

	{
	int32_t i;
	char c8Buf[4] = {0};
	//char c8RServer[36];
	//snprintf(c8RServer, 36,"%s", json_object_to_json_string(pJsonObj));
	const char *pTmp = json_object_get_string(pJsonObj);
	for (i = 0; i < RAND_NUM_CNT; i++)
	{
		c8Buf[0] = pTmp[i * 2];		/* \" */
		c8Buf[1] = pTmp[i * 2 + 1];
		sscanf(c8Buf, "%02hhX", c8R + i);
	}
	}
	return 0;
}

/* resolve the command of 0xFB from the cloud, the c8ClientR is R1 of client */
int32_t ResolveCmd_0xFB(json_object *pJsonObj, char c8ClientR[RAND_NUM_CNT], const char c8Key[XXTEA_KEY_CNT_CHAR])
{
	/* command data */
	{
	int32_t s32Len;
	s32Len = json_object_get_string_len(pJsonObj);
	if (s32Len != (RAND_NUM_CNT * 2))
	{
		PRINT("command FB RN1's coding NULL\n");
		return MY_ERR(_Err_Cloud_Authentication);
	}
	}

	{
	int32_t i;
	char c8Buf[4] = {0};
	//char c8RServer[36];
	//snprintf(c8RServer, 36,"%s", json_object_to_json_string(pJsonObj));
	const char *pTmp = json_object_get_string(pJsonObj);
	btea((int32_t *)c8ClientR, RAND_NUM_CNT / sizeof(int32_t), (int32_t *)c8Key);

	for (i = 0; i < RAND_NUM_CNT; i++)
	{
		char c8R = 0;
		c8Buf[0] = pTmp[i * 2];		/* \" */
		c8Buf[1] = pTmp[i * 2 + 1];
		sscanf(c8Buf, "%02hhX", &c8R);
		if (c8R != c8ClientR[i])
		{
			PRINT("command FB RN1's coding error\n");
			return MY_ERR(_Err_Cloud_Authentication);
		}
	}
	}

	return 0;
}

json_object *GetIDPS(int32_t *pErr)
{
	json_object *pInfo, *pIDPS;
	PRINT(GATEWAY_INFO_FILE"\n");
	pInfo = json_object_from_file(GATEWAY_INFO_FILE);
	if (pInfo == NULL)
	{
		PRINT("json_object_from_file error\n");
		*pErr = MY_ERR(_Err_JSON);
		return NULL;
	}
	pIDPS = json_object_object_get(pInfo, "IDPS");
	if (pIDPS == NULL)
	{
		*pErr = MY_ERR(_Err_JSON);
		json_object_put(pInfo);
		return NULL;
	}
	json_object_get(pIDPS);
	json_object_put(pInfo);
	*pErr = 0;
	return pIDPS;
}
int32_t GetIDPSStruct(StPtlIDPS *pIDPS)
{
	json_object *pObj, *pObjStr;
	int32_t s32Err;
	const char *pTmp;
	if (pIDPS == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	pObj = GetIDPS(&s32Err);
	if (pObj == NULL)
	{
		return s32Err;
	}
	pObjStr = json_object_object_get(pObj, "Ptl");
	if (pObjStr == NULL)
	{
		s32Err = MY_ERR(_Err_JSON);
		goto end;
	}
	pTmp = json_object_get_string(pObjStr);
	if (pTmp == NULL)
	{
		goto end;
	}
	strncpy(pIDPS->c8Protocol, pTmp, 16);

	pObjStr = json_object_object_get(pObj, "Name");
	if (pObjStr == NULL)
	{
		s32Err = MY_ERR(_Err_JSON);
		goto end;
	}
	pTmp = json_object_get_string(pObjStr);
	if (pTmp == NULL)
	{
		goto end;
	}
	strncpy(pIDPS->c8Name, pTmp, 16);
	strncpy(pIDPS->c8ProductName, pTmp, 32);

	pObjStr = json_object_object_get(pObj, "MFR");
	if (pObjStr == NULL)
	{
		s32Err = MY_ERR(_Err_JSON);
		goto end;
	}
	pTmp = json_object_get_string(pObjStr);
	if (pTmp == NULL)
	{
		goto end;
	}
	strncpy(pIDPS->c8Manufacturer, pTmp, 16);

	pObjStr = json_object_object_get(pObj, "Model");
	if (pObjStr == NULL)
	{
		s32Err = MY_ERR(_Err_JSON);
		goto end;
	}
	pTmp = json_object_get_string(pObjStr);
	if (pTmp == NULL)
	{
		goto end;
	}
	strncpy(pIDPS->c8ModelNumber, pTmp, 16);

	pObjStr = json_object_object_get(pObj, "SN");
	if (pObjStr == NULL)
	{
		s32Err = MY_ERR(_Err_JSON);
		goto end;
	}
	pTmp = json_object_get_string(pObjStr);
	if (pTmp == NULL)
	{
		goto end;
	}
	strncpy(pIDPS->c8SN, pTmp, 16);

	pObjStr = json_object_object_get(pObj, "FVer");
	if (pObjStr == NULL)
	{
		s32Err = MY_ERR(_Err_JSON);
		goto end;
	}
	pTmp = json_object_get_string(pObjStr);
	if (pTmp == NULL)
	{
		goto end;
	}
	pIDPS->c8FirmwareVersion[0] = pTmp[0];
	pIDPS->c8FirmwareVersion[1] = pTmp[2];
	pIDPS->c8FirmwareVersion[2] = pTmp[4];

	pObjStr = json_object_object_get(pObj, "HVer");
	if (pObjStr == NULL)
	{
		s32Err = MY_ERR(_Err_JSON);
		goto end;
	}
	pTmp = json_object_get_string(pObjStr);
	if (pTmp == NULL)
	{
		goto end;
	}
	pIDPS->c8HardwareVersion[0] = pTmp[0];
	pIDPS->c8HardwareVersion[1] = pTmp[2];
	pIDPS->c8HardwareVersion[2] = pTmp[4];

end:
	json_object_put(pObj);
	return s32Err;
}
static uint32_t s_u32QueueNum = 0;
int32_t AuthCmdAddBaseInfo(json_object *pJsonObj)
{
	int32_t s32Err = 0;
	json_object_object_add(pJsonObj, "Sc", json_object_new_string(AUTHENTICATION_SC));
	json_object_object_add(pJsonObj, "Sv", json_object_new_string(AUTHENTICATION_SV));
	json_object_object_add(pJsonObj, "QueueNum", json_object_new_int(s_u32QueueNum++));
	json_object *pIDPS = GetIDPS(&s32Err);
	if (s32Err == 0)
	{
		json_object_object_add(pJsonObj, "IDPS", pIDPS);
	}
	return s32Err;
}

json_object *BuildupCmd_0xF5_A(char c8Cmd, int32_t *pErr)
{
	json_object * pJsonObj = json_object_new_object();
	if (pJsonObj == NULL)
	{
		*pErr = MY_ERR(_Err_JSON);
		return NULL;
	}

	*pErr = AuthCmdAddBaseInfo(pJsonObj);
	if (*pErr != 0)
	{
		json_object_put(pJsonObj);
		return NULL;
	}

	{
	char c8Tmp[8];
	sprintf(c8Tmp, "%02hhX", c8Cmd);
	json_object_object_add(pJsonObj, "Command", json_object_new_string(c8Tmp));
	}

	return pJsonObj;
}

#if 0
/* buildup the command of 0xFA, 0xFC, 0xFD for client */
json_object *BuildupCmd_0xFA_C_D(char c8Cmd, int32_t *pErr)
{
	json_object * pJsonObj = json_object_new_object();
	if (pJsonObj == NULL)
	{
		*pErr = MY_ERR(_Err_JSON);
		return NULL;
	}

	*pErr = AuthCmdAddBaseInfo(pJsonObj);
	if (*pErr != 0)
	{
		json_object_put(pJsonObj);
		return NULL;
	}

	{
	char c8Tmp[8];
	sprintf(c8Tmp, "0x%02hhX", c8Cmd);
	json_object_object_add(pJsonObj, "Command", json_object_new_string(c8Tmp));
	}

	return pJsonObj;
}
#endif

/* buildup the command of 0xFB for client */
json_object *BuildupCmd_0xFB(char c8CloudR[RAND_NUM_CNT], char c8ClientR[RAND_NUM_CNT],
		const char c8ID[PRODUCT_ID_CNT], const char c8Key[XXTEA_KEY_CNT_CHAR],
		int32_t *pErr)
{
	int32_t s32Err = 0;
	json_object *pJsonObj = json_object_new_object();
	if (pJsonObj == NULL)
	{
		PRINT("json_object_new_object");
		s32Err = MY_ERR(_Err_JSON);
		goto err;
	}

	*pErr = AuthCmdAddBaseInfo(pJsonObj);
	if (*pErr != 0)
	{
		json_object_put(pJsonObj);
		return NULL;
	}

	json_object_object_add(pJsonObj, "Command", json_object_new_string("FB"));

	btea((int32_t *)c8CloudR, RAND_NUM_CNT / sizeof(int32_t), (int32_t *)c8Key);
	{
	char c8CloudBuf[36], c8ClientBuf[36];
#if 0
	char c8ClientID[36];
	sprintf(c8ClientID + (i * 2), "%02hhX", c8ID[i]);
	json_object_object_add(pJsonObj, "GatewayID", json_object_new_string(c8ClientID));
#endif
	int32_t i;
	srand(time(NULL));
	for (i = 0; i < RAND_NUM_CNT; i++)
	{
		sprintf(c8CloudBuf + (i * 2), "%02hhX", c8CloudR[i]);
		c8ClientR[i] = rand();
		sprintf(c8ClientBuf + (i * 2), "%02hhX", c8ClientR[i]);
	}
	json_object_object_add(pJsonObj, "GatewayID", json_object_new_string_len(c8ID, PRODUCT_ID_CNT));
	json_object_object_add(pJsonObj, "RNCoded", json_object_new_string(c8CloudBuf));
	json_object_object_add(pJsonObj, "RN1", json_object_new_string(c8ClientBuf));
	}
	*pErr = s32Err;
	return pJsonObj;
err:
	if (pJsonObj != NULL)
	{
		json_object_put(pJsonObj);
	}
	*pErr = s32Err;
	return NULL;
}

#define TEST_CLOUD		1

typedef struct _tagStIDPS
{
	char c8ProtocolString[16];					/* com.jiuan.HG3 */
	char c8AccessoryName[16];					/* HG3 */
	char c8AccessoryFirmware[3];				/* 1.0.0 */
	char c8AccessoryHardware[3];				/* 1.0.1 */
	char c8AccessoryManufacturer[16];			/* HG3 */
	char c8AccessoryModelNumber[16];			/* HG3 12312 */
	char c8AccessorySerialNumber[16];			/* 128912387145897 */

}StIDPS;
/*
 * 函数名      : CloudSendIDPS
 * 功能        : 发送IDPS
 * 参数        : pStat [in] (StCloudStat * 类型): 云状态，详见定义
 *             : pIDPS [in] (StIDPS * 类型): 产品IDPS
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t CloudSendIDPS(StCloudStat *pStat, const char c8ID[PRODUCT_ID_CNT], const char c8Key[XXTEA_KEY_CNT_CHAR])
{
	return 0;
}


/*
 * 函数名      : CloudAuthentication
 * 功能        : 云认证
 * 参数        : pStat [in] (StCloudStat * 类型): 云状态，详见定义
 * 			   : boIsCoordination [in] (bool 类型): 是否通过协调服务器
 *             : c8ID [in] (const char * 类型): 产品ID
 *             : c8Key [in] (const char *): 产品KEY
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t CloudAuthentication(StCloudDomain *pStat, bool boIsCoordination,
		const char c8ID[PRODUCT_ID_CNT], const char c8Key[XXTEA_KEY_CNT_CHAR])
{
	int32_t s32Err = 0;
	json_object *pJsonObj = NULL;

	char c8CloudR[RAND_NUM_CNT];
	char c8ClientR[RAND_NUM_CNT];
	char c8Char[PAGE_SIZE];

	if ((pStat == NULL) || (c8ID == NULL) || (c8Key == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}
	pJsonObj = BuildupCmd_0xF5_A(0xF5, &s32Err);
	if (pJsonObj == NULL)
	{
		return MY_ERR(_Err_JSON);
	}

	sprintf(c8Char, "content=%s", json_object_to_json_string(pJsonObj));
	json_object_put(pJsonObj);
	pJsonObj = NULL;
	PRINT("%s\n", c8Char);

	{
#if TEST_CLOUD
	StSendInfo stSendInfo = {false, NULL, AUTHENTICATION_ADDR, NULL, -1};

	if (!boIsCoordination)
	{
		stSendInfo.pSecondDomain = AUTHENTICATION_SECOND_DOMAIN;
	}

	{
	StMMap stMap = {NULL};
	stSendInfo.pSendBody = c8Char;
	stSendInfo.s32BodySize = -1;
	s32Err = CloudSendAndGetReturn(pStat, &stSendInfo, &stMap);
	if (s32Err != 0)
	{
		return s32Err;
	}
	/* analysis the return text */
	pJsonObj = GetHttpJsonBody(&stMap, &s32Err);
	CloudMapRelease(&stMap);
	if (pJsonObj == NULL)
	{
		return s32Err;
	}
	s32Err = ResolveCmd_0xF5(pJsonObj);
	if (s32Err == 0xF0)
	{
		s32Err = 0;
		goto end;
	}
	else if (s32Err == 0xFD)
	{
		s32Err = MY_ERR(_Err_IDPS);
		goto end;
	}
	else if (s32Err != 0xF5)
	{
		s32Err = MY_ERR(_Err_Cloud_Data);
		goto end;
	}
	json_object_put(pJsonObj);
	pJsonObj = NULL;

	}
#endif

	pJsonObj = BuildupCmd_0xF5_A(0xFA, &s32Err);
	if (pJsonObj == NULL)
	{
		return MY_ERR(_Err_JSON);
	}

	sprintf(c8Char, "content=%s", json_object_to_json_string(pJsonObj));
	json_object_put(pJsonObj);
	pJsonObj = NULL;
	PRINT("%s\n", c8Char);
#if TEST_CLOUD
	{
	StMMap stMap = {NULL};

	stSendInfo.pSendBody = c8Char;
	stSendInfo.s32BodySize = -1;
	s32Err = CloudSendAndGetReturn(pStat, &stSendInfo, &stMap);
	if (s32Err != 0)
	{
		return s32Err;
	}
	/* analysis the return text */
	pJsonObj = GetHttpJsonBody(&stMap, &s32Err);
	CloudMapRelease(&stMap);
	if (pJsonObj == NULL)
	{
		return s32Err;
	}
	s32Err = ResolveCmd_0xFA(pJsonObj, c8CloudR);
	if (s32Err != 0)
	{
		goto end;
	}
	json_object_put(pJsonObj);
	pJsonObj = NULL;

	}
#endif


	pJsonObj = BuildupCmd_0xFB(c8CloudR, c8ClientR, c8ID, c8Key, &s32Err);
	if (pJsonObj == NULL)
	{
		PRINT("json_object_new_object");
		return s32Err;
	}
	sprintf(c8Char, "content=%s", json_object_to_json_string(pJsonObj));
	json_object_put(pJsonObj);
	pJsonObj = NULL;
	PRINT("%s\n", c8Char);

#if TEST_CLOUD
	{
	StMMap stMap = {NULL};
	stSendInfo.pSendBody = c8Char;
	stSendInfo.s32BodySize = -1;
	s32Err = CloudSendAndGetReturn(pStat, &stSendInfo, &stMap);
	if (s32Err != 0)
	{
		return s32Err;
	}
	/* analysis the return text */
	pJsonObj = GetHttpJsonBody(&stMap, &s32Err);
	CloudMapRelease(&stMap);
	if (pJsonObj == NULL)
	{
		return s32Err;
	}

	s32Err = ResolveCmd_0xFB(pJsonObj, c8ClientR, c8Key);
	json_object_put(pJsonObj);
	pJsonObj = NULL;
	}
#endif
	}

#if TEST_CLOUD
end:
#endif

	if (pJsonObj != NULL)
	{
		json_object_put(pJsonObj);
	}
	return s32Err;
}

int32_t GetSelfRegionCmdAddBaseInfo(json_object *pJsonObj)
{
	int32_t s32Err = 0;
	json_object_object_add(pJsonObj, "Sc", json_object_new_string(GET_SELF_REGION_SC));
	json_object_object_add(pJsonObj, "Sv", json_object_new_string(GET_SELF_REGION_SV));
	json_object_object_add(pJsonObj, "QueueNum", json_object_new_int(s_u32QueueNum++));
	json_object *pIDPS = GetIDPS(&s32Err);
	if (s32Err == 0)
	{
		json_object_object_add(pJsonObj, "IDPS", pIDPS);
	}
	return s32Err;
}


json_object *BuildupCmdGetSelfRegion(int32_t *pErr)
{
	json_object * pJsonObj = json_object_new_object();
	if (pJsonObj == NULL)
	{
		*pErr = MY_ERR(_Err_JSON);
		return NULL;
	}

	*pErr = GetSelfRegionCmdAddBaseInfo(pJsonObj);
	if (*pErr != 0)
	{
		json_object_put(pJsonObj);
		return NULL;
	}
	return pJsonObj;
}

int32_t ResolveCmdGetSelfRegion(json_object *pJsonObj, char *pRegion, uint32_t u32Size)
{
	const char *pTmp = json_object_get_string(pJsonObj);
	if (pTmp == NULL)
	{
		return MY_ERR(_Err_Cloud_JSON);
	}
	pRegion[u32Size - 1] = 0;
	strncpy(pRegion, pTmp, u32Size - 1);
	return 0;
}
/*
 * 函数名      : CloudGetSelfRegion
 * 功能        : 通过云得到自身的区域
 * 参数        : pStat [in] (StCloudStat * 类型): 云状态，详见定义
 *             : pRegion [in/out] (char * 类型): 指向输出buffer, 正确时, 保存区域
 *             : u32Size [in] (uint32_t): pRegion buffer 的大小
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t CloudGetSelfRegion(StCloudDomain *pStat, char *pRegion, uint32_t u32Size)
{
	int32_t s32Err = 0;
	json_object *pJsonObj = NULL;
	char c8Char[PAGE_SIZE];

	if ((pStat == NULL) || (pRegion == NULL) || (u32Size == 0))
	{
		return MY_ERR(_Err_InvalidParam);
	}
	pJsonObj = BuildupCmdGetSelfRegion(&s32Err);
	if (pJsonObj == NULL)
	{
		return MY_ERR(_Err_JSON);
	}

	sprintf(c8Char, "content=%s", json_object_to_json_string(pJsonObj));
	json_object_put(pJsonObj);
	pJsonObj = NULL;
	PRINT("%s\n", c8Char);
#if TEST_CLOUD
	{
	StMMap stMap = {NULL};
	StSendInfo stSendInfo = {false, NULL, GET_SELF_REGION_ADDR, NULL, -1};

	stSendInfo.pSendBody = c8Char;
	stSendInfo.s32BodySize = -1;
	s32Err = CloudSendAndGetReturn(pStat, &stSendInfo, &stMap);
	if (s32Err != 0)
	{
		return s32Err;
	}
	/* analysis the return text */
	pJsonObj = GetHttpJsonBody(&stMap, &s32Err);
	CloudMapRelease(&stMap);
	if (pJsonObj == NULL)
	{
		return s32Err;
	}
	s32Err = ResolveCmdGetSelfRegion(pJsonObj, pRegion, u32Size);
	json_object_put(pJsonObj);
	pJsonObj = NULL;

	}
#endif
	return s32Err;
}


int32_t GetRegionMappingCmdAddBaseInfo(json_object *pJsonObj)
{
	int32_t s32Err = 0;
	json_object_object_add(pJsonObj, "Sc", json_object_new_string(GET_REGION_MAPPING_SC));
	json_object_object_add(pJsonObj, "Sv", json_object_new_string(GET_REGION_MAPPING_SV));
	json_object_object_add(pJsonObj, "QueueNum", json_object_new_int(s_u32QueueNum++));
	json_object *pIDPS = GetIDPS(&s32Err);
	if (s32Err == 0)
	{
		json_object_object_add(pJsonObj, "IDPS", pIDPS);
	}
	return s32Err;
}


json_object *BuildupCmdGetRegionMapping(int32_t *pErr)
{
	json_object * pJsonObj = json_object_new_object();
	if (pJsonObj == NULL)
	{
		*pErr = MY_ERR(_Err_JSON);
		return NULL;
	}

	*pErr = GetRegionMappingCmdAddBaseInfo(pJsonObj);
	if (*pErr != 0)
	{
		json_object_put(pJsonObj);
		return NULL;
	}
	return pJsonObj;
}

int32_t ResolveCmdGetRegionMapping(json_object *pJsonObj, StRegionMapping **p2Mapping, uint32_t *pCnt)
{
	int32_t s32Err = 0;
	uint32_t i;
	uint32_t u32Cnt = 0;
	StRegionMapping *pMap = NULL;
	const char *pKey[3] =
	{
		"Region",
		"CloudUrlVar",
		"HBUrl",
	};
	const uint8_t u8DestSize[3] =
	{
		16 - 1,
		64 - 1,
		64 - 1,
	};
	if (!json_object_is_type(pJsonObj, json_type_array))
	{
		PRINT("error json type\n");
		return MY_ERR(_Err_Cloud_JSON);
	}

	u32Cnt = json_object_array_length(pJsonObj);
	PRINT("json array length is: %u\n", u32Cnt);
	if (u32Cnt == 0)
	{
		return MY_ERR(_Err_Cloud_JSON);
	}

	pMap = malloc(u32Cnt * sizeof(StRegionMapping));
	if (pMap == NULL)
	{
		return MY_ERR(_Err_Mem);
	}
	*p2Mapping = pMap;

	for (i = 0; i < u32Cnt; i++)
	{
		json_object *pObj = json_object_array_get_idx(pJsonObj, i);
		char *pDest[3] = {pMap->c8Region, pMap->c8Cloud, pMap->c8Heartbeat};
		int32_t j;
		for (j = 0; j < 3; j++)
		{
			json_object *pValue = json_object_object_get(pObj, pKey[j]);
			const char *pTmp = json_object_get_string(pValue);
			if (pTmp == NULL)
			{
				PRINT("can not find value of key(%s)\n", pKey[j]);
				s32Err = MY_ERR(_Err_Cloud_JSON);
				break;
			}
			pDest[j][u8DestSize[j]] = 0;
			strncpy(pDest[j], pTmp, u8DestSize[j]);
		}
		if (s32Err != 0)
		{
			break;
		}
		pMap++;
	}
	if (s32Err != 0)
	{
		free(*p2Mapping);
		*p2Mapping = NULL;
		*pCnt = 0;
	}
	else
	{
		*pCnt = u32Cnt;
	}

	return s32Err;
}

/*
 * 函数名      : CloudGetRegionMapping
 * 功能        : 通过云得到区域的对应的domain
 * 参数        : pStat [in] (StCloudStat * 类型): 云状态，详见定义
 * 			   : p2Mapping [in/out] (StRegionMapping ** 类型): 成功返回申请的数组
 *             : pCnt [in/out] (uint32_t * 类型): 成功返回*p2Mapping申请的数组的大小
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t CloudGetRegionMapping(StCloudDomain *pStat, StRegionMapping **p2Mapping, uint32_t *pCnt)
{
	int32_t s32Err = 0;
	json_object *pJsonObj = NULL;
	char c8Char[PAGE_SIZE];

	if ((pStat == NULL) || (p2Mapping == NULL) || (pCnt == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}
	pJsonObj = BuildupCmdGetRegionMapping(&s32Err);
	if (pJsonObj == NULL)
	{
		return MY_ERR(_Err_JSON);
	}

	sprintf(c8Char, "content=%s", json_object_to_json_string(pJsonObj));
	json_object_put(pJsonObj);
	pJsonObj = NULL;
	PRINT("%s\n", c8Char);
#if TEST_CLOUD
	{
	StMMap stMap = {NULL};
	StSendInfo stSendInfo = {false, NULL, GET_REGION_MAPPING_ADDR, NULL, -1};

	stSendInfo.pSendBody = c8Char;
	stSendInfo.s32BodySize = -1;
	s32Err = CloudSendAndGetReturn(pStat, &stSendInfo, &stMap);
	if (s32Err != 0)
	{
		return s32Err;
	}
	/* analysis the return text */
	pJsonObj = GetHttpJsonBody(&stMap, &s32Err);
	CloudMapRelease(&stMap);
	if (pJsonObj == NULL)
	{
		return s32Err;
	}
	s32Err = ResolveCmdGetRegionMapping(pJsonObj, p2Mapping, pCnt);
	json_object_put(pJsonObj);
	pJsonObj = NULL;
	}
#endif
	return s32Err;
}



int32_t GetLastVersionCmdAddBaseInfo(json_object *pJsonObj)
{
	int32_t s32Err = 0;
	json_object_object_add(pJsonObj, "Sc", json_object_new_string(GET_LAST_VERSION_SC));
	json_object_object_add(pJsonObj, "Sv", json_object_new_string(GET_LAST_VERSION_SV));
	json_object_object_add(pJsonObj, "QueueNum", json_object_new_int(s_u32QueueNum++));
	json_object *pIDPS = GetIDPS(&s32Err);
	if (s32Err == 0)
	{
		json_object_object_add(pJsonObj, "IDPS", pIDPS);
	}
	return s32Err;
}


json_object *BuildupCmdGetLastVersion(int32_t *pErr)
{
	json_object * pJsonObj = json_object_new_object();
	if (pJsonObj == NULL)
	{
		*pErr = MY_ERR(_Err_JSON);
		return NULL;
	}

	*pErr = GetLastVersionCmdAddBaseInfo(pJsonObj);
	if (*pErr != 0)
	{
		json_object_put(pJsonObj);
		return NULL;
	}
	return pJsonObj;
}

json_object *ClouldGetLastVersion(StCloudDomain *pStat, int32_t *pErr)
{
	int32_t s32Err = 0;
	json_object *pJsonObj = NULL;
	char c8Char[PAGE_SIZE];

	if (pStat == NULL)
	{
		s32Err = MY_ERR(_Err_InvalidParam);
		goto end;
	}
	pJsonObj = BuildupCmdGetLastVersion(&s32Err);
	if (pJsonObj == NULL)
	{
		s32Err = MY_ERR(_Err_JSON);
		goto end;
	}

	sprintf(c8Char, "content=%s", json_object_to_json_string(pJsonObj));
	json_object_put(pJsonObj);
	pJsonObj = NULL;
	PRINT("%s\n", c8Char);
#if TEST_CLOUD
	{
	StMMap stMap = {NULL};
	StSendInfo stSendInfo = {false, NULL, GET_LAST_VERSION_ADDR, NULL, -1};

	stSendInfo.pSendBody = c8Char;
	stSendInfo.s32BodySize = -1;
#if 0
	s32Err = CloudSendAndGetReturnNoSSL(pStat, &stSendInfo, &stMap);
#else
	s32Err = CloudSendAndGetReturn(pStat, &stSendInfo, &stMap);
#endif
	if (s32Err != 0)
	{
		goto end;
	}
	/* analysis the return text */
	pJsonObj = GetHttpJsonBody(&stMap, &s32Err);
	CloudMapRelease(&stMap);
	if (pJsonObj == NULL)
	{
		goto end;
	}
#if 0
	PRINT("\nreturn:\n%s\n", json_object_to_json_string_ext(pJsonObj, JSON_C_TO_STRING_PRETTY));
	json_object_put(pJsonObj);
	pJsonObj = NULL;
#endif
	}
#endif

end:
	if (pErr != NULL)
	{
		*pErr = s32Err;
	}
	return pJsonObj;
}
/*
 * 函数名      : CloudKeepAlive
 * 功能        : 云保活
 * 参数        : pStat [in] (StCloudDomain * 类型): 云状态，详见定义
 *             : c8ID [in] (const char * 类型): 产品ID
 * 返回        : 正确返回0, 错误返回错误码
 * 作者        : 许龙杰
 */
int32_t CloudKeepAlive(StCloudDomain *pStat, const char c8ID[PRODUCT_ID_CNT])
{
#if 0
	json_object *pJsonObj;
	char c8Char[PAGE_SIZE];
	int32_t s32Err = 0;

	if ((pStat == NULL) || (c8ID == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}

	pJsonObj = BuildupCmd_0xF5(c8ID, &s32Err);
	if (pJsonObj == NULL)
	{
		return MY_ERR(_Err_JSON);
	}

	sprintf(c8Char, "Body:%s", json_object_to_json_string(pJsonObj));
	json_object_put(pJsonObj);
	pJsonObj = NULL;
	PRINT("%s\n", c8Char);
#if TEST_CLOUD
	{
	StSendInfo stSendInfo = {false, AUTHENTICATION_ADDR, NULL, -1};
	StMMap stMap = {NULL};
	json_object *pReturnValue;
	stSendInfo.pSendBody = c8Char;
	s32Err = CloudSendAndGetReturn(pStat, &stSendInfo, &stMap);
	CloudMapRelease(&stMap);
	}
#endif
	return s32Err;
#else
	return 0;
#endif
}


/*
 * 函数名      : UDPKAInit
 * 功能        : UDP保活超时结构体初始化, 与UDPKADestroy成对使用
 * 参数        : 无
 * 返回        : StUDPKeepalive *类型, 错误返回指针NULL, 其余正确
 * 作者        : 许龙杰
 */
StUDPKeepalive *UDPKAInit(void)
{
	int32_t i;
	StUDPKeepalive *pUDP = (StUDPKeepalive *)calloc(1, sizeof(StUDPKeepalive));

	if (pUDP == NULL)
	{
		return NULL;
	}
	for (i = 0; i < CMD_CNT - 1; i++)
	{
		pUDP->stUDPInfo[i].pNext = pUDP->stUDPInfo + i + 1;
	}
	pUDP->stUDPInfo[i].pNext = pUDP->stUDPInfo;
	pUDP->stUDPInfo[0].pPrev = pUDP->stUDPInfo + i;
	for (i = 1; i < CMD_CNT; i++)
	{
		pUDP->stUDPInfo[i].pPrev = pUDP->stUDPInfo + i - 1;
	}
	return pUDP;
}

/*
 * 函数名      : UDPKAAddASendTime
 * 功能        : 当发送一个心跳包后, 将报序号和发送的时间记录
 * 参数        : pUDP [in] (StUDPKeepalive *) UDPKAInit返回的结构体指针
 *             : u16SendNum [in] (uint16_t) 包序号
 *             : u64SendTime[in] (uint64_t) 发送的时间
 * 返回        : int32_t类型, 正确返回0, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t UDPKAAddASendTime(StUDPKeepalive *pUDP, uint16_t u16SendNum,
	uint64_t u64SendTime)
{
	if (pUDP == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}


	if (pUDP->u16SendCnt == 0)
	{
		pUDP->u16LatestSendNum = u16SendNum;
		pUDP->u16OldestSendNum = u16SendNum;
		pUDP->pCur = pUDP->pOldest = pUDP->stUDPInfo;
	}

	{
	int32_t i;
	StUDPInfo *pInfo = pUDP->pCur->pPrev;
	for (i = pUDP->u16SendCnt; i > 0; i--)
	{
		if (pInfo->u16QueueNum == u16SendNum)
		{
			pInfo->u64SendTime = u64SendTime;
			return 0;
		}
		pInfo = pInfo->pPrev;
	}
	}

	pUDP->u16LatestSendNum = u16SendNum;

	pUDP->pCur->u16QueueNum = u16SendNum;
	pUDP->pCur->u64SendTime = u64SendTime;
	pUDP->pCur->u64ReceivedTime = 0;

	pUDP->pCur = pUDP->pCur->pNext;

	if (pUDP->u16SendCnt == CMD_CNT)
	{
		pUDP->pOldest = pUDP->pOldest->pNext;
	}
	else
	{
		pUDP->u16SendCnt++;
	}
	return 0;
}

/*
 * 函数名      : UDPKAAddAReceivedTime
 * 功能        : 当受到一个心跳包后, 将报序号, 接收的时间和服务器发送记录时的时间记录
 * 参数        : pUDP [in] (StUDPKeepalive *) UDPKAInit返回的结构体指针
 *             : Received [in] (uint16_t) 包序号
 *             : u64SendTime[in] (uint64_t) 接收的时间
 *             : u64ServerTime[in] (uint64_t) 服务器发送记录时的时间
 * 返回        : int32_t类型, 正确返回0, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t UDPKAAddAReceivedTime(StUDPKeepalive *pUDP, uint16_t u16ReceivedNum,
	uint64_t u64ReceivedTime, uint64_t u64ServerTime)
{

	if (pUDP == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	if (pUDP->u16SendCnt == 0)
	{
		return MY_ERR(_Err_Common);
	}
	{
	int32_t i;
	StUDPInfo *pInfo = pUDP->pCur->pPrev;
	for (i = pUDP->u16SendCnt; i > 0; i--)
	{
		if (pInfo->u16QueueNum == u16ReceivedNum)
		{
			pUDP->u16LatestReceivedNum = u16ReceivedNum;
			pInfo->u64ReceivedTime = u64ReceivedTime;
			pInfo->u64ServerTime = u64ServerTime;
			if (pUDP->u16RecvCnt < CMD_CNT)
			{
				pUDP->u16RecvCnt++;
			}
			return 0;
		}
		pInfo = pInfo->pPrev;
	}
	}
	return MY_ERR(_Err_Common);
}

/*
 * 函数名      : UDPKAAddAReceivedTime
 * 功能        : 判断当前记录的数据中, UDP保活是否已经超时
 * 参数        : pUDP [in] (StUDPKeepalive *) UDPKAInit返回的结构体指针
 * 返回        : bool类型, 超时返回true, 否则返回false
 * 作者        : 许龙杰
 */
bool UPDKAIsTimeOut(StUDPKeepalive *pUDP)
{
	if (pUDP == NULL)
	{
		return true;
	}
	if (pUDP->u16SendCnt < TIMEOUT_CNT)
	{
		return false;
	}

	if ((pUDP->u16LatestSendNum - pUDP->u16LatestReceivedNum) > TIMEOUT_CNT)
	{
		return true;
	}
	return false;
}

/*
 * 函数名      : UDPKAGetTimeDiff
 * 功能        : 得到本地时间与服务器之间的时间差
 * 参数        : pUDP [in] (StUDPKeepalive *) UDPKAInit返回的结构体指针
 *             : pTimeDiff [in/out] (int64_t *) 用于保存时间差
 * 返回        : int32_t类型, 正确返回0, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t UDPKAGetTimeDiff(StUDPKeepalive *pUDP, int64_t *pTimeDiff)
{
	int32_t i;
	int32_t s32EchoCnt;
	int64_t s64TimeDiff = 0;

	if (pUDP == NULL)
	{
		return  MY_ERR(_Err_Common);
	}
	if (pUDP->u16RecvCnt < MIN_COMPUTE_CNT)
	{
		return  MY_ERR(_Err_Common);
	}

	do
	{
		StUDPInfo *pInfo = pUDP->pCur->pPrev;
		s32EchoCnt = 0;
		for (i = pUDP->u16SendCnt; i > 0; i--)
		{
			if ((pInfo->u64ReceivedTime != 0) && (pInfo->u64SendTime != 0))
			{
				uint64_t u64EchoTimeDiff = pInfo->u64ReceivedTime - pInfo->u64SendTime;
				u64EchoTimeDiff = pInfo->u64SendTime + u64EchoTimeDiff / 2;
				s64TimeDiff += (pInfo->u64ServerTime - u64EchoTimeDiff);
				s32EchoCnt++;
				if (s32EchoCnt >= MIN_COMPUTE_CNT)
				{
					s64TimeDiff /= MIN_COMPUTE_CNT;
					if (pTimeDiff != NULL)
					{
						*pTimeDiff = s64TimeDiff;
					}
					return 0;
				}
			}
			pInfo = pInfo->pPrev;
		}
	}while(0);
	return  MY_ERR(_Err_Common);
}

/*
 * 函数名      : UDPKAClearTimeDiff
 * 功能        : 清除时间差统计结果
 * 参数        : pUDP [in] (StUDPKeepalive *) UDPKAInit返回的结构体指针
 * 返回        : int32_t类型, 正确返回0, 否则返回错误码
 * 作者        : 许龙杰
 */
int32_t UDPKAClearTimeDiff(StUDPKeepalive *pUDP)
{
	int32_t i;
	if (pUDP == NULL)
	{
		return  MY_ERR(_Err_Common);
	}
	for (i = 0; i < CMD_CNT; i++)
	{
		pUDP->stUDPInfo[i].u64ReceivedTime = 0;
		pUDP->stUDPInfo[i].u64SendTime = 0;
		pUDP->u16RecvCnt = 0;
	}
	return 0;
}

/*
 * 函数名      : UDPKAReset
 * 功能        : 将记录数据复位
 * 参数        : pUDP [in] (StUDPKeepalive *) UDPKAInit返回的结构体指针
 * 返回        : 无
 * 作者        : 许龙杰
 */
void UDPKAReset(StUDPKeepalive *pUDP)
{
	if(pUDP != NULL)
	{
		pUDP->u16SendCnt = 0;
		pUDP->u16RecvCnt = 0;
	}
}

/*
 * 函数名      : UDPKADestroy
 * 功能        : 销毁资源, 与UDPKAInit成对使用
 * 参数        : pUDP [in] (StUDPKeepalive *) UDPKAInit返回的结构体指针
 * 返回        : 无
 * 作者        : 许龙杰
 */
void UDPKADestroy(StUDPKeepalive *pUDP)
{
	if (pUDP != NULL)
	{
		free(pUDP);
	}
}

#if 0
void FunctionTest(void)
{
	struct addrinfo stFilter = {0};
	struct addrinfo *pRslt = NULL;
	int32_t s32Err = 0;
	stFilter.ai_family = AF_INET;
	stFilter.ai_socktype = SOCK_STREAM;
	s32Err = getaddrinfo("www.google.com.hk", NULL, &stFilter, &pRslt);
	if (s32Err == 0)
	{
		struct addrinfo *pTmp = pRslt;
		while (pTmp != NULL)
		{
			PRINT("after inet_ntoa: %s\n", inet_ntoa(((struct sockaddr_in*)(pTmp->ai_addr))->sin_addr));
			pTmp = pTmp->ai_next;
		}
	}
	else
	{
		PRINT("getaddrinfo s32Err = %d, error: %s\n", s32Err, gai_strerror(s32Err));
	}

}
#endif

#if 0

void FunctionTest(void)
{
	struct hostent *pHost;
	struct sockaddr_in stAddr;
	char c8Addr[64];

	pHost = gethostbyname("www.baidu.com");
	PRINT("www.baidu.com's host(name) is: %s\n", pHost->h_name);
	PRINT("www.baidu.com's host(AF_INET) is: %hhu.%hhu.%hhu.%hhu\n",
			pHost->h_addr[0], pHost->h_addr[1], pHost->h_addr[2], pHost->h_addr[3]);
	PRINT("after inet_ntoa: %s\n", inet_ntoa(*((struct in_addr *)pHost->h_addr)));

	pHost = gethostbyname("74.125.128.199");
	PRINT("74.125.128.199's host is: %s\n", pHost->h_addr);
	PRINT("after inet_ntoa: %s\n", inet_ntoa(*((struct in_addr *)pHost->h_addr)));


}
#endif

#if 0
/*
 * {IDPS:{"Ptl":"com.jiuan.GW01","Name":" Health Gateway","FVer":"1.0.2","HVer":"1.1.1","MFR":"iHealth","Model":"GW 001","SN":"0000000001"},Ptr:{"Region"="CN"}}
 */
void JSONTest()
{
	json_object *new_obj, *pSon_obj, *pJson;
	new_obj = json_object_from_file("/home/lyndon/workspace/nfsboot/GatewayIDPS.idps");
	printf("new_obj.to_string()=%s\n", json_object_to_json_string_ext(new_obj, JSON_C_TO_STRING_PRETTY));

	pJson = json_object_new_object();

	printf("\n\n");
	json_object_object_get_ex(new_obj, "IDPS", &pSon_obj);
	printf("pSon_obj(IDPS).to_string()=%s\n", json_object_to_json_string_ext(pSon_obj, JSON_C_TO_STRING_PRETTY));

	//json_object_get(pSon_obj);
	json_object_object_add(pJson, "IDPS", pSon_obj);

	printf("\n\n");
	json_object_object_get_ex(new_obj, "Ptr", &pSon_obj);
	printf("pSon_obj(Ptr).to_string()=%s\n", json_object_to_json_string(pSon_obj));

	json_object_put(new_obj);

	printf("\n\n");
	printf("pJson.to_string()=%s\n", json_object_to_json_string(pJson));
	json_object_put(pJson);

}
#endif

#if 0
void JSONTest()
{
	json_object *new_obj, *pSon_obj;
	new_obj = json_tokener_parse("{\"Result\": \"1\",\n\t\t \"TS\": 19923928382000, "\
			"\"ResultMessage\": \"100\", \"ReturnValue\": null }DADADsdf}");
	json_object_to_file("/home/lyndon/workspace/nfsboot/GatewayIDPS.idps", new_obj);
	printf("new_obj.to_string()=%s\n", json_object_to_json_string(new_obj));

	printf("\n\n");
	json_object_object_get_ex(new_obj, "Result", &pSon_obj);
	printf("pSon_obj(Result).to_string()=%s\n", json_object_to_json_string(pSon_obj));

	printf("\n\n");
	json_object_object_get_ex(new_obj, "TS", &pSon_obj);
	printf("pSon_obj(TS).to_string()=%s\n", json_object_to_json_string(pSon_obj));
	printf("pSon_obj(TS).type=%d\n", json_object_get_type(pSon_obj));

	printf("\n\n");
	json_object_object_get_ex(new_obj, "ResultMessage", &pSon_obj);
	printf("pSon_obj(ResultMessage).to_string()=%s\n", json_object_to_json_string(pSon_obj));

	printf("\n\n");
	json_object_object_get_ex(new_obj, "ReturnValue", &pSon_obj);
	printf("pSon_obj(ReturnValue).to_string()=%s\n", json_object_to_json_string(pSon_obj));
	printf("pSon_obj(ReturnValue).type=%d\n", json_object_get_type(pSon_obj));

	json_object_put(new_obj);
}
#endif



