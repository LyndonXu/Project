/*
 * test.cpp
 *
 *  Created on: 2017年4月5日
 *      Author: lyndon
 */

#include <unistd.h>
#include "uart_daemon.h"

#if 0

void *ThreadEchoCntlFlush(void *pArg)
{
	CEchoCntl *pCntl = (CEchoCntl *)pArg;
	uint8_t u8Msg[8];
	int32_t i = 0;
	while (!g_boIsExit)
	{
		pCntl->Flush(((i++) * 2), u8Msg, 8);
		usleep(10 * 1000);
	}
	return NULL;
}

void EchoCntlTest(void)
{
	pthread_t s32TreadTest = -1;
	uint8_t u8Msg[8];
	CEchoCntl csCntl;
	csCntl.Init(-1);

	SignalRegister();

	MakeThread(ThreadEchoCntlFlush, &csCntl, false, &s32TreadTest, false);

	int32_t i = 0;
	for (;i < 10; i++)
	{
		CEchoInfo *pInfo = new CEchoInfo;
		if (pInfo != NULL)
		{
			pInfo->Init(u8Msg, 8, -1, i);
			csCntl.InsertAElement(pInfo);
		}
	}

	while (!g_boIsExit)
	{
		CEchoInfo *pInfo = new CEchoInfo;
		if (pInfo != NULL)
		{
			pInfo->Init(u8Msg, 8, -1, i++);
			csCntl.InsertAElement(pInfo);
		}
		usleep(1000 * 1000);
	}


	pthread_join(s32TreadTest, NULL);

	return;
}

int main(int argc, const char *argv[])
{
	EchoCntlTest();
	return 0;
}
#endif

#if 0
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>

int set_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop)
{
    struct termios newtio,oldtio;
    if  ( tcgetattr( fd,&oldtio)  !=  0)
    {
        perror("SetupSerial 1");
        return -1;
    }
#if 1
	do
	{
		int32_t *pInt = (int32_t *)(&oldtio);
		uint32_t i;
		for(i = 0; i < sizeof(oldtio) / 4; i++)
		{
			printf("%08X\n", pInt[i]);
		}
		printf("time, %02x\n", oldtio.c_cc[VTIME]);
		printf("min, %02x\n", oldtio.c_cc[VMIN]);
	} while(0);
#endif
    bzero( &newtio, sizeof( newtio ) );
    newtio.c_cflag  |=  CLOCAL | CREAD;
    newtio.c_cflag &= ~CSIZE;

    switch( nBits )
    {
    case 7:
        newtio.c_cflag |= CS7;
        break;
    case 8:
        newtio.c_cflag |= CS8;
        break;
    }

    switch( nEvent )
    {
    case 'O':                     //奇校验
        newtio.c_cflag |= PARENB;
        newtio.c_cflag |= PARODD;
        newtio.c_iflag |= (INPCK | ISTRIP);
        break;
    case 'E':                     //偶校验
        newtio.c_iflag |= (INPCK | ISTRIP);
        newtio.c_cflag |= PARENB;
        newtio.c_cflag &= ~PARODD;
        break;
    case 'N':                    //无校验
        newtio.c_cflag &= ~PARENB;
        break;
    }

switch( nSpeed )
    {
    case 2400:
        cfsetispeed(&newtio, B2400);
        cfsetospeed(&newtio, B2400);
        break;
    case 4800:
        cfsetispeed(&newtio, B4800);
        cfsetospeed(&newtio, B4800);
        break;
    case 9600:
        cfsetispeed(&newtio, B9600);
        cfsetospeed(&newtio, B9600);
        break;
    case 115200:
        cfsetispeed(&newtio, B115200);
        cfsetospeed(&newtio, B115200);
        break;
    default:
        cfsetispeed(&newtio, B9600);
        cfsetospeed(&newtio, B9600);
        break;
    }
    if( nStop == 1 )
    {
        newtio.c_cflag &=  ~CSTOPB;
    }
    else if ( nStop == 2 )
    {
        newtio.c_cflag |=  CSTOPB;
    }
    newtio.c_cc[VTIME]  = 0;
    newtio.c_cc[VMIN] = 0;
    tcflush(fd,TCIFLUSH);
    if((tcsetattr(fd,TCSANOW,&newtio))!=0)
    {
        perror("com set error");
        return -1;
    }
    printf("set done!\n");
    return 0;
}

