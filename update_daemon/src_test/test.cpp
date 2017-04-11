/*
 * test.cpp
 *
 *  Created on: 2017年4月5日
 *      Author: lyndon
 */

#include <unistd.h>
#include "update_daemon.h"

int main(int argc, char * const argv[])
{
	int32_t s32Socket = -1;
	int32_t s32Err = 0;
	uint8_t *pMCS;
	uint32_t u32Size = 0;
	if (argc < 2)
	{
		printf("%s [some string]\n", argv[0]);
		return 0;
	}
	s32Socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (s32Socket < 0)
	{
		PRINT("socket error: %s\n", strerror(errno));
		return -1;
	}


//	s32Err = RebindToEth(&s32Socket, SOCK_DGRAM, 0, INTERFACE_NAME);
//	if (s32Err < 0)
//	{
//		PRINT("RebindToEth: %08x\n", s32Err);
//		close(s32Socket);
//		return -1;
//	}
	{
		int32_t s32Enable = 1;
		s32Err = setsockopt(s32Socket, SOL_SOCKET, SO_BROADCAST, &s32Enable, sizeof(int32_t));
		if (s32Err < 0)
		{
			PRINT("setsockopt: %08x\n", errno);
			close(s32Socket);
			return -1;
		}
	}

	pMCS = (uint8_t *)MCSMakeAnArrayVarialbleCmd(7777, (argv[1]), 1, strlen(argv[1]) + 1, &u32Size);

	if (pMCS != NULL)
	{
		struct sockaddr_in stAddr = {0};
		stAddr.sin_family = AF_INET;
		stAddr.sin_port = htons(UDP_SERVER_PORT);
		stAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); //inet_addr("192.168.247.135"); //htonl(INADDR_BROADCAST);//
		s32Err = sendto(s32Socket, 	pMCS, u32Size,
				MSG_NOSIGNAL, (struct sockaddr *)(&stAddr), sizeof(struct sockaddr));
		if (s32Err < 0)
		{
			PRINT("UDP send message error: %s\n", strerror(errno));
		}

		MCSFree(pMCS);
	}
	close(s32Socket);

	return 0;

}
