/*
 * yna_protocol.c
 *
 *  Created on: 2017年4月3日
 *      Author: lyndon
 */
#define NOT_USING_PRINT
#include "common.h"
#include "common_define.h"

int32_t CycleMsgInit(StCycleBuf *pCycleBuf, void *pBuf, uint32_t u32Length)
{
	if ((pCycleBuf == NULL) || (pBuf == NULL) || (u32Length == 0))
	{
		return -1;
	}
	memset(pCycleBuf, 0, sizeof(StCycleBuf));
	pCycleBuf->pBuf = pBuf;
	pCycleBuf->u32TotalLength = u32Length;

	return 0;
}

void *CycleGetOneMsg(StCycleBuf *pCycleBuf, const char *pData,
	uint32_t u32DataLength, uint32_t *pLength, int32_t *pProtocolType, int32_t *pErr)
{
	char *pBuf = NULL;
	int32_t s32Err = 0;
	if ((pCycleBuf == NULL) || (pLength == NULL))
	{
		s32Err = -1;
		goto end;
	}
	if (((pCycleBuf->u32TotalLength - pCycleBuf->u32Using) < u32DataLength)
		/*|| (u32DataLength == 0)*/)
	{
		PRINT("data too long\n");
		s32Err = -1;
	}
	if (u32DataLength != 0)
	{
		if (pData == NULL)
		{
			s32Err = -1;
			goto end;
		}
		else	/* copy data */
		{
			uint32_t u32Tmp = pCycleBuf->u32Write + u32DataLength;
			if (u32Tmp > pCycleBuf->u32TotalLength)
			{
				uint32_t u32CopyLength = pCycleBuf->u32TotalLength - pCycleBuf->u32Write;
				memcpy(pCycleBuf->pBuf + pCycleBuf->u32Write, pData, u32CopyLength);
				memcpy(pCycleBuf->pBuf, pData + u32CopyLength, u32DataLength - u32CopyLength);
				pCycleBuf->u32Write = u32DataLength - u32CopyLength;
			}
			else
			{
				memcpy(pCycleBuf->pBuf + pCycleBuf->u32Write, pData, u32DataLength);
				pCycleBuf->u32Write += u32DataLength;
			}
			pCycleBuf->u32Using += u32DataLength;

		}
	}

	do
	{
		uint32_t i;
		bool boIsBreak = false;


#define WRONG_MSG_DEFINE() \
		pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + 1));\
		pCycleBuf->u32Read += (i + 1);\
		pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;\
		pCycleBuf->u32Flag = 0;\
		break;

