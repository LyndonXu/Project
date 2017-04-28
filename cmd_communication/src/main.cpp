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
	pthread_t s32ThreadCMDParse = -1;
	SignalRegister();

	CSockCtrl csSocketCtrl;

	StThreadArg stArg;
	stArg.pCtrl = &csSocketCtrl;

	s32Err = MakeThread(ThreadTCPOutCMDParse, &stArg, false, &s32ThreadCMDParse, false);
	if (s32Err < 0)
	{
		goto end;
		return -1;
	}

	ThreadTCPOutCMD(&stArg);
end:
	if (s32ThreadCMDParse >= 0)
	{
		pthread_join(s32ThreadCMDParse, NULL);
	}

	g_boIsExit = true;
	return s32Err;
}

