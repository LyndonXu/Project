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

#define UPDATE_ERR(code)	ERROR_CODE(0x16, (code))

extern bool g_boIsExit;

#ifdef UBUNTU
//#warning "ubuntu platform"
#else
//#warning "fedroa platform"
#endif

#if HAS_CROSS
#define INTERFACE_NAME	"eth0"
#else

#ifdef UBUNTU
#define INTERFACE_NAME	"ens33"//
#else
#define INTERFACE_NAME	"wlp4s0"//"ens33"//
#endif

#endif
#define BTEA_CODE_LENGTH	16
typedef struct _tagStBteaCheck
{
	uint8_t u8Rand[BTEA_CODE_LENGTH];
	uint8_t u8RandCode[BTEA_CODE_LENGTH];
}StBteaCheck;


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

typedef struct _tagStSetNetConfig
{
	StBteaCheck stCheck;
	StNetInterfaceConfig stConfig;
}StSetNetConfig;

typedef struct _tagStSetMacAddr
{
	StBteaCheck stCheck;
	StHardwareAddr stConfig;
}StSetMacAddr;

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
	StBteaCheck stCheck;
}StUpdateMode;

typedef struct _tagStUpdateVersion
{
	char c8Name[256];
	int32_t s32Version;
}StUpdateVersion;

typedef struct _tagStRollBack
{
	StUpdateMode stMode;
	StUpdateVersion stVersion;
}StRollBack;

enum
{
	_UDP_Cmd_GetEthInfo = _MCS_Cmd_UpdateDaemon,	/* payload none, echo _MCS_Cmd_Echo | _UDP_Cmd_GetEthInfo with data StNetInterfaceConfig */
	_UDP_Cmd_SetEthInfo,							/* payload StSetNetConfig, echo _MCS_Cmd_Echo | _UDP_Cmd_SetEthInfo with data StNetInterfaceConfig */

	_UDP_Cmd_SetMAC = _MCS_Cmd_UpdateDaemon + 0x10, /* payload StSetMacAddr, echo _MCS_Cmd_Echo | _UDP_Cmd_SetEthInfo with no data */


	_TCP_Cmd_Update = _MCS_Cmd_UpdateDaemon + 0x100,

	_TCP_Cmd_Update_Mode = _TCP_Cmd_Update,		/* StUpdateMode, echo _MCS_Cmd_Echo | _TCP_Cmd_Update_Mode with no data */
	_TCP_Cmd_Update_Name,						/* strlen(name) + 1, echo _MCS_Cmd_Echo | _TCP_Cmd_Update_Name with no data */
	_TCP_Cmd_Update_File,						/* length of the update file, echo _MCS_Cmd_Echo | _TCP_Cmd_Update_File with no data */

	_TCP_Cmd_Update_GetVersion,					/* StUpdateMode, echo _MCS_Cmd_Echo | _TCP_Cmd_Update_GetVersion with StUpdateVersion * N */
	_TCP_Cmd_Update_RollBack,					/* StRollBack */

};

#if HAS_CROSS
#define PROGRAM_DIR		"/opt/program/"
#define LINK_DIR		"/opt/"
#else

#ifdef UBUNTU
#define PROGRAM_DIR		"/home/ubuntu/workspace/nfsboot/program/"
#define LINK_DIR		"/home/ubuntu/workspace/nfsboot/"
#else
#define PROGRAM_DIR		"/home/lyndon/workspace/nfsboot/program/"
#define LINK_DIR		"/home/lyndon/workspace/nfsboot/"
#endif
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

#define VERSION_LIST_FILE			"VersionList.json"
#define MAX_RESERVED_VERSION_CNT	1

void *ThreadUDP(void *pArg);
void *ThreadTCPUpdate(void *pArg);

class CMCSAutoRelease
{
public:
	void *m_pMCS;
	CMCSAutoRelease()
		:m_pMCS(NULL)
	{};
	CMCSAutoRelease(void *pMCS)
	{
		m_pMCS = pMCS;
	};

	~CMCSAutoRelease()
	{
		if (m_pMCS != NULL)
		{
			free(m_pMCS);
		}
	}
};

#endif /* UARTDAEMON_UART_DAEMON_H_ */
