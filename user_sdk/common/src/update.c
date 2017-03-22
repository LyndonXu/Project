/****************************************************************************
 * Copyright(c), 2001-2060, ******************************* 版权所有
 ****************************************************************************
 * 文件名称             : update.c
 * 版本                 : 0.0.1
 * 作者                 : 许龙杰
 * 创建日期             : 2015年1月15日
 * 描述                 : 
 ****************************************************************************/
#include "common_define.h"
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ctype.h>

#include "../inc/common.h"
#include "openssl/crypto.h"
#include "openssl/ssl.h"
#include "openssl/err.h"
#include "openssl/rand.h"


#include "json/json.h"

/* release the source */
void UpdateFileInfoRelease(StUpdateFileInfo *pInfo)
{
	if (pInfo != NULL)
	{
		free(pInfo);
	}
}
/* release the source */
void MovingFileInfoRelease(StMovingFileInfo *pInfo)
{
	if (pInfo != NULL)
	{
		if (pInfo->pFileNew != NULL)
		{
			free(pInfo->pFileNew);
		}
		if (pInfo->pFileDelete != NULL)
		{
			free(pInfo->pFileDelete);
		}

		free(pInfo);
	}
}

/* get the file list for update and deletion, from new version file list and old version file list */
static StMovingFileInfo *GetUpdateFileArray(json_object *pListNew, json_object *pListOld)
{
	if ((pListNew == NULL) || (pListOld == NULL))
	{
		PRINT("error arg\n");
		return NULL;
	}
	else
	{
		char c8Buf[16];
		json_object *pObj;
		uint32_t u32VersionNew = 0, u32VersionOld = 0;

#define JSON_GET_VERSION(pObj, u32Version)\
{\
	char *pTmp;\
	strncpy(c8Buf, json_object_to_json_string(pObj), 16);\
	PRINT("version: %s\n", c8Buf);\
	pTmp = (char *)(&(u32Version));\
	u32Version = 0;\
	sscanf(c8Buf, "\"%hhd.%hhd.%hhd\"", pTmp + 2, pTmp + 1, pTmp);\
}

		pObj = json_object_object_get(pListNew, "LastVersion");
		JSON_GET_VERSION(pObj, u32VersionNew);

		pObj = json_object_object_get(pListOld, "LastVersion");
		JSON_GET_VERSION(pObj, u32VersionOld);
		/* the new version is not bigger than old version */
		if (u32VersionNew <= u32VersionOld)
		{
			return NULL;
		}
		else
		{
			int32_t i, s32UpdateCnt, s32DeleteCnt;
			StUpdateFileInfo *pNew, *pOld;
			StMovingFileInfo *pMoving = NULL;
			int32_t s32NewLength;
			int32_t s32OldLength;

			pListNew = json_object_object_get(pListNew, "FileList");
			pListOld = json_object_object_get(pListOld, "FileList");
			s32NewLength = json_object_array_length(pListNew);
			s32OldLength = json_object_array_length(pListOld);

			if ((s32NewLength == 0) || (s32OldLength == 0))
			{
				goto err;
			}
			pMoving = calloc(1, sizeof(StMovingFileInfo));
			if (pMoving == NULL)
			{
				return NULL;
			}
			pMoving->pFileNew = pNew = calloc(s32NewLength + 1, sizeof(StUpdateFileInfo));
			pMoving->pFileDelete = pOld = calloc(s32OldLength + 1, sizeof(StUpdateFileInfo));

			if ((pNew == NULL) || (pOld == NULL))
			{
				goto err;
			}
/* #define NAME_TRUNCATION*/
/* translate JSON to array */
#if !(defined NAME_TRUNCATION)
#define JSON_TRANSLATE(src, dest, length)\
for(i = 0; i < length; i++)\
{\
	json_object *pObj;\
	json_object *pObjNew = json_object_array_get_idx(src, i);\
\
	StUpdateFileInfo *pInfo = dest + i;\
	PRINT("\t[%d]=%s\n", i, json_object_to_json_string(pObjNew));\
\
	pObj = json_object_object_get(pObjNew, "Url");\
	strncpy(pInfo->c8Url, json_object_get_string(pObj), 255);\
\
	pObj = json_object_object_get(pObjNew, "FileName");\
	strncpy(pInfo->c8Name, json_object_get_string(pObj), 63);\
\
	pObj = json_object_object_get(pObjNew, "FileType");\
	pInfo->u32FileType = json_object_get_int(pObj);\
\
	pObj = json_object_object_get(pObjNew, "Version");\
	JSON_GET_VERSION(pObj, pInfo->u32Version);\
	pObj = json_object_object_get(pObjNew, "CRC32");\
	strncpy(c8Buf, json_object_get_string(pObj), 16);\
				sscanf(c8Buf, "%u", &(pInfo->u32CRC32));\
}
#else
#define JSON_TRANSLATE(src, dest, length)\
for(i = 0; i < length; i++)\
{\
	json_object *pObj;\
	json_object *pObjNew = json_object_array_get_idx(src, i);\
	char *pTmp;\
	StUpdateFileInfo *pInfo = dest + i;\
	PRINT("\t[%d]=%s\n", i, json_object_to_json_string(pObjNew));\
\
	pObj = json_object_object_get(pObjNew, "Url");\
	strncpy(pInfo->c8Url, json_object_get_string(pObj), 255);\
\
	pObj = json_object_object_get(pObjNew, "FileName");\
	strncpy(pInfo->c8Name, json_object_get_string(pObj), 63);\
	pTmp = strrchr(pInfo->c8Name, '_');\
	if (pTmp != NULL)\
	{\
		pTmp[0] = 0;	/* truncation */ \
	}\
\
	pObj = json_object_object_get(pObjNew, "FileType");\
	pInfo->u32FileType = json_object_get_int(pObj);\
\
	pObj = json_object_object_get(pObjNew, "Version");\
	JSON_GET_VERSION(pObj, pInfo->u32Version);\
	pObj = json_object_object_get(pObjNew, "CRC32");\
	strncpy(c8Buf, json_object_get_string(pObj), 16);\
	sscanf(c8Buf, "%u", &(pInfo->u32CRC32));\
}
#endif
			JSON_TRANSLATE(pListNew, pNew, s32NewLength);
			JSON_TRANSLATE(pListOld, pOld, s32OldLength);

#undef JSON_GET_VERSION
#undef	JSON_TRANSLATE
			s32UpdateCnt = 0;
			for (i = 0; i < s32NewLength; i++)
			{
				int32_t j;
				for (j = 0; j < s32OldLength; j++)
				{
					if ((pOld[j].c8Name[0] != 0) && (strcmp(pNew[i].c8Name, pOld[j].c8Name) == 0))
					{
						PRINT("name: %s\n", pNew[i].c8Name);
						PRINT("version: %06X, old version: %06X\n",
								pNew[i].u32Version, pOld[j].u32Version);
						if (pNew[i].u32Version > pOld[j].u32Version)
						{
							/* get the file need to update */
							pNew[s32UpdateCnt] = pNew[i];
							pNew[s32UpdateCnt].u32OldVersion = pOld[j].u32Version;
							s32UpdateCnt++;
						}
						pOld[j].c8Name[0] = 0;
						break;
					}
				}
				/* get the file need to add */
				if (j == s32OldLength)	/* new file */
				{
					pNew[s32UpdateCnt] = pNew[i];
					pNew[s32UpdateCnt].u32OldVersion = 0;
					s32UpdateCnt++;
				}
			}
			/* get the files need to delete */
			s32DeleteCnt = 0;
			for (i = 0; i < s32OldLength; i++)
			{
				if (pOld[i].c8Name[0] != 0)
				{
					pOld[s32DeleteCnt++] = pOld[i];
					PRINT("the file need to delete is: %s\n", pOld[s32DeleteCnt - 1].c8Name);
				}
			}
			PRINT("count of update is:%d \n", s32UpdateCnt);
			PRINT("count of delete is:%d \n", s32DeleteCnt);
			pNew[s32UpdateCnt].c8Name[0] = 0;
			pOld[s32DeleteCnt].c8Name[0] = 0;

			if (s32UpdateCnt == 0)
			{
				UpdateFileInfoRelease(pNew);
				pMoving->pFileNew = NULL;
			}

			if (s32DeleteCnt == 0)
			{
				UpdateFileInfoRelease(pOld);
				pMoving->pFileDelete = NULL;
			}
			if ((s32DeleteCnt != 0) || (s32UpdateCnt != 0))
			{
				return pMoving;
			}
err:
			MovingFileInfoRelease(pMoving);
			return NULL;
		}
	}
}

