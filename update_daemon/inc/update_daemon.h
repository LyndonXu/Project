/*
 * update_daemon.h
 *
 *  Created on: 2017年4月5日
 *      Author: ubuntu
 */

#ifndef _UPDATE_DAEMON_H_
#define _UPDATE_DAEMON_H_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include <termios.h>

#include "common.h"

#include <list>

using namespace std;

extern bool g_boIsExit;


#if HAS_CREOSS
#define INTERFACE_NAME	"eth0"
#else
#define INTERFACE_NAME	"wlp4s0"//"ens33"//
#endif

typedef struct _tagStNetInterfaceConfig
{
	bool boIsDHCP;
	char c8IPV4[IPV4_ADDR_LENGTH];
	char c8Mask[IPV4_ADDR_LENGTH];
	char c8Gateway[IPV4_ADDR_LENGTH];
	char c8DNS[IPV4_ADDR_LENGTH];
	char c8ReserveDNS[IPV4_ADDR_LENGTH];

	char c8MACAddr[MAC_ADDR_LENGTH];
}StNetInterfaceConfig;

typedef struct _tagStNetIfConfigInner
{
	StNetInterfaceConfig stConfig;
	int s32DHCPPid;
}StNetIfConfigInner;

typedef struct _tagStHardwareAddr
{
	char c8OldMACAddr[MAC_ADDR_LENGTH];
	char c8NewMACAddr[MAC_ADDR_LENGTH];
}StHardwareAddr;


typedef enum _tagEmUpdateMode
{
	_Update_Mode_Exe,
	_Update_Mode_GUI_Config,
	_Update_Mode_Driver,
	_Update_Mode_ThirdPaty,

	_Update_Mode_Reserved,

}EmUpdateMode;

typedef struct _tagStUpdateMode
{
	EmUpdateMode emMode;
	uint8_t u8Rand[16];
	uint8_t u8RandCode[16];
}StUpdateMode;

enum
{
	_UDP_Cmd_GetEthInfo = _MCS_Cmd_UpdateDaemon,
	_UDP_Cmd_SetEthInfo,

	_UDP_Cmd_SetMAC = _MCS_Cmd_UpdateDaemon + 0x10,


	_TCP_Cmd_Update = _MCS_Cmd_UpdateDaemon + 0x100,
	_TCP_Cmd_Update_Mode = _TCP_Cmd_Update,		/*  */
	_TCP_Cmd_Update_Name,
	_TCP_Cmd_Update_File,

	_TCP_Cmd_Update_GetVersion,
};

#if HAS_CROSS
#define PROGRAM_DIR		"/opt/program/"
#define LINK_DIR		"/opt/"
#else
#define PROGRAM_DIR		"/home/lyndon/workspace/nfsboot/program/"
#define LINK_DIR		"/home/lyndon/workspace/nfsboot/"
#endif

#define HW_ADDR_CONFIG			PROGRAM_DIR"HWAddrConfig"
#define ETH_ADDR_CONFIG			PROGRAM_DIR"ETHAddrConfig.json"

#define DRIVER_DIR				PROGRAM_DIR"Driver"
#define THIRD_PARTY_DIR			PROGRAM_DIR"ThirdParty"
#define GUI_CONFIG_DIR			PROGRAM_DIR"GuiConfig"
#define EXE_DIR					PROGRAM_DIR"Exe"

#define LINK_DRIVER_DIR			LINK_DIR"driver_cur"
#define LINK_THIRD_PARTY_DIR	LINK_DIR"third_party_cur"
#define LINK_GUI_CONFIG_DIR		LINK_DIR"gui_config_cur"
#define LINK_EXE_DIR			LINK_DIR"exe_cur"


void *ThreadUDP(void *pArg);

#endif /* UARTDAEMON_UART_DAEMON_H_ */