#define MSG_NOT_LONG_ENOUGH_DEFINE() \
		if (i != 0)\
		{\
			pCycleBuf->u32Using = (pCycleBuf->u32Using - i);\
			pCycleBuf->u32Read += i;\
			pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;\
		}\
		boIsBreak = true;

		for (i = 0; i < pCycleBuf->u32Using; i++)
		{
			uint32_t u32ReadIndex = i + pCycleBuf->u32Read;
			int32_t s32FirstByte;
			u32ReadIndex %= pCycleBuf->u32TotalLength;
			s32FirstByte = pCycleBuf->pBuf[u32ReadIndex];
			s32FirstByte &= 0xFF;
			if (s32FirstByte == 0xAA)
			{
				uint32_t u32CmdLen = 0;
				int32_t s32RemainLength = pCycleBuf->u32Using - i;

				/* check whether it's a variable length command */
				if (s32RemainLength >= PROTOCOL_YNA_DECODE_LENGTH - 1)
				{
					if (pCycleBuf->u32Flag != 0)
					{
						u32CmdLen = pCycleBuf->u32Flag;
					}
					else
					{
						uint32_t u32Start = i + pCycleBuf->u32Read;
						char *pTmp = pCycleBuf->pBuf;
						if ((pTmp[(u32Start + _YNA_Mix) % pCycleBuf->u32TotalLength] == 0x04)
							&& (pTmp[(u32Start + _YNA_Cmd) % pCycleBuf->u32TotalLength] == 0x00))
						{
							uint32_t u32MSB = pTmp[(u32Start + _YNA_Data2) % pCycleBuf->u32TotalLength];
							uint32_t u32LSB = pTmp[(u32Start + _YNA_Data3) % pCycleBuf->u32TotalLength];
							if (s32RemainLength >= PROTOCOL_YNA_DECODE_LENGTH)
							{
								uint32_t u32Start = i + pCycleBuf->u32Read;
								uint32_t u32End = PROTOCOL_YNA_DECODE_LENGTH - 1 + i + pCycleBuf->u32Read;
								char c8CheckSum = 0;
								uint32_t j;
								char c8Tmp;
								for (j = u32Start; j < u32End; j++)
								{
									c8CheckSum ^= pCycleBuf->pBuf[j % pCycleBuf->u32TotalLength];
								}
								c8Tmp = pCycleBuf->pBuf[u32End % pCycleBuf->u32TotalLength];
								if (c8CheckSum != c8Tmp) /* wrong message */
								{
									PRINT("get a wrong command: %d\n", u32CmdLen);
									WRONG_MSG_DEFINE();
									break;
								}

								u32MSB &= 0xFF;
								u32LSB &= 0xFF;

								pCycleBuf->u32Flag = ((u32MSB << 8) + u32LSB);
								u32CmdLen = pCycleBuf->u32Flag;
							}
						}
					}
				}
				u32CmdLen &= 0xFFFF;
				u32CmdLen += PROTOCOL_YNA_DECODE_LENGTH;
				PRINT("the data length is: %d\n", u32CmdLen);
				if (u32CmdLen > (pCycleBuf->u32TotalLength / 2))	/* maybe the message is wrong */
				{
					WRONG_MSG_DEFINE();
				}
				else if (((int32_t)(u32CmdLen)) <= s32RemainLength) /* good, I may got a message */
				{
					if (u32CmdLen == PROTOCOL_YNA_DECODE_LENGTH)
					{
						char c8CheckSum = 0, *pBufTmp, c8Tmp;
						uint32_t j, u32Start, u32End;
						uint32_t u32CmdLength = u32CmdLen;
						pBuf = (char *)malloc(u32CmdLength);
						if (pBuf == NULL)
						{
							s32Err = -1; /* big problem */
							goto end;
						}
						pBufTmp = pBuf;
						u32Start = i + pCycleBuf->u32Read;

						u32End = u32CmdLen - 1 + i + pCycleBuf->u32Read;
						PRINT("start: %d, end: %d\n", u32Start, u32End);
						for (j = u32Start; j < u32End; j++)
						{
							c8Tmp = pCycleBuf->pBuf[j % pCycleBuf->u32TotalLength];
							c8CheckSum ^= c8Tmp;
							*pBufTmp++ = c8Tmp;
						}
						c8Tmp = pCycleBuf->pBuf[u32End % pCycleBuf->u32TotalLength];
						if (c8CheckSum == c8Tmp) /* good message */
						{
							boIsBreak = true;
							*pBufTmp = c8Tmp;

							pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + u32CmdLength));
							pCycleBuf->u32Read = i + pCycleBuf->u32Read + u32CmdLength;
							pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
							pCycleBuf->u32Flag = 0;
							PRINT("get a command: %d\n", u32CmdLen);
							PRINT("u32Using: %d, u32Read: %d, u32Write: %d\n", pCycleBuf->u32Using, pCycleBuf->u32Read, pCycleBuf->u32Write);
							*pLength = u32CmdLength;
							if (pProtocolType != NULL)
							{
								*pProtocolType = _Protocol_YNA;
							}
						}
						else
						{
							free(pBuf);
							pBuf = NULL;
							PRINT("get a wrong command: %d\n", u32CmdLen);
							WRONG_MSG_DEFINE();
						}
					}
					else /* variable length */
					{
						uint32_t u32Start, u32End;
						uint32_t u32CmdLength = u32CmdLen;
						uint16_t u16CRCModBus;
						uint16_t u16CRCBuf;
						pBuf = (char *)malloc(u32CmdLength);
						if (pBuf == NULL)
						{
							s32Err = -1; /* big problem */
							goto end;
						}
						u32Start = (i + pCycleBuf->u32Read) % pCycleBuf->u32TotalLength;
						u32End = (u32CmdLen + i + pCycleBuf->u32Read) % pCycleBuf->u32TotalLength;
						PRINT("start: %d, end: %d\n", u32Start, u32End);
						if (u32End > u32Start)
						{
							memcpy(pBuf, pCycleBuf->pBuf + u32Start, u32CmdLen);
						}
						else
						{
							uint32_t u32FirstCopy = pCycleBuf->u32TotalLength - u32Start;
							memcpy(pBuf, pCycleBuf->pBuf + u32Start, u32FirstCopy);
							memcpy(pBuf + u32FirstCopy, pCycleBuf->pBuf, u32CmdLen - u32FirstCopy);
						}

						pCycleBuf->u32Flag = 0;

						/* we need not check the head's check sum,
						 * just check the CRC16-MODBUS
						 */
						u16CRCModBus = CRC16((const uint8_t *)pBuf + PROTOCOL_YNA_DECODE_LENGTH,
							u32CmdLen - PROTOCOL_YNA_DECODE_LENGTH - 2);
						u16CRCBuf = 0;

						LittleAndBigEndianTransfer((char *)(&u16CRCBuf), pBuf + u32CmdLen - 2, 2);
						if (u16CRCBuf == u16CRCModBus) /* good message */
						{
							boIsBreak = true;

							pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + u32CmdLength));
							pCycleBuf->u32Read = i + pCycleBuf->u32Read + u32CmdLength;
							pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
							pCycleBuf->u32Flag = 0;
							PRINT("get a command: %d\n", u32CmdLen);
							PRINT("u32Using: %d, u32Read: %d, u32Write: %d\n", pCycleBuf->u32Using, pCycleBuf->u32Read, pCycleBuf->u32Write);
							*pLength = u32CmdLength;
							if (pProtocolType != NULL)
							{
								*pProtocolType = _Protocol_YNA;
							}
						}
						else
						{
							free(pBuf);
							pBuf = NULL;
							PRINT("get a wrong command: %d\n", u32CmdLen);
							WRONG_MSG_DEFINE();
						}
					}
				}
				else	/* message not enough long */
				{
					MSG_NOT_LONG_ENOUGH_DEFINE();
				}
				break;
			}

			else if ((s32FirstByte & 0xF0) == 0x80)
			{
				int32_t s32RemainLength = pCycleBuf->u32Using - i;
				if (s32RemainLength >= PROTOCOL_VISCA_MIN_LENGTH)
				{
					uint32_t j;
					uint32_t u32Start = i + pCycleBuf->u32Read;
					uint32_t u32End = pCycleBuf->u32Using + pCycleBuf->u32Read;
					char c8Tmp = 0;
					for (j = u32Start + PROTOCOL_VISCA_MIN_LENGTH - 1; j < u32End; j++)
					{
						c8Tmp = pCycleBuf->pBuf[j % pCycleBuf->u32TotalLength];
						if (c8Tmp == (char)0xFF)
						{
							u32End = j;
							break;
						}
					}
					if (c8Tmp == (char)0xFF)
					{
						/* wrong message */
						if ((u32End - u32Start + 1) > PROTOCOL_VISCA_MAX_LENGTH)
						{
							pBuf = NULL;
							PRINT("get a wrong visca command: %d\n", (u32End - u32Start + 1));
							WRONG_MSG_DEFINE();
						}
						else
						{
							char *pBufTmp;
							uint32_t u32CmdLength = u32End - u32Start + 1;
							pBuf = (char *)malloc(u32CmdLength);
							if (pBuf == NULL)
							{
								s32Err = -1; /* big problem */
								goto end;
							}
							pBufTmp = pBuf;
							boIsBreak = true;

							PRINT("start: %d, end: %d\n", u32Start, u32End);
							for (j = u32Start; j <= u32End; j++)
							{
								*pBufTmp++ = pCycleBuf->pBuf[j % pCycleBuf->u32TotalLength];
							}

							pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + u32CmdLength));
							pCycleBuf->u32Read = i + pCycleBuf->u32Read + u32CmdLength;
							pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
							pCycleBuf->u32Flag = 0;
							PRINT("get a command: %d\n", u32CmdLength);
							PRINT("u32Using: %d, u32Read: %d, u32Write: %d\n", pCycleBuf->u32Using, pCycleBuf->u32Read, pCycleBuf->u32Write);
							*pLength = u32CmdLength;
							if (pProtocolType != NULL)
							{
								*pProtocolType = _Protocol_VISCA;
							}

						}
					}
					else
					{
						/* wrong message */
						if ((u32End - u32Start) >= PROTOCOL_VISCA_MAX_LENGTH)
						{
							pBuf = NULL;
							PRINT("get a wrong visca command: %d\n", (u32End - u32Start));
							WRONG_MSG_DEFINE();
						}
						else	/*message not long enough */
						{
							MSG_NOT_LONG_ENOUGH_DEFINE();
						}
					}
				}
				else	/* message not enough long */
				{
					/* correct the message header message, because the i is maybe not 0 */
					MSG_NOT_LONG_ENOUGH_DEFINE();
				}
				break;
			}

			else if (s32FirstByte == 'M')	/* MCS */
			{
				uint32_t u32CmdLen = 0;
				int32_t s32RemainLength = pCycleBuf->u32Using - i;

				/* get variable length */
				if (s32RemainLength >= sizeof(StMCSHeader))
				{
					if (pCycleBuf->u32Flag != 0)
					{
						u32CmdLen = pCycleBuf->u32Flag;
					}
					else
					{
						uint32_t u32Start = i + pCycleBuf->u32Read;
						char *pTmp = pCycleBuf->pBuf;
						uint32_t j;
						pCycleBuf->u32Flag = 0;
						char c8Header[4];
						for (j = 0; j < 4; j++)
						{
							c8Header[j] = pTmp[(u32Start + j) % pCycleBuf->u32TotalLength];
						}
						if (memcmp(c8Header, c_u8MixArr, 4) != 0)
						{
							PRINT("get a wrong MCS\n");
							WRONG_MSG_DEFINE();
							break;
						}
						for (j = 0; j < sizeof(uint32_t); j++)
						{
							pCycleBuf->u32Flag <<= 8;
							uint32_t u32Tmp = pTmp[(u32Start + offsetof(StMCSHeader, u32CmdTotalSize) + j) %
												pCycleBuf->u32TotalLength];
							u32Tmp &= 0xFF;
							pCycleBuf->u32Flag |= u32Tmp;
						}
						u32CmdLen = pCycleBuf->u32Flag;
					}
				}
				u32CmdLen += sizeof(StMCSHeader);
				PRINT("the data length is: %d\n", u32CmdLen);
				if (u32CmdLen > (pCycleBuf->u32TotalLength / 2))	/* maybe the message is wrong */
				{
					WRONG_MSG_DEFINE();
				}
				else if (((int32_t)(u32CmdLen)) <= s32RemainLength) /* good, I may got a message */
				{
					uint32_t j;
					uint32_t u32Start = i + pCycleBuf->u32Read;
					uint32_t u32End = u32Start + u32CmdLen;
					char *pBufTmp;
					pBuf = (char *)malloc(u32CmdLen);
					if (pBuf == NULL)
					{
						s32Err = -1; /* big problem */
						goto end;
					}
					pBufTmp = pBuf;
					boIsBreak = true;

					PRINT("start: %d, end: %d\n", u32Start, u32End);
					for (j = u32Start; j <= u32End; j++)
					{
						*pBufTmp++ = pCycleBuf->pBuf[j % pCycleBuf->u32TotalLength];
					}

					pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + u32CmdLen));
					pCycleBuf->u32Read = i + pCycleBuf->u32Read + u32CmdLen;
					pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
					pCycleBuf->u32Flag = 0;
					PRINT("get a MCS command: %d\n", u32CmdLen);
					PRINT("u32Using: %d, u32Read: %d, u32Write: %d\n", pCycleBuf->u32Using, pCycleBuf->u32Read, pCycleBuf->u32Write);
					*pLength = u32CmdLen;
					if (pProtocolType != NULL)
					{
						*pProtocolType = _Protocol_MCS;
					}
				}
				else	/* message not enough long */
				{
					MSG_NOT_LONG_ENOUGH_DEFINE();
				}
				break;

			}
		}
		if ((i == pCycleBuf->u32Using) && (!boIsBreak))
		{
			PRINT("cannot find AA, i = %d\n", pCycleBuf->u32Using);
			pCycleBuf->u32Using = 0;
			pCycleBuf->u32Read += i;
			pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
			pCycleBuf->u32Flag = 0;
		}

		if (boIsBreak)
		{
			break;
		}