int open_port(int fd,int comport)
{
    if (comport==1)
    {    fd = open( "/dev/ttyUSB0", O_RDWR|O_NOCTTY|O_NDELAY);
        if (-1 == fd)
        {
            perror("Can't Open Serial Port");
            return(-1);
        }
        else
        {
            printf("open ttyS0 .....\n");
        }
    }
    else if(comport==2)
    {    fd = open( "/dev/ttyS1", O_RDWR|O_NOCTTY|O_NDELAY);
        if (-1 == fd)
        {
            perror("Can't Open Serial Port");
            return(-1);
        }
        else
        {
            printf("open ttyS1 .....\n");
        }
    }
    else if (comport==3)
    {
        fd = open( "/dev/ttyS2", O_RDWR|O_NOCTTY|O_NDELAY);
        if (-1 == fd)
        {
            perror("Can't Open Serial Port");
            return(-1);
        }
        else
        {
            printf("open ttyS2 .....\n");
        }
    }
    if(fcntl(fd, F_SETFL, 0)<0)
    {
        printf("fcntl failed!\n");
    }
    else
    {
        printf("fcntl=%d\n",fcntl(fd, F_SETFL,0));
    }
    if(isatty(STDIN_FILENO)==0)
    {
        printf("standard input is not a terminal device\n");
    }
    else
    {
        printf("isatty success!\n");
    }
    printf("fd-open=%d\n",fd);
    return fd;
}

int main(void)
{
    int fd = 0;
    int nread,i;
    char buff[]="Hello\n";

    if((fd = open_port(fd,1))<0)
    {
        perror("open_port error");
        return 0;
    }
    if((i=set_opt(fd,115200,8,'N',1))<0)
    {
        perror("set_opt error");
        return 0;
    }
    printf("fd=%d\n",fd);

    {
    	uint8_t u8Buf[8] = { 0xAA, 0x00, 0x0C, 0x80, 0x00, 0x00, 0x02, 0x24};
    	write(fd, u8Buf, 8);
    	sleep(1);
    }
    nread=read(fd,buff,8);
    printf("nread=%d\n",nread);
	for (uint32_t i = 0; i < 8; i++)
	{
		printf("0x%02x ", ((uint8_t *)buff)[i]);
	}
	printf("\n");

    close(fd);
    return 0;
}
int main(int argc, const char *argv[])
{
	int32_t s32Err = 0;
	int32_t s32FDUart = open("/dev/ttyUSB0", O_RDWR);
	uint8_t u8Buf[8] = { 0xAA, 0x00, 0x0C, 0x80, 0x00, 0x00, 0x02, 0x24};

	if ((s32Err = UARTInit(s32FDUart, B115200, 0, 8, 1, 0, 10)) < 0)
	{
		PRINT("UARTInit error: 0x%08x\n", s32Err);
		goto end;
	}

	write(s32FDUart, u8Buf, 8);
	tcflush(s32FDUart, TCIOFLUSH);

	sleep(1);

	s32Err = read(s32FDUart, u8Buf, 8);
	PRINT("get some data %d\n", s32Err);

end:

	close(s32FDUart);
}
#endif


#if 0
void *ThreadRead(void *pArg)
{
	int32_t s32MsgId = GetTheMsgId(MSG_KEY_NAME);
	if (s32MsgId < 0)
	{
		return NULL;
	}
	while (!g_boIsExit)
	{
		StMsgStruct stMsg = {0};
		int32_t s32Err = 0;
		s32Err = msgrcv(s32MsgId, &stMsg, sizeof(StMsgStruct), 0, 0);
		if (s32Err < 0)
		{
			if (errno == EIDRM)
			{
				PRINT("msg(%s) is removed\n", MSG_KEY_NAME);
				break;
			}

			continue;
		}

		PRINT("get a msg, %08x-%08x\n", stMsg.u32WParam, stMsg.u32LParam);

	}

	return 0;
}


