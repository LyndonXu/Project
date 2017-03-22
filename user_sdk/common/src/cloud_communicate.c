/****************************************************************************
 * Copyright(c), 2001-2060, ******************************* 版权所有
 ****************************************************************************
 * 文件名称             : cloud_communicate.c
 * 版本                 : 0.0.1
 * 作者                 : 许龙杰
 * 创建日期             : 2014年12月8日
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

typedef enum _tagEmCloudCommunicateProcessWithSSL
{
	_CCP_SSL_Close,
	_CCP_SSL_Init,
	_CCP_SSL_DNS_Begin = _CCP_SSL_Init,
	_CCP_SSL_DNS_Send,
	_CCP_SSL_DNS_Receive,
	_CCP_SSL_DNS_End,
	_CCP_SSL_SSL_Begin = _CCP_SSL_DNS_End,
	_CCP_SSL_Socket_Connect,
	_CCP_SSL_SSL_Setup,
	_CCP_SSL_SSL_Connect,
	_CCP_SSL_HTTP_Send_Head,
	_CCP_SSL_HTTP_Send_Body,
	_CCP_SSL_HTTP_Receive,
	_CCP_SSL_SSL_End,
}EmCCPWithSSL;

#define CCP_STEP_COMPLETE		(1)
#define CCP_STEP_NEED_REPEAT	(0)

#define CSGRNB_SEND_BUF			(1024)
#define CSGRNB_RECEIVE_BUF		(1024)
#define CSGRNB_BUF				(CSGRNB_SEND_BUF + CSGRNB_RECEIVE_BUF)
typedef struct _tagStCloudSendAndGetReturnNonblock
{
	EmCCPWithSSL emCCPWithSSL;			/* polling status */
	uint32_t u32TimeOut;				/* time for close the structure if no data come */
	uint64_t u64EndTime;
	StCloudDomain *pStat;				/* cloud domain, port, online state, our local IP etc. */
	StSendInfo *pSendInfo;				/* second domain, file I want to communicate, data I want send etc. */
	StMMap *pMap;						/* memory map, if I get some data from cloud,
										   I will write it into a RAM file, and map it to our process */
	struct
	{
		union
		{
			struct
			{
				char c8BufSend[CSGRNB_SEND_BUF];			/* send buffer */
				char c8BufReceive[CSGRNB_RECEIVE_BUF];		/* receive buffer */
			};
			char c8Buf[CSGRNB_BUF];							/* mix buffer, it's better to allocate when using :) */
		};
		union
		{
			struct
			{
				uint32_t u32SendBufLength;					/* total data length I want to send */
				uint32_t u32ReceiveBufLength;				/* total data length I want to receive */
			};
			uint32_t u32BufLength;							/* mix length */
		};
	};			/* for send and receive */

	struct in_addr stServerInternetAddr;					/* after DNS I will save Internet format address in it  */
	char c8ServerIPV4Addr[IPV4_ADDR_LENGTH + 4];			/* after DNS I will save dotted-decimal format address in it */
	char c8Domain[128];										/* after DNS I will save the integer host domain in it */
	union
	{
		struct
		{
			int32_t s32DNSSocket;							/* DNS socket */
			struct sockaddr_in stDNSServerAddr;				/* DNS server address, for send and check the received data */
			FILE *pDNSFile;									/* /etc/resolv.conf */
		};		/* for DNS */
		struct
		{
			int32_t s32SSLSocket;							/* SSL socket */
			SSL *pSSL;										/* SSL connect handle */
			SSL_CTX *pSSLCTX;								/* SSL method */
			uint32_t u32SSLCurSend;							/* data count I have send */
			uint32_t u32SSLCurReceive;						/* data count I have received */
			uint32_t u32SSLCouldReceive;					/* data count I can receive */
			FILE *pSSLFile;									/* RAM file where I will write the received data */
			void *pReceive;									/* map address of the RAM file */
		};
	};
}StCSGRNB;
int32_t DNSBuildQuery(const char *pHost, char *pBuf);
int32_t ResolveDNSAnswer(const char *pSendBuf, const uint32_t u32SendLength,
		const char *pRecvBuf, const uint32_t u32RecvLength,
		char c8IPV4Addr[IPV4_ADDR_LENGTH], struct in_addr *pInternetAddr);

StCSGRNB *CSGRNBInit(StCloudDomain *pStat, StSendInfo *pSendInfo, StMMap *pMap, uint32_t u32TimeOut, int32_t *pErr)
{
	StCSGRNB *pHandle = NULL;
	int32_t s32Err = 0;
	if ((pStat == NULL) || (pSendInfo == NULL) || (pMap == NULL))
	{
		s32Err = MY_ERR(_Err_InvalidParam);
		goto end;
	}
	if (pStat->stStat.emStat != _Cloud_IsOnline)
	{
		s32Err = MY_ERR(_Err_Cloud_IsNotOnline);
		goto end;
	}

	pHandle = calloc(1, sizeof(StCSGRNB));
	if (pHandle == NULL)
	{
		goto end;
	}

	pHandle->emCCPWithSSL = _CCP_SSL_Init;
	pHandle->pStat = pStat;
	pHandle->pMap = pMap;
	pHandle->pSendInfo = pSendInfo;
	pHandle->u32TimeOut = u32TimeOut;
	pHandle->u64EndTime = 0;
end:
	if (pErr != NULL)
	{
		*pErr = s32Err;
	}
	return pHandle;
}