#undef WRONG_MSG_DEFINE
#undef MSG_NOT_LONG_ENOUGH_DEFINE

	} while (((int32_t)pCycleBuf->u32Using) > 0);

	//if (pCycleBuf->u32Write + u32DataLength)

end:
	if (pErr != NULL)
	{
		*pErr = s32Err;
	}
	return pBuf;
}
/*
 * 函数名		: YNAMakeAnArrayVarialbleCmd
 * 功能			: 组建一个YNA可变长协议
 * 参数			: u16Cmd [in]: (uint16_t 类型) 命令号
 * 				  pData [in]: (void * 类型) 数据
 * 				  u32Count [in]: (uint32_t 类型) 数据数量
 * 				  u32Length [in]: (uint32_t 类型) 单个数组的长度
 * 				  pCmdLength [out]: (uint32_t * 类型) 组建成功的长度
 * 返回值		: (void * 类型) 非NULL表示成功，指向数据, 否则表示错误
 * 作者			: 许龙杰
 */
void *YNAMakeAnArrayVarialbleCmd(uint16_t u16Cmd, void *pData,
	uint32_t u32Count, uint32_t u32Length, uint32_t *pCmdLength)
{
	uint32_t u32CmdLength;
	uint32_t u32DataLength;
	uint32_t u32Tmp;
	uint8_t *pCmd = NULL;
	uint8_t *pVarialbleCmd;
	if (pData == NULL)
	{
		return NULL;
	}

	u32DataLength = u32Count * u32Length;

	/*  */
	u32CmdLength = PROTOCOL_YNA_DECODE_LENGTH + 6 + u32DataLength + 2;
	pCmd = malloc(u32CmdLength);
	if (pCmd == NULL)
	{
		return NULL;
	}
	memset(pCmd, 0, u32CmdLength);
	pCmd[_YNA_Sync] = 0xAA;
	pCmd[_YNA_Mix] = 0x04;
	pCmd[_YNA_Cmd] = 0x00;

	/* total length */
	u32Tmp = u32CmdLength - PROTOCOL_YNA_DECODE_LENGTH;
	LittleAndBigEndianTransfer((char *)pCmd + _YNA_Data2, (const char *)(&u32Tmp), 2);

	YNAGetCheckSum(pCmd);

	pVarialbleCmd = pCmd + PROTOCOL_YNA_DECODE_LENGTH;

	/* command serial */
	LittleAndBigEndianTransfer((char *)pVarialbleCmd, (const char *)(&u16Cmd), 2);

	/* command count */
	LittleAndBigEndianTransfer((char *)pVarialbleCmd + 2, (const char *)(&u32Count), 2);

	/* Varialble data length */
	LittleAndBigEndianTransfer((char *)pVarialbleCmd + 4, (const char *)(&u32Length), 2);

	/* copy the data */
	memcpy(pVarialbleCmd + 6, pData, u32DataLength);

	/* get the CRC16 of the variable command */
	u32Tmp = CRC16(pVarialbleCmd, 6 + u32DataLength);

	LittleAndBigEndianTransfer((char *)pVarialbleCmd + 6 + u32DataLength,
		(const char *)(&u32Tmp), 2);

	if (pCmdLength != NULL)
	{
		*pCmdLength = u32CmdLength;
	}

	return pCmd;
}

void *YNAMakeASimpleVarialbleCmd(uint16_t u16Cmd, void *pData,
	uint32_t u32DataLength, uint32_t *pCmdLength)
{
	return YNAMakeAnArrayVarialbleCmd(u16Cmd, pData, 1, u32DataLength, pCmdLength);
}

void YNAGetCheckSum(uint8_t *pBuf)
{
	if (pBuf != NULL)
	{
		int32_t i, s32End;
		uint8_t u8Sum = pBuf[0];
		s32End = PROTOCOL_YNA_DECODE_LENGTH - 1;

		for (i = 1; i < s32End; i++)
		{
			u8Sum ^= pBuf[i];
		}
		pBuf[i] = u8Sum;
	}
}

