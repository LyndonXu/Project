/****************************************************************************
 * Copyright(c), 2001-2060, ******************************* 版权所有
 ****************************************************************************
 * 文件名称             : sundries_thread.c
 * 版本                 : 0.0.1
 * 作者                 : 许龙杰
 * 创建日期             : 2014年12月12日
 * 描述                 : 
 ****************************************************************************/
#include "common_define.h"
#include <arpa/inet.h>

#include "../inc/common.h"
#include "json/json.h"

/* get the interface configure from json */
static void LoadNetInterface(json_object *pSonObj, StNetInterfaceConfig *pStruct)
{
	json_object *pTmp = json_object_object_get(pSonObj, "DHCP");
	if (pTmp != NULL)
	{
		pStruct->boIsDHCP = json_object_get_int(pTmp);
	}
	if (!pStruct->boIsDHCP)
	{
		pTmp = json_object_object_get(pSonObj, "IPAddress");
		if (pTmp != NULL)
		{
			strncpy(pStruct->c8IPV4, json_object_get_string(pTmp), IPV4_ADDR_LENGTH - 1);
		}
		pTmp = json_object_object_get(pSonObj, "Mask");
		if (pTmp != NULL)
		{
			strncpy(pStruct->c8Mask, json_object_get_string(pTmp), IPV4_ADDR_LENGTH - 1);
		}
		pTmp = json_object_object_get(pSonObj, "Gateway");
		if (pTmp != NULL)
		{
			strncpy(pStruct->c8Gateway, json_object_get_string(pTmp), IPV4_ADDR_LENGTH - 1);
		}
		pTmp = json_object_object_get(pSonObj, "DNS");
		if (pTmp != NULL)
		{
			strncpy(pStruct->c8DNS, json_object_get_string(pTmp), IPV4_ADDR_LENGTH - 1);
		}
		pTmp = json_object_object_get(pSonObj, "ReserveDNS");
		if (pTmp != NULL)
		{
			strncpy(pStruct->c8ReserveDNS, json_object_get_string(pTmp), IPV4_ADDR_LENGTH - 1);
		}
	}
}

/* get configure from configure file */
int32_t LoadConfingFile(StGlobeConfig *pConfig)
{
	json_object *pObj = NULL;
	if (pConfig == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}

	pObj = json_object_from_file(CONFIG_FILE);
	if (pObj == NULL)
	{
		return MY_ERR(_Err_JSON);
	}
	PRINT("\n%s\n", json_object_to_json_string_ext(pObj, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY));
	do
	{
		json_object *pSonObj = NULL;
		pSonObj = json_object_object_get(pObj, "Ethernet");
		if (pSonObj == NULL)
		{
			break;
		}
		LoadNetInterface(pSonObj, &pConfig->stETHAddr);

	}while(0);

	do
	{
		json_object *pSonObj = NULL;
		pSonObj = json_object_object_get(pObj, "Wifi");
		if (pSonObj == NULL)
		{
			break;
		}
		do
		{
			json_object *pTmp = NULL;
			pTmp = json_object_object_get(pSonObj, "WifiName");
			if (pTmp != NULL)
			{
				strncpy(pConfig->stWifi.c8SSID, json_object_get_string(pTmp), 64 - 1);
				PRINT("SSID: %s\n", pConfig->stWifi.c8SSID);
			}
			pTmp = json_object_object_get(pSonObj, "WifiKey");
			if (pTmp != NULL)
			{
				strncpy(pConfig->stWifi.c8PSK, json_object_get_string(pTmp), 64 - 1);
				PRINT("PSK: %s\n", pConfig->stWifi.c8PSK);
			}
		}while(0);
		LoadNetInterface(pSonObj, &pConfig->stWifi.stWifiAddr);

	}while(0);

	do
	{
		json_object *pSonObj = NULL;
		pSonObj = json_object_object_get(pObj, "TimeZoneDST");
		if (pSonObj == NULL)
		{
			break;
		}
		do
		{
			json_object *pTmp = NULL;
			pTmp = json_object_object_get(pSonObj, "TimeZone");
			if (pTmp != NULL)
			{
				pConfig->stTimeZoneDST.s8TimeZone = json_object_get_int(pTmp);
			}
			pTmp = json_object_object_get(pSonObj, "DST");
			if (pTmp != NULL)
			{
				pConfig->stTimeZoneDST.boIsDST = json_object_get_int(pTmp);
			}
		}while(0);
	}while(0);

	do
	{
		json_object *pSonObj = NULL;
		pSonObj = json_object_object_get(pObj, "TS");
		if (pSonObj == NULL)
		{
			break;
		}
		pConfig->u64TS = json_object_get_int64(pSonObj);
	}while(0);

	json_object_put(pObj);
	return 0;
}