static void CSGRNBTombDestroyDNS(StCSGRNB *pHandle)
{
	if (pHandle->s32DNSSocket >= 0)
	{
		close(pHandle->s32DNSSocket);
	}
	if (pHandle->pDNSFile != NULL)
	{
		fclose(pHandle->pDNSFile);
	}
}
static int32_t CSGRNBPollGetDNS(StCSGRNB *pHandle)
{
	int32_t s32Result;
	switch (pHandle->emCCPWithSSL)
	{
		case _CCP_SSL_DNS_Begin:		/* this state will be run just once */
		{
			pHandle->s32DNSSocket = -1;		/* initial argument */
			pHandle->pDNSFile = NULL;

			PRINT("_CCP_SSL_DNS_Begin\n");
			pHandle->c8ServerIPV4Addr[IPV4_ADDR_LENGTH] = 0;		/* avoid no terminating null byte */
			if (pHandle->pStat->c8Domain == NULL)
			{
				return MY_ERR(_Err_InvalidParam);
			}

			/* check whether the domain is rightful dotted-decimal format address, if true, return complete :) */
			if (inet_pton(AF_INET, pHandle->pStat->c8Domain, &s32Result) > 0)
			{
				pHandle->stServerInternetAddr.s_addr = s32Result;
				strncpy(pHandle->c8ServerIPV4Addr, pHandle->pStat->c8Domain, IPV4_ADDR_LENGTH);
				pHandle->emCCPWithSSL = _CCP_SSL_DNS_End;
				PRINT("IPV4 is %s %08X\n", pHandle->pStat->c8Domain, pHandle->stServerInternetAddr.s_addr);
				return CCP_STEP_COMPLETE;
			}
			else
			{
				FILE *pFile;
				int32_t s32Socket;
				struct sockaddr_in stAddr;

				/* transform the address */
				if (inet_aton(pHandle->pStat->stStat.c8ClientIPV4, &stAddr.sin_addr) == 0)
				{
					PRINT("client IP address error!\n");
					return MY_ERR(_Err_SYS + errno);
				}
				else
				{
					PRINT("local IP is %s\n", pHandle->pStat->stStat.c8ClientIPV4);
				}

				pFile = fopen("/etc/resolv.conf", "rb");
				if (pFile == NULL)
				{
					s32Result = MY_ERR(_Err_SYS + errno);
					PRINT("open file /etc/resolv.conf error: %s\n", strerror(errno));
					return s32Result;
				}
				s32Socket = socket(AF_INET, SOCK_DGRAM, 0);
				if (s32Socket < 0)
				{
					fclose(pFile);
					s32Result = MY_ERR(_Err_SYS + errno);
					PRINT("socket error: %s\n", strerror(errno));
					return s32Result;
				}

				stAddr.sin_family = AF_INET;
				stAddr.sin_port = htons(0);
				stAddr.sin_port = htonl(INADDR_ANY);
				if (bind(s32Socket, (struct sockaddr *)(&stAddr), sizeof(struct sockaddr)))
				{
					s32Result = MY_ERR(_Err_SYS + errno);
					PRINT("socket error: %s\n", strerror(errno));
					goto err;
				}
				do
				{
					int32_t s32Mode;

					/*First we make the socket nonblocking*/
					s32Mode = fcntl(s32Socket,F_GETFL,0);
					s32Mode |= O_NONBLOCK;
					if (fcntl(s32Socket, F_SETFL, s32Mode) == -1)
					{
						PRINT("fcntl error:%s\n", strerror(errno));
						s32Result = MY_ERR(_Err_SYS + errno);
						goto err;
					}
				}while(0);

				/* build the domain and build the DNS query */
				if (pHandle->pSendInfo->pSecondDomain != NULL)
				{
					sprintf(pHandle->c8Domain, "%s.%s", pHandle->pSendInfo->pSecondDomain, pHandle->pStat->c8Domain);
					s32Result = DNSBuildQuery(pHandle->c8Domain, pHandle->c8BufSend);
				}
				else
				{
					sprintf(pHandle->c8Domain, "%s", pHandle->pStat->c8Domain);
					s32Result = DNSBuildQuery(pHandle->pStat->c8Domain, pHandle->c8BufSend);
				}

				if (s32Result < 0)
				{
					goto err;
				}
				pHandle->u32SendBufLength = s32Result;
				/* save argument */
				pHandle->pDNSFile = pFile;
				pHandle->s32DNSSocket = s32Socket;
				/* changed state to _CCP_SSL_DNS_Receive and set the timeout */
				pHandle->emCCPWithSSL = _CCP_SSL_DNS_Send;
				pHandle->u64EndTime = TimeGetSetupTime() + pHandle->u32TimeOut;

				goto dns_send;	/* no error happened  */
err:
				fclose(pFile);
				close(s32Socket);
				return s32Result;
			}
		}
		/* no break, because I want try to receive something immediately */
		case _CCP_SSL_DNS_Send:
		{
dns_send:
			PRINT("_CCP_SSL_DNS_Send\n");
			s32Result = -1;
			while (feof(pHandle->pDNSFile) == 0)
			{
				char *pRead = fgets(pHandle->c8BufReceive, CSGRNB_RECEIVE_BUF, pHandle->pDNSFile);
				if (pRead == NULL)
				{
					fclose(pHandle->pDNSFile);
					close(pHandle->s32DNSSocket);
					return MY_ERR(_Err_TimeOut);
				}
				if (pRead[0] == '#')
				{
					continue;
				}
				if (pRead[strlen(pRead) - 1] == '\n')
				{
					pRead[strlen(pRead) - 1] = 0;
				}
				if (inet_pton(AF_INET, pRead + 11, &s32Result) <= 0)
				{
					PRINT("unknown string: %s\n", pRead + 11);
					continue;
				}
				PRINT("resolve from: %s\n", pRead + 11);
				break;
			}
			if ((feof(pHandle->pDNSFile) != 0) && (s32Result == -1))
			{
				PRINT("no resolve could be try\n");
				fclose(pHandle->pDNSFile);
				close(pHandle->s32DNSSocket);
				return MY_ERR(_Err_TimeOut);
			}

			pHandle->stDNSServerAddr.sin_family = AF_INET;
			pHandle->stDNSServerAddr.sin_port = htons(DNS_PORT);
			pHandle->stDNSServerAddr.sin_addr.s_addr = s32Result;
			pHandle->u64EndTime = TimeGetSetupTime() + pHandle->u32TimeOut;

			s32Result = sendto(pHandle->s32DNSSocket, pHandle->c8BufSend, pHandle->u32SendBufLength, MSG_NOSIGNAL,
					(struct sockaddr *)(&(pHandle->stDNSServerAddr)), sizeof(struct sockaddr));
			if (s32Result != pHandle->u32SendBufLength)
			{
				s32Result = MY_ERR(_Err_SYS + errno);
				fclose(pHandle->pDNSFile);
				close(pHandle->s32DNSSocket);
				return s32Result;
			}
#if defined _DEBUG
			{
				uint32_t i;
				for (i = 0; i < pHandle->u32SendBufLength; i++)
				{
					if ((i & 0x0F) == 0)
					{
						printf("\n");
					}
					printf("%02hhX ", pHandle->c8BufSend[i]);
				}
				printf("\n");
			}
#endif
			pHandle->emCCPWithSSL = _CCP_SSL_DNS_Receive;
		}
		/* no break */
		case _CCP_SSL_DNS_Receive:
		{
			socklen_t s32Len;
			struct sockaddr_in stAddr;

			PRINT("_CCP_SSL_DNS_Receive\n");

			s32Len = sizeof(struct sockaddr_in);
			s32Result = recvfrom(pHandle->s32DNSSocket, pHandle->c8BufReceive, CSGRNB_RECEIVE_BUF,
									0, (struct sockaddr *)(&stAddr), &s32Len);
			if (s32Result < 0)
			{
				if ((errno == EWOULDBLOCK) || (errno == EAGAIN))
				{
					if (TimeGetSetupTime() > pHandle->u64EndTime)
					{
						/* try to resolve the host from anther server */
						PRINT("_CCP_SSL_DNS_Receive timeout, try next resolve\n");
						goto dns_send;
					}
					PRINT("_CCP_SSL_DNS_Receive try again\n");
					return CCP_STEP_NEED_REPEAT;
				}
				else	/* error happened */
				{
					PRINT("_CCP_SSL_DNS_Receive: %s(%d)\n", strerror(errno), errno);
					fclose(pHandle->pDNSFile);
					close(pHandle->s32DNSSocket);
					return MY_ERR(_Err_SYS + errno);
				}
			}
			pHandle->u32ReceiveBufLength = s32Result;
			/* for whatever, we need close the file and the socket */
			fclose(pHandle->pDNSFile);
			close(pHandle->s32DNSSocket);

#if defined _DEBUG
			{
				uint32_t i;
				for (i = 0; i < pHandle->u32ReceiveBufLength; i++)
				{
					if ((i & 0x0F) == 0)
					{
						printf("\n");
					}
					printf("%02hhX ", pHandle->c8BufReceive[i]);
				}
				printf("\n");
			}
#endif
			/* the message is not for us!!, big problem! */
			if((s32Len != sizeof (struct sockaddr_in)) ||
					(memcmp(&pHandle->stDNSServerAddr, &stAddr, sizeof(struct sockaddr_in)) != 0))
			{
				return MY_ERR(_Err_Common);
			}

			PRINT("begin to resolve the DNS answer\n");
			s32Result = ResolveDNSAnswer(pHandle->c8BufSend, pHandle->u32SendBufLength,
					pHandle->c8BufReceive, pHandle->u32ReceiveBufLength,
					pHandle->c8ServerIPV4Addr, &(pHandle->stServerInternetAddr));
			if (s32Result == 0)
			{
				PRINT("get host IP: %s %08X\n", pHandle->c8ServerIPV4Addr, pHandle->stServerInternetAddr.s_addr);
				return CCP_STEP_COMPLETE;
			}
			break;
		}
		default:
			s32Result = MY_ERR(_Err_InvalidParam);
			break;
	}
	return s32Result;
}
static void CSGRNBDestroySSL(StCSGRNB *pHandle)
{
	if (pHandle->pSSL != NULL)
	{
		SSL_shutdown(pHandle->pSSL);
		SSL_free(pHandle->pSSL);
	}
	if(pHandle->pSSLCTX != NULL)
	{
		SSL_CTX_free(pHandle->pSSLCTX);
	}
	if(pHandle->s32SSLSocket > 0)
	{
		close(pHandle->s32SSLSocket);
	}
}

