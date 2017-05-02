/*
 * main.cpp
 *
 *  Created on: 2017年4月3日
 *      Author: lyndon
 */
#include "cmd_communication.h"

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

int main(int argc, const char *argv[])
{
	int32_t s32Err = 0;
	pthread_t s32ThreadTCPCMDParse = -1;
	pthread_t s32ThreadTCPOutCMD = -1;
	SignalRegister();

	CSockCtrl csSocketCtrl;

	StThreadArg stArg;
	stArg.pCtrl = &csSocketCtrl;

	s32Err = MakeThread(ThreadTCPOutCMDParse, &stArg, false, &s32ThreadTCPCMDParse, false);
	if (s32Err < 0)
	{
		goto end;
		return -1;
	}
	s32Err = MakeThread(ThreadTCPOutCMD, &stArg, false, &s32ThreadTCPOutCMD, false);
	if (s32Err < 0)
	{
		goto end;
		return -1;
	}

	ThreadUnixCmd(&stArg);

end:
	if (s32ThreadTCPCMDParse >= 0)
	{
		pthread_join(s32ThreadTCPCMDParse, NULL);
	}
	if (s32ThreadTCPOutCMD >= 0)
	{
		pthread_join(s32ThreadTCPOutCMD, NULL);
	}

	g_boIsExit = true;
	return s32Err;
}

