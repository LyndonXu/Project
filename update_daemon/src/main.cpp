/*
 * main.cpp
 *
 *  Created on: 2017年4月11日
 *      Author: ubuntu
 */

#include "update_daemon.h"

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



int main(int argc, char * const argv[])
{
	int32_t s32Err = 0;
	pthread_t s32ThreadUDP = -1;

	SignalRegister();

	s32Err = MakeThread(ThreadUDP, NULL, false, &s32ThreadUDP, false);
	if (s32Err < 0)
	{
		goto end;
		return -1;
	}

	while (!g_boIsExit)
	{
		sleep(1);
	}

end:
	g_boIsExit = true;

	if (s32ThreadUDP >= 0)
	{
		pthread_join(s32ThreadUDP, NULL);
	}
	return 0;
}