static void CSGRNBTombDestroySSL(StCSGRNB *pHandle)
{
	if (pHandle->pSSL != NULL)
	{
		SSL_shutdown(pHandle->pSSL);
		SSL_free(pHandle->pSSL);
	}
	if(pHandle->pSSLCTX != NULL)
	{
		SSL_CTX_free(pHandle->pSSLCTX);
	}
	if(pHandle->s32SSLSocket > 0)
	{
		close(pHandle->s32SSLSocket);
	}
	if (pHandle->pReceive != NULL)
	{
		munmap(pHandle->pReceive, pHandle->u32SSLCouldReceive);
	}
	if (pHandle->pSSLFile != NULL)
	{
		fclose(pHandle->pSSLFile);
	}
}

static int32_t CSGRNBPollSSL(StCSGRNB *pHandle)
{
	int32_t s32Result;
	switch (pHandle->emCCPWithSSL)
	{
		case _CCP_SSL_SSL_Begin:		/* this state will be run just once */
		{
			/* initial some argument */
			struct sockaddr_in stLocalAddr, stServerAddr;
			int32_t s32Socket = -1;
			pHandle->s32SSLSocket = -1;
			pHandle->pSSL = NULL;
			pHandle->pSSLCTX = NULL;
			pHandle->u32SSLCurReceive = 0;
			pHandle->u32SSLCouldReceive = 0;
			pHandle->pSSLFile = NULL;
			pHandle->pReceive = NULL;

			PRINT("_CCP_SSL_SSL_Begin\n");

			bzero(&stServerAddr, sizeof(struct sockaddr_in));
			stServerAddr.sin_family = AF_INET;
			stServerAddr.sin_port = htons(pHandle->pStat->s32Port);	/* https */
			stServerAddr.sin_addr = pHandle->stServerInternetAddr;
			bzero(&stLocalAddr, sizeof(struct sockaddr_in));

			stLocalAddr.sin_family = AF_INET;    /* Internet protocol */
			if (inet_aton(pHandle->pStat->stStat.c8ClientIPV4, &stLocalAddr.sin_addr) == 0)
			{
				PRINT("client IP address error!\n");
				return MY_ERR(_Err_SYS + errno);
			}
			stLocalAddr.sin_port = htons(0);    /* the system will allocate a port number for it */

			if ((s32Socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
			{
				PRINT("socket error:%s\n", strerror(errno));
				return MY_ERR(_Err_SYS + errno);
			}

			if (bind(s32Socket, (struct sockaddr*) &stLocalAddr, sizeof(struct sockaddr_in)) != 0)
			{
				s32Result = MY_ERR(_Err_SYS + errno);
				PRINT("bind error:%s\n", strerror(errno));
				close(s32Socket);
				return s32Result;
			}
			{
				int32_t s32Mode;

				/*First we make the socket nonblocking*/
				s32Mode = fcntl(s32Socket,F_GETFL,0);
				s32Mode |= O_NONBLOCK;
				if (fcntl(s32Socket, F_SETFL, s32Mode) == -1)
				{
					s32Result = MY_ERR(_Err_SYS + errno);
					PRINT("bind error:%s\n", strerror(errno));
					close(s32Socket);
					return s32Result;
				}
			}
			pHandle->u64EndTime = TimeGetSetupTime() + pHandle->u32TimeOut;
			PRINT("begin connect\n");
			if (connect(s32Socket, (struct sockaddr *)(&stServerAddr), sizeof(struct sockaddr_in)) < 0)
			{
				if (errno != EINPROGRESS)
				{
					s32Result = MY_ERR(_Err_SYS + errno);
					PRINT("bind error:%s\n", strerror(errno));
					close(s32Socket);
					return s32Result;
				}
				s32Result = CCP_STEP_NEED_REPEAT;
				pHandle->s32SSLSocket = s32Socket;
				pHandle->emCCPWithSSL = _CCP_SSL_Socket_Connect;
			}
			else
			{
				PRINT("it's connect OK unbelievably\n");
				pHandle->emCCPWithSSL = _CCP_SSL_SSL_Setup;
				goto ssl_setup;
			}
			break;
		}
		case _CCP_SSL_Socket_Connect:
		{
			struct timeval stTimeVal;
			fd_set stFdSet;

			PRINT("_CCP_SSL_Socket_Connect\n");

			/* check connect status */
			stTimeVal.tv_sec = 0;
			stTimeVal.tv_usec = 0;
			FD_ZERO(&stFdSet);
			FD_SET(pHandle->s32SSLSocket, &stFdSet);
			s32Result = select(pHandle->s32SSLSocket + 1, NULL, &stFdSet, NULL, &stTimeVal);
			if (s32Result > 0)		/* OK */
			{
				socklen_t u32Len = sizeof(int32_t);
				/* for firewall */
				getsockopt(pHandle->s32SSLSocket, SOL_SOCKET, SO_ERROR, &s32Result, &u32Len);
				if (s32Result != 0)
				{
					s32Result = MY_ERR(_Err_SYS + errno);
					close(pHandle->s32SSLSocket);
					PRINT("connect error:%s\n", strerror(errno));
					return s32Result;
				}
				pHandle->emCCPWithSSL = _CCP_SSL_SSL_Setup;
				goto ssl_setup;
			}
			else
			{
				if (TimeGetSetupTime() > pHandle->u64EndTime)
				{
					PRINT("connect timeout\n");
					close(pHandle->s32SSLSocket);
					return MY_ERR(_Err_TimeOut);
				}
				s32Result = CCP_STEP_NEED_REPEAT;
			}
			break;
		}
		case _CCP_SSL_SSL_Setup:
		{
ssl_setup:
			PRINT("_CCP_SSL_SSL_Setup\n");

			pHandle->pSSLCTX = SSL_CTX_new(SSLv23_client_method());
			if (pHandle->pSSLCTX == NULL)
			{
				PRINT("SSL_CTX_new error:%s\n", strerror(errno));
				s32Result = MY_ERR(_Err_SYS + errno);
				CSGRNBDestroySSL(pHandle);
				return s32Result;
			}

			pHandle->pSSL = SSL_new(pHandle->pSSLCTX);
			if (pHandle->pSSL == NULL)
			{
				PRINT("SSL_new error:%s\n", strerror(errno));
				s32Result = MY_ERR(_Err_SYS + errno);
				CSGRNBDestroySSL(pHandle);
				return s32Result;
			}

			/* link the socket to the SSL */
			s32Result = SSL_set_fd(pHandle->pSSL, pHandle->s32SSLSocket);
			if (s32Result == 0)
			{
				ERR_print_errors_fp(stderr);
				s32Result = SSL_get_error(pHandle->pSSL, s32Result);
				PRINT("SSL_set_fd error %d\n", s32Result);
				s32Result = MY_ERR(_Err_SSL + s32Result);
				CSGRNBDestroySSL(pHandle);
				return s32Result;
			}

			RAND_poll();
			while (RAND_status() == 0)
			{
				time_t s32Time = time(NULL);
				RAND_seed(&s32Time, sizeof(time_t));
			}
			pHandle->u64EndTime = TimeGetSetupTime() + pHandle->u32TimeOut;
			pHandle->emCCPWithSSL = _CCP_SSL_SSL_Connect;
		}
		/* no break */
		case _CCP_SSL_SSL_Connect:
		{

			PRINT("_CCP_SSL_SSL_Connect\n");

			s32Result = SSL_connect(pHandle->pSSL);
			if (s32Result < 0)
			{
				s32Result = SSL_get_error(pHandle->pSSL, s32Result);
				if ((s32Result == SSL_ERROR_WANT_READ) || (s32Result == s32Result))
				{
					if (TimeGetSetupTime() < pHandle->u64EndTime)
					{
						s32Result = CCP_STEP_NEED_REPEAT;
						return s32Result;
					}
				}
				ERR_print_errors_fp(stderr);

				PRINT("SSL_connect error %d\n", s32Result);
				s32Result = MY_ERR(_Err_SSL + s32Result);
				CSGRNBDestroySSL(pHandle);
				return s32Result;
			}
			else if (s32Result == 0)
			{
				s32Result = SSL_get_error(pHandle->pSSL, s32Result);
				ERR_print_errors_fp(stderr);

				PRINT("SSL_connect error %d\n", s32Result);
				s32Result = MY_ERR(_Err_SSL + s32Result);
				CSGRNBDestroySSL(pHandle);
				return s32Result;

			}

			if (pHandle->pSendInfo->s32BodySize == -1)
			{
				if (pHandle->pSendInfo->pSendBody == NULL)
				{
					pHandle->pSendInfo->s32BodySize = 0;
				}
				else
				{
					pHandle->pSendInfo->s32BodySize = strlen(pHandle->pSendInfo->pSendBody);
				}
			}
			sprintf(pHandle->c8BufSend,
					"%s /%s HTTP/1.1\r\n"
					"Accept: */*\r\n"
					/* "Accept-Language: zh-cn\r\n"
					"User-Agent: Mozilla/4.0 (compatible; MSIE 5.01; Windows NT 5.0)\r\n" maybe not useful */
					"Host: %s:%d\r\n"
					"Content-Length: %d\r\n"
					"Connection: Close\r\n\r\n",
					pHandle->pSendInfo->boIsGet ? "GET" : "POST",
					pHandle->pSendInfo->pFile, pHandle->c8Domain, pHandle->pStat->s32Port,
					pHandle->pSendInfo->s32BodySize);
			pHandle->u32SendBufLength = strlen(pHandle->c8BufSend);
			pHandle->u32SSLCurSend = 0;
			pHandle->u64EndTime = TimeGetSetupTime() + pHandle->u32TimeOut;
			PRINT("\n%s\n", pHandle->c8BufSend);
			pHandle->emCCPWithSSL = _CCP_SSL_HTTP_Send_Head;
		}
		/* no break */
		case _CCP_SSL_HTTP_Send_Head:
		{

			PRINT("_CCP_SSL_HTTP_Send_Head\n");
			/* send the data once as much as possible */
			while (pHandle->u32SSLCurSend < pHandle->u32SendBufLength)
			{
				int32_t s32Send = SSL_write(pHandle->pSSL, pHandle->c8BufSend + pHandle->u32SSLCurSend,
						pHandle->u32SendBufLength - pHandle->u32SSLCurSend);
				if (s32Send <= 0)
				{
					s32Result = SSL_get_error(pHandle->pSSL, s32Send);
					if (s32Result == SSL_ERROR_WANT_WRITE)
					{
						if (TimeGetSetupTime() > pHandle->u64EndTime)
						{
							PRINT("SSL_write timeout\n");
							CSGRNBDestroySSL(pHandle);
							return MY_ERR(_Err_TimeOut);
						}
						s32Result = CCP_STEP_NEED_REPEAT;
						return s32Result;
					}
					else
					{
						PRINT("SSL_set_fd error %d\n", s32Result);
						s32Result = MY_ERR(_Err_SSL + s32Result);
						CSGRNBDestroySSL(pHandle);
						return s32Result;
					}
				}
				pHandle->u32SSLCurSend += s32Send;
			}
			/* send head ok, initial body argument */
			pHandle->u64EndTime = TimeGetSetupTime() + pHandle->u32TimeOut;
			if (pHandle->pSendInfo->pSendBody != NULL)
			{
				int32_t s32Size = pHandle->pSendInfo->s32BodySize;
				if(s32Size < 0)
				{
					s32Size = strlen(pHandle->pSendInfo->pSendBody);
				}
				pHandle->u32SendBufLength = s32Size;
				pHandle->u32SSLCurSend = 0;
				pHandle->emCCPWithSSL = _CCP_SSL_HTTP_Send_Body;
			}
			else
			{
				/* there is no body data, initial the receive argument */
				{
					pHandle->u32SSLCurReceive = 0;
					pHandle->u32SSLCouldReceive = 0;
					pHandle->pSSLFile = tmpfile();
					pHandle->pReceive = NULL;
					if (pHandle->pSSLFile == NULL)
					{
						s32Result = MY_ERR(_Err_SYS + errno);
						PRINT("fopen error:%s\n", strerror(errno));
						CSGRNBDestroySSL(pHandle);
						return s32Result;
					}
				}
				pHandle->emCCPWithSSL = _CCP_SSL_HTTP_Receive;
				goto receive;	/* and goto receive state */
			}

		}
		/* no break */
		case _CCP_SSL_HTTP_Send_Body:
		{

			PRINT("_CCP_SSL_HTTP_Send_Body\n");

			while (pHandle->u32SSLCurSend < pHandle->u32SendBufLength)
			{
				int32_t s32Send = SSL_write(pHandle->pSSL, pHandle->pSendInfo->pSendBody + pHandle->u32SSLCurSend,
						pHandle->u32SendBufLength - pHandle->u32SSLCurSend);
				if (s32Send <= 0)
				{
					s32Result = SSL_get_error(pHandle->pSSL, s32Send);
					if (s32Result == SSL_ERROR_WANT_WRITE)
					{
						if (TimeGetSetupTime() > pHandle->u64EndTime)
						{
							PRINT("SSL_write timeout\n");
							CSGRNBDestroySSL(pHandle);
							return MY_ERR(_Err_TimeOut);
						}
						PRINT("SSL_write neep try again \n");
						s32Result = CCP_STEP_NEED_REPEAT;
						return s32Result;
					}
					else
					{
						PRINT("SSL_write error %d\n", s32Result);
						s32Result = MY_ERR(_Err_SSL + s32Result);
						CSGRNBDestroySSL(pHandle);
						return s32Result;
					}
				}
				pHandle->u32SSLCurSend += s32Send;
			}
			/* send body ok, initial receive argument */
			pHandle->u64EndTime = TimeGetSetupTime() + pHandle->u32TimeOut;

			{
				pHandle->u32SSLCurReceive = 0;
				pHandle->u32SSLCouldReceive = 0;
				pHandle->pSSLFile = tmpfile();
				pHandle->pReceive = NULL;
				if (pHandle->pSSLFile == NULL)
				{
					s32Result = MY_ERR(_Err_SYS + errno);
					PRINT("fopen error:%s\n", strerror(errno));
					CSGRNBDestroySSL(pHandle);
					return s32Result;
				}
				PRINT("tmp file number is: %d(%p)\n", fileno(pHandle->pSSLFile), pHandle->pSSLFile);
				pHandle->emCCPWithSSL = _CCP_SSL_HTTP_Receive;
			}
		}
		/* no break */
		case _CCP_SSL_HTTP_Receive:
		{
receive:

			PRINT("_CCP_SSL_HTTP_Receive\n");
			/* receive the data once as much as possible */
			while(1)
			{
				/* now, the receive space is not enough, expand it */
				if (pHandle->u32SSLCurReceive == pHandle->u32SSLCouldReceive)
				{
					int32_t s32Fd = fileno(pHandle->pSSLFile);
					if (pHandle->pReceive != NULL)			/* first, unmap the address */
					{
						munmap(pHandle->pReceive, pHandle->u32SSLCouldReceive);
					}
					pHandle->u32SSLCouldReceive += PAGE_SIZE;	/* expand one page size */
					s32Result = ftruncate(s32Fd, pHandle->u32SSLCouldReceive);		/* expand the file */
					if (s32Result != 0)
					{
						s32Result = MY_ERR(_Err_SYS + errno);
						PRINT("ftruncate error:%s\n", strerror(errno));
						fclose(pHandle->pSSLFile);
						CSGRNBDestroySSL(pHandle);
						return s32Result;
					}
					/* map the address */
					pHandle->pReceive = mmap(0, pHandle->u32SSLCouldReceive,
							PROT_READ | PROT_WRITE, MAP_SHARED, s32Fd, 0);
					if (pHandle->pReceive == NULL)
					{
						s32Result = MY_ERR(_Err_SYS + errno);
						PRINT("mmap error:%s\n", strerror(errno));
						fclose(pHandle->pSSLFile);
						CSGRNBDestroySSL(pHandle);
						return s32Result;
					}
					PRINT("mmap %d OK\n", pHandle->u32SSLCouldReceive);
				}
				else
				{
					bool boIsFinished = false;
					while (pHandle->u32SSLCurReceive < pHandle->u32SSLCouldReceive)
					{
						int32_t s32RecvTmp = SSL_read(pHandle->pSSL, (void *)((uint32_t)pHandle->pReceive +
								pHandle->u32SSLCurReceive),	pHandle->u32SSLCouldReceive - pHandle->u32SSLCurReceive);
						if (s32RecvTmp == 0)	/* the server have close the connect */
						{
							boIsFinished = true;
							break;
						}
						else if (s32RecvTmp < 0)
						{
							s32Result = SSL_get_error(pHandle->pSSL, s32RecvTmp);
							if (s32Result == SSL_ERROR_WANT_READ)
							{
								if (TimeGetSetupTime() > pHandle->u64EndTime)
								{
									PRINT("SSL_read timeout\n");
									CSGRNBTombDestroySSL(pHandle);
									return MY_ERR(_Err_TimeOut);
								}
								PRINT("SSL_read neep try again \n");
								s32Result = CCP_STEP_NEED_REPEAT;
								return s32Result;
							}
							else		/* error happened, break receiving */
							{
								PRINT("SSL_read error: %d\n", s32Result);
								CSGRNBTombDestroySSL(pHandle);
								return MY_ERR(_Err_SSL + s32Result);
								/*
								boIsFinished = true;
								break; */
							}
						}
						pHandle->u32SSLCurReceive += s32RecvTmp;
					}
					if (boIsFinished)
					{
						PRINT("finish receive\n");
						if (pHandle->pReceive != NULL)
						{
							munmap(pHandle->pReceive, pHandle->u32SSLCouldReceive);
						}
						if (pHandle->u32SSLCurReceive == 0)
						{
							fclose(pHandle->pSSLFile);
							CSGRNBDestroySSL(pHandle);
							return MY_ERR(_Err_Cloud_Data);
						}
						else
						{
							int32_t s32Fd = fileno(pHandle->pSSLFile);
							PRINT("remap: %d\n", pHandle->u32SSLCurReceive);
							pHandle->pReceive = mmap(0, pHandle->u32SSLCurReceive,
									PROT_READ | PROT_WRITE, MAP_SHARED, s32Fd, 0);
							if (pHandle->pReceive == NULL)
							{
								s32Result = MY_ERR(_Err_SYS + errno);
								PRINT("mmap error:%s\n", strerror(errno));
								fclose(pHandle->pSSLFile);
								CSGRNBDestroySSL(pHandle);
								return s32Result;
							}
							else
							{
								PRINT("read ok(%p)\n", pHandle->pSSLFile);
								pHandle->pMap->pMap = pHandle->pReceive;
								pHandle->pMap->u32MapSize = pHandle->u32SSLCurReceive;
								pHandle->pMap->pFile = pHandle->pSSLFile;
								CSGRNBDestroySSL(pHandle);
								return CCP_STEP_COMPLETE;
							}
						}
					}
				}
			}
			break;
		}
		default:
		{
			s32Result = MY_ERR(_Err_InvalidParam);
			break;
		}
	}
	return s32Result;
}
void CSGRNBDestroy(StCSGRNB *pHandle)
{
	if (pHandle == NULL)
	{
		return;
	}

	if (pHandle->emCCPWithSSL <= _CCP_SSL_DNS_Begin)
	{

	}
	else if (pHandle->emCCPWithSSL <= _CCP_SSL_SSL_Begin)
	{
		CSGRNBTombDestroyDNS(pHandle);
	}
	else if (pHandle->emCCPWithSSL <= _CCP_SSL_SSL_End)
	{
		CSGRNBTombDestroySSL(pHandle);
	}

	free(pHandle);

}

int32_t CSGRNBPoll(StCSGRNB *pHandle)
{
	int32_t s32Err = 0;
	if (pHandle == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}

	switch (pHandle->emCCPWithSSL)
	{
		case _CCP_SSL_Close:
		{
			return MY_ERR(_Err_InvalidParam);
		}
		case _CCP_SSL_DNS_Begin ... _CCP_SSL_DNS_Receive:
		{
			s32Err = CSGRNBPollGetDNS(pHandle);
			if (s32Err < 0)
			{
				pHandle->emCCPWithSSL = _CCP_SSL_Close;
				return s32Err;
			}
			else if (s32Err == CCP_STEP_COMPLETE)
			{
				pHandle->emCCPWithSSL = _CCP_SSL_DNS_End;
				goto next;	/* the return value is CCP_STEP_COMPLETE, goto next step */
			}
			break;		/* the return value is CCP_STEP_NEED_REPEAT, return for next poll */
next:
			;
		}
		/* no break */
		case _CCP_SSL_SSL_Begin ... _CCP_SSL_HTTP_Receive:
		{
			s32Err = CSGRNBPollSSL(pHandle);
			if (s32Err < 0)
			{
				pHandle->emCCPWithSSL = _CCP_SSL_Close;
				return s32Err;
			}
			else if (s32Err == CCP_STEP_COMPLETE)
			{
				pHandle->emCCPWithSSL = _CCP_SSL_Close;
			}
			break;
		}
		default:
			return MY_ERR(_Err_InvalidParam);
	}
	return s32Err;
}


#define LOACAL_IP				"10.0.0.106"

#if 1
#define SERVER_DOMAIN			"ebsnew.boc.cn"
#define SERVER_PORT				443

#define SERVER_SECOND_DOMAIN	NULL
#define SERVER_FILE				"boc15/login.html"

#define SEND_BODY				NULL
#define BODY_LENGTH				(-1)

#else

#define SERVER_DOMAIN				"203.195.202.228"
#define SERVER_PORT					8018

#define SERVER_SECOND_DOMAIN		NULL
#define SERVER_FILE					"gateway/product_data.ashx"

#define SEND_BODY					"content={ \"Sc\": \"001cfe2fe7044aa691d4e6eff9bfb56c\", \"Sv\": \"a379eee043dd4521b5376613d4fea16c\","\
        "\"QueueNum\": 123, \"IDPS\": { \"Ptl\": \"com.jiuan.HGV010\", \"Name\": \"19\", \"FVer\": \"1.1.0\", \"HVer\": \"1.1.0\", "\
        "\"MFR\": \"iHealth\", \"Model\": \"G1-100\", \"SN\": \"012345678900000B\" }, \"ProdData\": { \"Sc\": \"001cfe2fe7044aa691d4e6eff9bfb56c\","\
        "\"Sv\": \"a379eee043dd4521b5376613d4fea16c\", \"IDPS\": { \"Ptl\": \"com.jiuan.BPW10\", \"Name\": \"iHealth\", \"FVer\": \"0.0.1\", "\
        "\"HVer\": \"0.0.1\", \"MFR\": \"iHealth\", \"Model\": \"BP7 11070\", \"SN\": \"1A2B3C4D516100000000000000000000\" }, \"UserData\": [ { \"UserID\": 3, \"Data\": \"iAwXERgATDRD\n\" }, "\
        "\{ \"UserID\": 3, \"Data\": \"iAwXERsARDBH\n\" }, { \"UserID\": 3, \"Data\": \"CAwXERwAQSc8\n\" } ] } }"
#define BODY_LENGTH					(-1)
#endif
void CSGRNBPollTest(void)
{
	int32_t s32Cnt = 0;
	SSLInit();
	while (s32Cnt++ < 100)
	{
		StCloudDomain stStat = { {_Cloud_IsOnline, LOACAL_IP}, SERVER_DOMAIN, SERVER_PORT};
		StSendInfo  stInfo = { true, SERVER_SECOND_DOMAIN, SERVER_FILE, SEND_BODY, BODY_LENGTH};
		StMMap stMMap = {NULL,};
		StCSGRNB *pHandle = CSGRNBInit(&stStat, &stInfo, &stMMap, 30000, NULL);
		int32_t s32Err = 0;
		PRINT("PID is: %d\n", getpid());
		if (pHandle == NULL)
		{
			PRINT("memory error\n");
			return;
		}
		while(1)
		{
			s32Err = CSGRNBPoll(pHandle);
			if(s32Err != 0)
			{
				break;
			}
			usleep(100 * 1000);
		}
		if (s32Err == CCP_STEP_COMPLETE)
		{
			//FILE *pFile = fopen("login.jsp", "wb+");
			//fwrite(stMMap.pMap, stMMap.u32MapSize, 1, pFile);
			//fclose(pFile);
			PRINT("get return OK\n");
			CloudMapRelease(&stMMap);
		}
		CSGRNBDestroy(pHandle);
	}
	SSLDestory();

}
int32_t PoolSSLTest(void *pArg)
{
	StCSGRNB *pHandle = (StCSGRNB *)pArg;
	int s32Err = CSGRNBPoll(pHandle);
	if(s32Err != 0)
	{
		if (s32Err == CCP_STEP_COMPLETE)
		{
			//FILE *pFile = fopen("login.jsp", "wb+");
			//fwrite(stMMap.pMap, stMMap.u32MapSize, 1, pFile);
			//fclose(pFile);
			if (pHandle->pMap != NULL)
			{
				PRINT("fileno: %d(%p)\n", fileno(pHandle->pMap->pFile), pHandle->pMap->pFile);
			}
			CloudMapRelease(pHandle->pMap);
		}
		free(pHandle->pMap);
		free(pHandle->pStat);
		free(pHandle->pSendInfo);
		CSGRNBDestroy(pHandle);
	}

	return s32Err;
}
void CSGRNBPollTest1(void)
{
	int32_t s32Cnt = 0;
	int32_t s32Handle = TimeTaskInit("time.task", 40, NULL);

	StCloudDomain stStat = { {_Cloud_IsOnline, LOACAL_IP}, SERVER_DOMAIN, SERVER_PORT};
	StSendInfo  stInfo = { true, SERVER_SECOND_DOMAIN, SERVER_FILE, SEND_BODY, BODY_LENGTH};
	StMMap stMMap = {NULL,};

	//SSLInit();
	PRINT("PID is: %d\n", getpid());
	sleep(2);
	while (!g_boIsExit)
	{
		sleep(30);
		for (s32Cnt = 0; s32Cnt < 60; s32Cnt++)
		{
			StCloudDomain *pStat = malloc(sizeof(StCloudDomain));
			StSendInfo *pInfo = malloc(sizeof(StSendInfo));
			StMMap *pMMap = malloc(sizeof(StMMap));
			StCSGRNB *pHandle;

			*pStat = stStat;
			*pInfo = stInfo;
			*pMMap = stMMap;
			pHandle = CSGRNBInit(pStat, pInfo, pMMap, 10000, NULL);

			TimeTaskAddATask(s32Handle, PoolSSLTest, (void *)pHandle, 5, _Time_Task_Periodic);
			usleep(1000);
		}
	}

	TimeTaskDestory(s32Handle);

	//SSLDestory();

}