/* download the new file form server if it's needed */
int32_t GetUpdateFile(StCloudStat *pCloudStat, json_object *pFileList, StMovingFileInfo **p2Info)
{
	int32_t s32Err = 0;
	StMovingFileInfo *pInfo = NULL;

	if ((pCloudStat == NULL) || (pFileList == NULL) ||(p2Info == NULL))
	{
		return MY_ERR(_Err_InvalidParam);
	}
	pInfo = *p2Info;
	if (pInfo != NULL)	/* it must be NULL */
	{
		return MY_ERR(_Err_InvalidParam);
	}
	else
	{
		json_object *pListOld = NULL;
		char c8FileName[_POSIX_PATH_MAX];

		sprintf(c8FileName, "%s%s", RUN_DIR, LIST_FILE);
		PRINT("file name %s\n", c8FileName);
		pListOld = json_object_from_file(c8FileName);
		if (pListOld == NULL)
		{
			PRINT("json_object_from_file error\n");
			s32Err = MY_ERR(_Err_JSON);
			goto err1;
		}
		/* get the information form this two list */
		pInfo = GetUpdateFileArray(pFileList, pListOld);
err1:
		if (pListOld != NULL)
		{
			json_object_put(pListOld);
		}

		/* now, download the files */
		if ((pInfo != NULL) && (pInfo->pFileNew != NULL))
		{
			int32_t s32Cnt = 0;
			*p2Info = pInfo;
			while(pInfo->pFileNew[s32Cnt].c8Name[0] != 0)
			{
				StUpdateFileInfo *pInfoTmp = pInfo->pFileNew + s32Cnt;
				StCloudDomain stStat;
				StSendInfo  stInfo = {true, NULL, NULL, NULL, -1};
				StMMap stMMap = {NULL,};
				char *pDomain, *pPort, *pFile;

				PRINT("FileName: %s\n, type: %d, new version: %06X, old version: %06X\n"\
						"url:%s\n"\
						"CRC32 is: %u\n"\
						"file type is: %u\n",
						pInfoTmp->c8Name, pInfoTmp->u32FileType,
						pInfoTmp->u32Version, pInfoTmp->u32OldVersion,
						pInfoTmp->c8Url,
						pInfoTmp->u32CRC32,
						pInfoTmp->u32FileType);

				/* check where the file is exist */
				{
					uint32_t u32CRC32 = -1;
					char c8Name[_POSIX_PATH_MAX];
					snprintf(c8Name, _POSIX_PATH_MAX - 1, "%s%s", UPDATE_DIR, pInfoTmp->c8Name);

					s32Err = CRC32File(c8Name, &u32CRC32);
					if (s32Err == 0)
					{
						if (u32CRC32 == pInfoTmp->u32CRC32)
						{
							PRINT("file %s is exist\n", c8Name);
							s32Cnt++;
							continue;	/* get the next file */
						}
					}
				}


				pDomain = strstr(pInfoTmp->c8Url, "http://");
				if (pDomain != NULL)
				{
					pDomain += 7;
				}
				pFile = strchr(pDomain, '/');
				if (pFile == NULL)
				{
					s32Err = MY_ERR(_Err_Cloud_Data);
					break;
				}
				pFile[0] = 0;
				pFile += 1;
				stInfo.pFile = pFile;
				pPort = strchr(pDomain, ':');
				if (pPort == NULL)
				{
					stStat.s32Port = 80;
				}
				else
				{
					pPort[0] = 0;
					stStat.s32Port = atoi(pPort + 1);
				}
				strncpy(stStat.c8Domain, pDomain, sizeof(stStat.c8Domain));
				stStat.stStat = *pCloudStat;
				/* down load */
				s32Err = CloudSendAndGetReturnNoSSL(&stStat, &stInfo, &stMMap);
				if (s32Err == 0)
				{
					int32_t s32File;
					uint32_t u32FileLength = 0;
					char *pTmp;
					char c8Name[_POSIX_PATH_MAX];
					pTmp = strstr((const char *)(stMMap.pMap), "Content-Length: ");
					sscanf(pTmp, "Content-Length: %u", &u32FileLength);
					PRINT("file length is: %uB\n", u32FileLength);

					snprintf(c8Name, _POSIX_PATH_MAX - 1, "%s%s", UPDATE_DIR, pInfoTmp->c8Name);
					pTmp = c8Name;
					PRINT("file name is: %s\n", pTmp);

					/* <TODO:> add execute permission according the file type */
					{
					int32_t s32ModeFlag = S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP | S_IROTH;
					if ((pInfoTmp->u32FileType == _File_Type_App) ||
							(pInfoTmp->u32FileType == _File_Type_Lib) ||
							(pInfoTmp->u32FileType == _File_Type_Shell))
					{
						s32ModeFlag |= (S_IXUSR | S_IXGRP | S_IXOTH);
						PRINT("this file could be executed\n");
					}
					s32File = open(pTmp, O_RDWR | O_CREAT | O_TRUNC | O_SYNC, s32ModeFlag);
					}

					if (s32File < 0)
					{
						s32Err = MY_ERR(_Err_SYS + errno);
						CloudMapRelease(&stMMap);
						break;
					}
					/* end of HTTP head */
					pTmp = strstr((const char *)(stMMap.pMap), "\r\n\r\n");
					pTmp += 4;
					write(s32File, pTmp, u32FileLength);
					close(s32File);
					CloudMapRelease(&stMMap);
					/* check whether the file is good */
					{
						uint32_t u32CRC32 = -1;
						s32Err = CRC32File(c8Name, &u32CRC32);
						if (s32Err != 0)
						{
							break;
						}
						PRINT("CRC32: %u, %u\n", pInfoTmp->u32CRC32, u32CRC32);
						if (u32CRC32 != pInfoTmp->u32CRC32)
						{
							s32Err = MY_ERR(_Err_File_CRC);
							break;
						}
					}
				}
				else
				{
					break;
				}
				s32Cnt++;
			}
		}
		else
		{
			s32Err = MY_ERR(_Err_No_New_Version);
		}
		/* no error happened, write the file list to the folder of update */
		if (s32Err >= 0)
		{
			char c8Name[_POSIX_PATH_MAX];
			snprintf(c8Name, _POSIX_PATH_MAX - 1, "%s%s", UPDATE_DIR, LIST_FILE);
			json_object_to_file_ext(c8Name, pFileList, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY);
		}
		return s32Err;
	}

}

