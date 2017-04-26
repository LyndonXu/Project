/*
 * main.cpp
 *
 *  Created on: 2017年4月3日
 *      Author: lyndon
 */
#include "cmd_communication.h"
#include <list>

using namespace std;
bool g_boIsExit = false;


void ProcessStop(int32_t s32Signal)
{
    PRINT("Signal is %d\n", s32Signal);
    g_boIsExit = true;
}
void SignalRegister(void)
{
	signal(SIGINT, ProcessStop);
	signal(SIGTERM, ProcessStop);
}



void *ThreadTCPCMD(void *pArg)
{
	int32_t s32Socket = -1;
	int32_t s32Err = 0;

	s32Socket = socket(AF_INET, SOCK_STREAM, 0);
	if (s32Socket < 0)
	{
		PRINT("socket error: %s\n", strerror(errno));
		return NULL;
	}


	/* bind to address any, and port TCP_SERVER_PORT */
	{
	    struct sockaddr_in stAddr;
	    bzero(&stAddr, sizeof(struct sockaddr_in));
	    stAddr.sin_family = AF_INET;
	    stAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	    stAddr.sin_port = htons(TCP_CMD_SERVER_PORT);

	    if(bind(s32Socket,(struct sockaddr *)&(stAddr), sizeof(struct sockaddr_in)) == -1)
	    {
			close(s32Socket);
			return NULL;
	    }
	}

    if (listen(s32Socket, 1) < 0)
    {
        PRINT("Listen the socket error: %s\n", strerror(errno));
		close(s32Socket);
		return NULL;
    }


	while (!g_boIsExit)
	{
		fd_set stSet;
		struct timeval stTimeout;

		stTimeout.tv_sec = 1;
		stTimeout.tv_usec = 0;
		FD_ZERO(&stSet);
		FD_SET(s32Socket, &stSet);

		if (select(s32Socket + 1, &stSet, NULL, NULL, &stTimeout) <= 0)
		{
		}
		else if (FD_ISSET(s32Socket, &stSet))
		{
			int32_t s32Client = ServerAccept(s32Socket);
			if (s32Client < 0)
			{
				continue;
			}
		}
	}

	return NULL;
}

int main(int argc, const char *argv[])
{
	int32_t s32Err = 0;
	SignalRegister();

	while (!g_boIsExit)
	{
		sleep(1);
	}
end:
	g_boIsExit = true;
	return s32Err;
}

