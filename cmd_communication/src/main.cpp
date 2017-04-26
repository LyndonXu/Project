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
	SignalRegister();

	while (!g_boIsExit)
	{
		sleep(1);
	}
end:
	g_boIsExit = true;
	return s32Err;
}