/* copy the file to the run folder form update folder */
int32_t UpdateFileCopyToRun(StMovingFileInfo **p2Info)
{
	int32_t s32Err = 0;
	StMovingFileInfo *pInfo = NULL;

	if (p2Info == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	pInfo = *p2Info;
	if (pInfo == NULL)
	{
		json_object *pListNew = NULL;
		json_object *pListOld = NULL;
		char c8FileName[_POSIX_PATH_MAX];

		sprintf(c8FileName, "%s%s", UPDATE_DIR, LIST_FILE);
		PRINT("file name %s\n", c8FileName);
		pListNew = json_object_from_file(c8FileName);
		if (pListNew == NULL)
		{
			PRINT("json_object_from_file error\n");
			s32Err = MY_ERR(_Err_JSON);
			goto err1;
		}

		sprintf(c8FileName, "%s%s", RUN_DIR, LIST_FILE);
		PRINT("file name %s\n", c8FileName);
		pListOld = json_object_from_file(c8FileName);
		if (pListOld == NULL)
		{
			PRINT("json_object_from_file error\n");
			s32Err = MY_ERR(_Err_JSON);
			goto err1;
		}
		pInfo = GetUpdateFileArray(pListNew, pListOld);
err1:
		if (pListNew != NULL)
		{
			json_object_put(pListNew);
		}
		if (pListOld != NULL)
		{
			json_object_put(pListOld);
		}
	}
	if (pInfo != NULL)
	{
		int32_t s32Cnt;
		char c8Buf[1024];
		*p2Info = pInfo;

		if (pInfo->pFileNew != NULL)
		{
			s32Cnt = 0;
			while(pInfo->pFileNew[s32Cnt].c8Name[0] != 0)
			{
				StUpdateFileInfo *pInfoTmp = pInfo->pFileNew + s32Cnt;
				PRINT("FileName: %s\n, type: %d, new version: %06X, old version: %06X\n",
						pInfoTmp->c8Name, pInfoTmp->u32FileType,
						pInfoTmp->u32Version, pInfoTmp->u32OldVersion);

				if (pInfoTmp->u32OldVersion != 0)
				{
					/* remove the backup file from the backup directory */
					sprintf(c8Buf, "rm -f %s%s*", BACKUP_DIR, pInfoTmp->c8Name);
					system(c8Buf);

					/* copy the run file to the backup directory */
					sprintf(c8Buf, "cp -f %s%s* %s", RUN_DIR, pInfoTmp->c8Name, BACKUP_DIR);
					system(c8Buf);
				}

				/* remove the run file from the run directory */
				sprintf(c8Buf, "rm -f %s%s*", RUN_DIR, pInfoTmp->c8Name);
				system(c8Buf);

				/* copy the update file to the run directory */
				sprintf(c8Buf, "cp -f %s%s* %s", UPDATE_DIR, pInfoTmp->c8Name, RUN_DIR);
				system(c8Buf);

				s32Cnt++;
			}

		}
		if (pInfo->pFileDelete != NULL)
		{
			s32Cnt = 0;
			while(pInfo->pFileDelete[s32Cnt].c8Name[0] != 0)
			{
				StUpdateFileInfo *pInfoTmp = pInfo->pFileDelete + s32Cnt;
				PRINT("FileName: %s\n, type: %d, new version: %06X, old version: %06X\n",
						pInfoTmp->c8Name, pInfoTmp->u32FileType,
						pInfoTmp->u32Version, pInfoTmp->u32OldVersion);

				/* remove the backup file from the backup directory */
				sprintf(c8Buf, "rm -f %s%s*", BACKUP_DIR, pInfoTmp->c8Name);
				system(c8Buf);

				/* copy the run file to the backup directory */
				sprintf(c8Buf, "cp -f %s%s* %s", RUN_DIR, pInfoTmp->c8Name, BACKUP_DIR);
				system(c8Buf);

				/* remove the run file from the run directory */
				sprintf(c8Buf, "rm -f %s%s*", RUN_DIR, pInfoTmp->c8Name);
				system(c8Buf);

				s32Cnt++;
			}
		}
		/* copy the list file from the run directory to backup directory */
		sprintf(c8Buf, "cp -f %s%s %s%s", RUN_DIR, LIST_FILE, BACKUP_DIR, LIST_FILE);
		system(c8Buf);

		/* copy the list file from the update directory to run directory */
		sprintf(c8Buf, "cp -f %s%s %s%s", UPDATE_DIR, LIST_FILE, RUN_DIR, LIST_FILE);
		system(c8Buf);
#if HAS_CROSS
		/* remove the update file from the update directory */
		sprintf(c8Buf, "rm -f %s%s", UPDATE_DIR, LIST_FILE);
		system(c8Buf);
		sprintf(c8Buf, "rm -f %s*", UPDATE_DIR);
		system(c8Buf);
#endif
	}
	else
	{
		PRINT("no file need to update\n");
		s32Err = MY_ERR(_Err_No_New_Version);
	}
	return s32Err;
}

/* re-setup the process */
int32_t UpdateTheNewFile(StMovingFileInfo *pInfo, bool boIsJustForUpdate)
{
	int32_t s32Cnt = 0;
	uint32_t u32FileType = 0;
	if (pInfo == NULL)
	{
		return MY_ERR(_Err_InvalidParam);
	}
	if (pInfo->pFileNew == NULL)
	{
		return 0;
	}
	while(pInfo->pFileNew[s32Cnt].c8Name[0] != 0)
	{
		if (pInfo->pFileNew[s32Cnt].u32FileType == _File_Type_Kernel)
		{
			u32FileType |= UPDATE_KERNEL;
			break;
		}
		else if ((pInfo->pFileNew[s32Cnt].u32FileType == _File_Type_Driver) ||
				(pInfo->pFileNew[s32Cnt].u32FileType == _File_Type_Lib) ||
				(pInfo->pFileNew[s32Cnt].u32FileType == _File_Type_Config) ||
				(pInfo->pFileNew[s32Cnt].u32FileType == _File_Type_Shell))
		{
			u32FileType |= UPDATE_REBOOT;
		}
		else
		{
			u32FileType |= UPDATE_NORMAL;
		}
		s32Cnt++;
	}

	if ((u32FileType & UPDATE_KERNEL) == UPDATE_KERNEL)
	{
		char c8Buf[64];
		sprintf(c8Buf, "mtd_write -r write %s%s Kernel", RUN_DIR, pInfo->pFileNew[s32Cnt].c8Name);
		PRINT("%s\n", c8Buf);
#if HAS_CROSS
		system(c8Buf);
#else
		(void) c8Buf;
#endif
		return 0;
	}
	if (boIsJustForUpdate)		/* for update -u  */
	{
		return 0;
	}
	else if ((u32FileType & UPDATE_REBOOT) == UPDATE_REBOOT)
	{
#if HAS_CROSS
		system("reboot");
#endif
	}
	else
	{
		bool boIsUpdateMySelf = false;
		s32Cnt = 0;
		while(pInfo->pFileNew[s32Cnt].c8Name[0] != 0)
		{
			int32_t s32Pid = AlreadyRunningUsingName(pInfo->pFileNew[s32Cnt].c8Name);
			if (s32Pid == getpid())
			{
				boIsUpdateMySelf = true;
			}
			else if (s32Pid > 0)
			{

				char c8Buf[_POSIX_PATH_MAX];
				struct stat stStat;
				int32_t s32RepeatCnt = KILL_REPEAT_CNT;

				kill(s32Pid, SIGTERM);
				sprintf(c8Buf, "/proc/%d", s32Pid);
				while (s32RepeatCnt > 0)
				{

					if (stat(c8Buf, &stStat) < 0)
					{
						s32RepeatCnt = KILL_REPEAT_CNT + 1;
						break;
					}
					sleep(1);
				}
				/* now, i can start it */
				if (KILL_REPEAT_CNT == (KILL_REPEAT_CNT + 1))
				{
					sprintf(c8Buf, "%s%s", RUN_DIR, pInfo->pFileNew[s32Cnt].c8Name);
					RunAProcess(c8Buf, NULL);
				}
				else
				{
					PRINT("I can't kill process----%s(%d)\n", pInfo->pFileNew[s32Cnt].c8Name, s32Pid);
				}

			}
			else if (s32Pid < 0)
			{
				PRINT("AlreadyRunningUsingName error: 0x%08x\n", s32Pid);
			}
			s32Cnt++;
		}
		if (boIsUpdateMySelf)/* re-setup myself latest */
		{
			PRINT("I will kill myself\n");
			kill(getpid(), SIGTERM);
		}
	}

	return 0;

}


/******************************************* for M3 *******************************************/