int32_t Test()
{
	pthread_t u32ThreadRecvId;
	int32_t s32Err = 0;
	int32_t s32MsgId = -1;
	SignalRegister();

	s32MsgId = GetTheMsgId(MSG_KEY_NAME);
	if (s32MsgId < 0)
	{
		return -1;
	}
	s32Err = MakeThread(ThreadRead, NULL, false, &u32ThreadRecvId, false);
	if (s32Err < 0)
	{
		ReleaseAMsgId(s32MsgId);
		return -1;
	}

	uint32_t u32Cnt = 0;
	while (!g_boIsExit)
	{
		StMsgStruct stMsg = {1, u32Cnt++};
		msgsnd(s32MsgId, &stMsg, sizeof(StMsgStruct) - offsetof(StMsgStruct, u32WParam), IPC_NOWAIT);
		sleep(1);
	}

	ReleaseAMsgId(s32MsgId);

	pthread_join(u32ThreadRecvId, NULL);


	return 0;
}
#endif



int main(int argc, char * const argv[])
{
	uint32_t u32Cmd = -1, u32Length = -1;
	const char *pDataIn = NULL;
	bool boNeedEcho = false;
	int32_t s32Char;
	while ((s32Char = getopt(argc, argv, "c:l:d:o")) != -1)
	{
		switch (s32Char)
		{
			case 'c':
			{
				sscanf(optarg, "%x", &u32Cmd);
				break;
			}
			case 'd':
			{
				pDataIn = optarg;
				break;
			}
			case 'l':
			{
				sscanf(optarg, "%d", &u32Length);
				break;
			}
			case 'o':
			{
				boNeedEcho = true;
				break;
			}
            default: /* '?' */
                fprintf(stderr, "Usage: %s [-c command number] [-l length] "
                		"[-d data] [-o need print echo]\n", argv[0]);
				return -1;
		}
	}


	if (u32Length == (uint32_t)(-1))
	{
		u32Length = strlen(pDataIn);
	}
	uint8_t *pData = (uint8_t *)malloc(u32Length);
	if (pData == NULL)
	{
		return -1;
	}
	switch (u32Cmd)
	{
		case _Unix_Cmd_Uart_Send_Data:
		case _Unix_Cmd_Uart_Send_Auth:
		{
			uint32_t u32Len = strlen(pDataIn);
			uint32_t i, j;
			for (i = 0, j = 0; i < u32Len && j < u32Length; i += 3, j++)
			{
				sscanf(pDataIn + i, "%hhx", pData + j);
				//printf("get data: %hhx\n", pData[j]);
			}
			break;
		}
		default:
			u32Cmd = _Unix_Cmd_Uart_Send_Data;
			memcpy(pData, pDataIn, u32Length);
			break;
	}

	int32_t s32Socket = ClientConnect(UNIX_SOCKET_NAME);
	if (s32Socket >= 0)
	{
		/*  */
		MCSSyncSend(s32Socket, 2000, u32Cmd, u32Length, pData);
		if (boNeedEcho)
		{
			uint8_t *pEcho = NULL;
			uint32_t u32Size;
			int32_t s32Err;
			pEcho = (uint8_t *)MCSSyncReceive(s32Socket, false, 2000, &u32Size, &s32Err);
			if (pEcho != NULL)
			{
				for (uint32_t i = 0; i < u32Size; i++)
				{
					printf("%02hhx ", pEcho[i]);
				}
				printf("\n");
				MCSSyncFree(pEcho);
			}
		}
		close(s32Socket);
	}
	else
	{
		PRINT("socket connect error: %08x\n", s32Socket);
	}

	free(pData);

	return 0;
}

/* AA 00 0C 80 00 00 02 24 */