/* save structure to json */
static void JsonAddInterfaceConfig(json_object *pObj, StNetInterfaceConfig *pConfig)
{
	json_object_object_add(pObj, "DHCP", json_object_new_int(pConfig->boIsDHCP));
	json_object_object_add(pObj, "IPAddress", json_object_new_string(pConfig->c8IPV4));
	json_object_object_add(pObj, "Mask", json_object_new_string(pConfig->c8Mask));
	json_object_object_add(pObj, "Gateway", json_object_new_string(pConfig->c8Gateway));
	json_object_object_add(pObj, "DNS", json_object_new_string(pConfig->c8DNS));
	json_object_object_add(pObj, "ReserveDNS", json_object_new_string(pConfig->c8ReserveDNS));
}

/* save structure to configure file */
int32_t SaveConfingFile(StGlobeConfig *pConfig)
{
	int32_t s32Err = 0;
	json_object *pObj = NULL, *pSonObj = NULL;
	if (pConfig == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	pObj = json_object_new_object();
	if (pObj == NULL)
	{
		return MY_ERR(_Err_Mem);
	}

	pSonObj = json_object_new_object();
	if (pObj == NULL)
	{
		s32Err = MY_ERR(_Err_Mem);
		goto end;
	}

	JsonAddInterfaceConfig(pSonObj, &pConfig->stETHAddr);
	json_object_object_add(pObj, "Ethernet", pSonObj);

	pSonObj = json_object_new_object();
	if (pObj == NULL)
	{
		s32Err = MY_ERR(_Err_Mem);
		goto end;
	}
	json_object_object_add(pSonObj, "WifiName", json_object_new_string(pConfig->stWifi.c8PSK));
	json_object_object_add(pSonObj, "WifiKey", json_object_new_string(pConfig->stWifi.c8SSID));
	JsonAddInterfaceConfig(pSonObj, &pConfig->stWifi.stWifiAddr);

	json_object_object_add(pObj, "Wifi", pSonObj);

	pSonObj = json_object_new_object();
	if (pObj == NULL)
	{
		s32Err = MY_ERR(_Err_Mem);
		goto end;
	}
	json_object_object_add(pSonObj, "TimeZone", json_object_new_int(pConfig->stTimeZoneDST.s8TimeZone));
	json_object_object_add(pSonObj, "DST", json_object_new_int(pConfig->stTimeZoneDST.boIsDST));

	json_object_object_add(pObj, "TimeZoneDST", pSonObj);
	json_object_object_add(pObj, "TS", json_object_new_int64(pConfig->u64TS));

	json_object_to_file_ext(CONFIG_FILE, pObj, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY);

end:
	json_object_put(pObj);
	return s32Err;
}

/* compare cmdline file to get the process's PID, if find it, return the PID(positive number) */
int32_t CompareCmdlineCB(const char *pCurPath, struct dirent *pInfo, void *pContext)
{
	char c8Buf[64];
	int32_t s32Fd;
	int32_t s32Tmp, i;
	const char **p2KeyStr = (const char **)pContext;

	/* not a process directory */
	if ((pInfo->d_name[0] > '9') || (pInfo->d_name[0] < '0'))
	{
		return 0;
	}

	sprintf(c8Buf, "%s%s/cmdline", pCurPath, pInfo->d_name);
	s32Fd = open(c8Buf, O_RDONLY);
	if (s32Fd < 0)
	{
		return 0;
	}

	s32Tmp = read(s32Fd, c8Buf, 62);
	close(s32Fd);
	if (s32Tmp < 0)
	{
		return 0;
	}
	for (i = 0; i < s32Tmp; i++)
	{
		if (c8Buf[i] == 0)
		{
			c8Buf[i] = ' ';
		}
	}
	if (s32Tmp < 20)
	{
		return 0;
	}
	c8Buf[s32Tmp + 1] = 0;
	PRINT("%s(%d)\n", c8Buf, s32Tmp);

	s32Tmp = 0;
	while (p2KeyStr[s32Tmp] != NULL)
	{
		/* the key work is not matching */
		if (strstr(c8Buf, p2KeyStr[s32Tmp]) == NULL)
		{
			return 0;
		}
		s32Tmp++;
	}
	return atoi(pInfo->d_name);
}

/* config the network interface from the configure structure */
void SetNetInterface(const char *pInterface, StNetInterfaceConfig *pStruct)
{
	char c8Str[128];
	if (pStruct->boIsDHCP)	/* for DHCP */
	{
		int32_t s32Err = 0;
		const char *pKeyStr[3] = {"udhcpc", pInterface, NULL};

		/* first, check whether the process had been run */
		s32Err = TraversalDir("/proc/", false, CompareCmdlineCB, (void *)pKeyStr);
		if (s32Err > 0)		/* we have set up it */
		{
			pStruct->s32DHCPPid = s32Err;
			sprintf(c8Str, "kill -SIGUSR1 %d", s32Err);
			PRINT("%s\n", c8Str);
			system(c8Str);	/* force a renew state */
			PRINT("udhcpc for %s had been set up and the PID is: %d\n", pInterface, s32Err);
		}
		else
		{
			/* run it */
			sprintf(c8Str, "udhcpc -b -i %s -s /sbin/udhcpc.sh", pInterface);
			PRINT("%s\n", c8Str);
			system(c8Str);

			/* get the PID */
			s32Err = TraversalDir("/proc/", false, CompareCmdlineCB, (void *)pKeyStr);
			if (s32Err > 0)
			{
				pStruct->s32DHCPPid = s32Err;
				PRINT("PID is: %d\n", s32Err);
			}
		}

	}
	else	/* for manual IP */
	{
		if (inet_addr(pStruct->c8IPV4) != INADDR_NONE)
		{
			sprintf(c8Str, "ifconfig %s %s", pInterface, pStruct->c8IPV4);
			PRINT("%s\n", c8Str);
			system(c8Str);
			if (inet_addr(pStruct->c8Mask) != INADDR_NONE)
			{
				sprintf(c8Str, "ifconfig %s netmask %s", pInterface, pStruct->c8Mask);
				PRINT("%s\n", c8Str);
				system(c8Str);
			}
			if (inet_addr(pStruct->c8Gateway) != INADDR_NONE)
			{
				sprintf(c8Str, "route add default gw %s dev %s metric 1", pStruct->c8Gateway, pInterface);
				PRINT("%s\n", c8Str);
				system(c8Str);
			}
			if (inet_addr(pStruct->c8DNS) != INADDR_NONE)
			{
				sprintf(c8Str, "echo nameserver %s > /etc/resolv.conf", pStruct->c8DNS);
				PRINT("%s\n", c8Str);
				system(c8Str);
			}
			if (inet_addr(pStruct->c8ReserveDNS) != INADDR_NONE)
			{
				sprintf(c8Str, "echo nameserver %s >> /etc/resolv.conf", pStruct->c8ReserveDNS);
				PRINT("%s\n", c8Str);
				system(c8Str);
			}
		}
	}
}

/* set up WPA server */
int32_t SetUpWPA(StGlobeConfig *pConfig, bool boIsUp)
{
	int32_t s32Err = 0;

	if ((pConfig->stWifi.s32WPAPid > 0) && (!boIsUp))
	{
		const char *pBuf = WPARequest(_WPA_CIC_Terminate, NULL, NULL, &s32Err);
		if (pBuf == NULL)
		{
			return s32Err;
		}
		if (strncmp(pBuf, "OK", strlen("OK")) != 0)
		{
			free((void *)pBuf);
			PRINT("%s\n", pBuf);
			return MY_ERR(_Err_Common);
		}
		free((void *)pBuf);

		/* reset the interface */
		system("ifconfig "ETH_WIFI_NAME" down");
		sleep(1);
		system("ifconfig "ETH_WIFI_NAME" up");
		sleep(1);
		pConfig->stWifi.s32WPAPid = 0;
		return 0;
	}
	else if ((pConfig->stWifi.s32WPAPid <= 0) && boIsUp)
	{
		const char *pKeyStr[3] = {"wpa_supplicant", ETH_WIFI_NAME, NULL};
		s32Err = TraversalDir("/proc/", false, CompareCmdlineCB, (void *)pKeyStr);
		if (s32Err > 0)
		{
			pConfig->stWifi.s32WPAPid = s32Err;
			PRINT("wpa_supplicant had been set up and the PID is: %d\n", s32Err);
			s32Err = 0;
		}
		else
		{
			system("ifconfig "ETH_WIFI_NAME" down");
			sleep(1);
			system("ifconfig "ETH_WIFI_NAME" up");
			sleep(1);
			system("wpa_supplicant -c/media/sdcard/Run/wpa_supplicant.conf -i"ETH_WIFI_NAME" -B -Dralink");

			do
			{
				s32Err = TraversalDir("/proc/", false, CompareCmdlineCB, (void *)pKeyStr);
				if (s32Err > 0)
				{
					pConfig->stWifi.s32WPAPid = s32Err;
					PRINT("PID is: %d\n", s32Err);
					s32Err = 0;
				}
			}while(0);
		}
	}
	return s32Err;
}

/* check whether the WIFI is connect */
int32_t CheckWifiConnect(void)
{
	int32_t s32Err = 0;
	const char *pBuf = WPARequest(_WPA_CIC_Status, NULL, NULL, &s32Err);
	if (pBuf == NULL)
	{
		return s32Err;
	}
	/* congratulate :), the connect is completed */
	if (strstr(pBuf, "wpa_state=COMPLETED") != NULL)
	{
		free((void *)pBuf);
		return 0;
	}
	free((void *)pBuf);
	return MY_ERR(_Err_Common);
}


/* set the WIFI interface SSID and PSK(password), error happened return negative; set complete,
 * but is connecting, return zero; set complete and connect complete, return positive  */
int32_t SetWifiSSIDAndPSK(const char *pSSID, const char *pPSK)
{
	int32_t s32Err = 0;
	int32_t s32NetworkNum = -1;
	const char *pBuf;

	do	/* first, check whether the SSID in the list */
	{
		const char *pTmp;
		pBuf = WPARequest(_WPA_CIC_List_Networks, NULL, NULL, &s32Err);
		if (pBuf == NULL)
		{
			return s32Err;
		}
		pTmp = strchr(pBuf, '\n');
		if (pTmp == NULL)
		{
			goto next;
		}
		if (pTmp[1] == 0)
		{
			goto next;
		}

		pTmp = strstr(pTmp, pSSID);		/* check whether the SSID is in the list */
		if (pTmp == NULL)
		{
			goto next;
		}
		while (*pTmp != '\n')	/* find the network number's position */
		{
			pTmp--;
		}
		s32NetworkNum = atoi(pTmp + 1);		/* get the network's number */

		pTmp = strstr(pTmp, "CURRENT");
		if (pTmp == NULL)
		{
			PRINT("I find the network %s, but it's not connect\n", pSSID);
			free((void *)pBuf);
			goto set_psk;
		}
		else
		{
			if (CheckWifiConnect() == 0)
			{
				PRINT("the network is %s, and it's OK\n", pSSID);
				free((void *)pBuf);
				return 1;
			}
		}

next:
		free((void *)pBuf);

	}while(0);

	do	/* add a new network */
	{
		pBuf = WPARequest(_WPA_CIC_Add_Network, NULL, NULL, &s32Err);
		if (pBuf == NULL)
		{
			return s32Err;
		}
		s32NetworkNum = atoi(pBuf);
		free((void *)pBuf);

	}while(0);

	do	/* set the SSID for this new network */
	{
		StArgv stArgv;
		char c8Tmp[128];
		char c8Tmp2[32];
		const char *pStr[3];
		sprintf(c8Tmp, "\"%s\"", pSSID);
		sprintf(c8Tmp2, "%d", s32NetworkNum);
		pStr[0] = c8Tmp2;
		pStr[1] = "ssid";
		pStr[2] = c8Tmp;
		stArgv.p2Argv = pStr;
		stArgv.u32Cnt = 3;
		pBuf = WPARequest(_WPA_CIC_Set_Network, NULL, &stArgv, &s32Err);
	}while(0);
	if (pBuf == NULL)
	{
		return s32Err;
	}
	if (strncmp(pBuf, "OK", strlen("OK")) != 0)
	{
		free((void *)pBuf);
		PRINT("%s\n", pBuf);
		return MY_ERR(_Err_Common);
	}
	free((void *)pBuf);

set_psk:
	do	/* set the PSK for this new network */
	{
		StArgv stArgv;
		char c8Tmp[128];
		char c8Tmp2[32];
		const char *pStr[3];
		sprintf(c8Tmp, "\"%s\"", pPSK);
		sprintf(c8Tmp2, "%d", s32NetworkNum);
		pStr[0] = c8Tmp2;
		pStr[1] = "psk";
		pStr[2] = c8Tmp;
		stArgv.p2Argv = pStr;
		stArgv.u32Cnt = 3;
		pBuf = WPARequest(_WPA_CIC_Set_Network, NULL, &stArgv, &s32Err);
	}while(0);
	if (pBuf == NULL)
	{
		return s32Err;
	}
	if (strncmp(pBuf, "OK", strlen("OK")) != 0)
	{
		free((void *)pBuf);
		PRINT("%s\n", pBuf);
		return MY_ERR(_Err_Common);
	}
	free((void *)pBuf);

	do		/* enable the network */
	{
		StArgv stArgv;
		char c8Tmp2[32];
		const char *pStr[1];
		sprintf(c8Tmp2, "%d", s32NetworkNum);
		pStr[0] = c8Tmp2;
		stArgv.p2Argv = pStr;
		stArgv.u32Cnt = 1;
		pBuf = WPARequest(_WPA_CIC_Enable_Network, NULL, &stArgv, &s32Err);
	}while(0);
	if (pBuf == NULL)
	{
		return s32Err;
	}
	if (strncmp(pBuf, "OK", strlen("OK")) != 0)
	{
		free((void *)pBuf);
		PRINT("%s\n", pBuf);
		return MY_ERR(_Err_Common);
	}
	free((void *)pBuf);

	do		/* select the network  */
	{
		StArgv stArgv;
		char c8Tmp2[32];
		const char *pStr[1];
		sprintf(c8Tmp2, "%d", s32NetworkNum);
		pStr[0] = c8Tmp2;
		stArgv.p2Argv = pStr;
		stArgv.u32Cnt = 1;
		pBuf = WPARequest(_WPA_CIC_Select_Network, NULL, &stArgv, &s32Err);
	}while(0);
	if (pBuf == NULL)
	{
		return s32Err;
	}
	if (strncmp(pBuf, "OK", strlen("OK")) != 0)
	{
		free((void *)pBuf);
		PRINT("%s\n", pBuf);
		return MY_ERR(_Err_Common);
	}
	free((void *)pBuf);
	return 0;
}


const StGlobeConfig c_stDefaultConfig =
{
	.stETHAddr =
	{
		.boIsDHCP = true,
		.c8IPV4 = "",
		.c8Mask = "",
		.c8Gateway = "",
		.c8DNS = "",
		.c8ReserveDNS = "",
	},
	.stWifi =
	{
		.c8SSID = "",
		.c8PSK = "",
		.s32WPAPid = 0,
		.stWifiAddr =
		{
			.boIsDHCP = true,
			.c8IPV4 = "",
			.c8Mask = "",
			.c8Gateway = "",
			.c8DNS = "",
			.c8ReserveDNS = "",
		},
	},
	.stTimeZoneDST =
	{
		.s8TimeZone = -8,
		.boIsDST = false,
	},
	.u64TS = 0,
};


static int32_t MCSCallBack(uint32_t u32CmdNum, uint32_t u32CmdCnt, uint32_t u32CmdSize,
        const char *pCmdData, void *pContext)
{
	int32_t s32Err = MY_ERR(_Err_CmdType);
	StGlobeConfig *pConfig = (StGlobeConfig *)pContext;
	int32_t s32Client = (int32_t)(pConfig->pContext);
	switch (u32CmdNum)
	{
		case _MCS_Cmd_Inner_SetWifiSSIDAndPSK:
		{
			StMCIWIFISSIDAndPSK *pWIFI = (StMCIWIFISSIDAndPSK *)pCmdData;
			s32Err = MY_ERR(_Err_Common);
			PRINT("get a set WIFI SSID and PSK command\n");
			if (pConfig->stWifi.s32WPAPid > 0)
			{
				s32Err = SetWifiSSIDAndPSK(pWIFI->pSSID, pWIFI->pPSK);
				if (s32Err >= 0)
				{
					s32Err = 0;
				}
			}
			else
			{
				PRINT("the WPA server isn't in daemon\n");
			}
			s32Err = MCSSyncSend(s32Client, 1000, _MCS_Cmd_Echo | _MCS_Cmd_Inner_SetWifiSSIDAndPSK,
							sizeof(int32_t), &s32Err);
			break;

		}
		case _MCS_Cmd_Inner_GetWifiConnectStatus:
		{
			s32Err = MY_ERR(_Err_Common);
			PRINT("get a check WIFI connect status command\n");
			if (pConfig->stWifi.s32WPAPid > 0)
			{
				s32Err = CheckWifiConnect();
			}
			else
			{
				PRINT("the WPA server isn't in daemon\n");
			}
			s32Err = MCSSyncSend(s32Client, 1000, _MCS_Cmd_Echo | _MCS_Cmd_Inner_GetWifiConnectStatus,
							sizeof(int32_t), &s32Err);
			break;
		}
		case _MCS_Cmd_Inner_LoadEthConfig:
		{
			s32Err = GetNetLinkStatus(ETH_LAN_NAME);
			if (s32Err < 0)
			{
				goto load_eth_config_end;
			}
			SetNetInterface(ETH_LAN_NAME, &(pConfig->stETHAddr));
load_eth_config_end:
			s32Err = MCSSyncSend(s32Client, 1000, _MCS_Cmd_Echo | _MCS_Cmd_Inner_LoadEthConfig,
							sizeof(int32_t), &s32Err);
			break;
		}
		case _MCS_Cmd_Inner_ClearEth:
		{
			char c8Str[64];
			if (pConfig->stETHAddr.s32DHCPPid != 0)
			{
				sprintf(c8Str, "kill -SIGUSR2 %d", pConfig->stETHAddr.s32DHCPPid);
				PRINT("%s\n", c8Str);
				system(c8Str);	/*
								 * force a release of the current lease,
								 * and cause udhcpc to go into an inactive state (until it is killed,
								 * or receives a SIGUSR1)
								 */
			}
			sprintf(c8Str, "ifconfig %s 0.0.0.0", ETH_LAN_NAME);
			PRINT("%s\n", c8Str);
			system(c8Str);
			s32Err = 0;
			s32Err = MCSSyncSend(s32Client, 1000, _MCS_Cmd_Echo | _MCS_Cmd_Inner_ClearEth,
							sizeof(int32_t), &s32Err);
			break;
		}
		case _MCS_Cmd_Inner_LoadWifiConfig_Connect:
		{
			s32Err = MY_ERR(_Err_Common);
			if (pConfig->stWifi.s32WPAPid > 0)
			{
				if ((strlen(pConfig->stWifi.c8SSID) != 0) && (strlen(pConfig->stWifi.c8PSK) != 0))
				{
					s32Err = SetWifiSSIDAndPSK(pConfig->stWifi.c8SSID, pConfig->stWifi.c8PSK);
				}
			}
			s32Err = MCSSyncSend(s32Client, 1000, _MCS_Cmd_Echo | _MCS_Cmd_Inner_LoadWifiConfig_Connect,
							sizeof(int32_t), &s32Err);
			break;
		}
		case _MCS_Cmd_Inner_LoadWifiConfig_SetIp:
		{
			if (CheckWifiConnect() < 0)
			{
				s32Err = MY_ERR(_Err_Common);
				goto load_wifi_config_set_ip_end;
			}
			SetNetInterface(ETH_WIFI_NAME, &(pConfig->stWifi.stWifiAddr));
load_wifi_config_set_ip_end:
			s32Err = MCSSyncSend(s32Client, 1000, _MCS_Cmd_Echo | _MCS_Cmd_Inner_LoadWifiConfig_SetIp,
							sizeof(int32_t), &s32Err);
			break;
		}
		case _MCS_Cmd_Inner_ClearWifi:
		{
			char c8Str[64];
			if (pConfig->stWifi.stWifiAddr.s32DHCPPid != 0)
			{
				sprintf(c8Str, "kill -SIGUSR2 %d", pConfig->stWifi.stWifiAddr.s32DHCPPid);
				PRINT("%s\n", c8Str);
				system(c8Str);	/*
								 * force a release of the current lease,
								 * and cause udhcpc to go into an inactive state (until it is killed,
								 * or receives a SIGUSR1)
								 */
			}
			sprintf(c8Str, "ifconfig %s 0.0.0.0", ETH_WIFI_NAME);
			PRINT("%s\n", c8Str);
			system(c8Str);
			s32Err = 0;
			s32Err = MCSSyncSend(s32Client, 1000, _MCS_Cmd_Echo | _MCS_Cmd_Inner_ClearWifi,
							sizeof(int32_t), &s32Err);
			break;
		}
		default:
			break;
	}
	return s32Err;
}

/*
 * thread for error information
 */
void *ThreadSundries(void *pArg)
{
	int32_t s32SundriesServer = -1;
	int32_t s32Err = 0;
	StGlobeConfig stConfig = c_stDefaultConfig;

	s32Err = LoadConfingFile(&stConfig);
	if (s32Err < 0)
	{
		PRINT("LoadConfingFile error 0x%08x\n", s32Err);
	}
#if HAS_CROSS
	s32Err = SetUpWPA(&stConfig, true);
	if (s32Err != 0)
	{
		PRINT("SetUpWPA error: 0x%08x\n", s32Err);
	}
	do
	{
		char c8Str[64];
		if (stConfig.stTimeZoneDST.boIsDST)
		{
			sprintf(c8Str, "echo STD%dDST> /etc/TZ", stConfig.stTimeZoneDST.s8TimeZone);
		}
		else
		{
			sprintf(c8Str, "echo STD%d> /etc/TZ", stConfig.stTimeZoneDST.s8TimeZone);
		}
		system(c8Str);
	}while(0);
#endif
#if 0
	/* the SSID and the PSK is valid */
	if ((strlen(stConfig.stWifi.c8SSID) != 0) && (strlen(stConfig.stWifi.c8PSK) != 0))
	{
		s32Err = SetUpWPA(&stConfig, true);
		if (s32Err == 0)
		{
			s32Err = SetWifiSSIDAndPSK(stConfig.stWifi.c8SSID, stConfig.stWifi.c8PSK);
			if (s32Err > 0)
			{
				SetNetInterface(ETH_WIFI_NAME, &(stConfig.stWifi.stWifiAddr));
			}
			else if (s32Err == 0)
			{
				int32_t s32Cnt = 0;
				sleep(4);
				do
				{
					sleep(1);
					s32Err = CheckWifiConnect();
					s32Cnt++;
				}while((s32Err != 0) && (s32Cnt < 5));
				if (s32Err == 0)
				{
					SetNetInterface(ETH_WIFI_NAME, &(stConfig.stWifi.stWifiAddr));
				}
			}
		}
	}

	SetNetInterface(ETH_LAN_NAME, &(stConfig.stETHAddr));
#endif
	s32SundriesServer = ServerListen(SUNDRIES_SOCKET);
	if (s32SundriesServer < 0)
	{
		PRINT("ServerListen error: 0x%08x", s32SundriesServer);
		return NULL;
	}

	while (!g_boIsExit)
	{
		fd_set stSet;
		struct timeval stTimeout;
		int32_t s32Client = -1;

		stTimeout.tv_sec = 1;
		stTimeout.tv_usec = 0;
		FD_ZERO(&stSet);
		FD_SET(s32SundriesServer, &stSet);

		if (select(s32SundriesServer + 1, &stSet, NULL, NULL, &stTimeout) <= 0)
		{
			continue;
		}
		if (!FD_ISSET(s32SundriesServer, &stSet))
		{
			continue;
		}
		s32Client = ServerAccept(s32SundriesServer);
		/* create detach process thread */
		if (s32Client < 0)
		{
			PRINT("ServerAccept error: 0x%08x\n", s32Client);
			continue;
		}
		else
		{
			uint32_t u32Size = 0;
			void *pMCSStream;
			pMCSStream = MCSSyncReceive(s32Client, true, 1000, &u32Size, &s32Err);

			if (pMCSStream != NULL)
			{
				stConfig.pContext = (void *)s32Client;
				s32Err = MCSResolve((const char *)pMCSStream, u32Size, MCSCallBack, (void *)(&stConfig));
				MCSSyncFree(pMCSStream);
			}
			else
			{
				PRINT("MCSSyncReceive error 0x%08x\n", s32Err);
			}
		}

		close(s32Client);
	}
	ServerRemove(s32SundriesServer, SUNDRIES_SOCKET);
	return NULL;
}
