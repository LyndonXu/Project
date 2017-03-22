/****************************************************************************
 * Copyright(c), 2001-2060, ******************************* 版权所有
 ****************************************************************************
 * 文件名称             : wpa_client.c
 * 版本                 : 0.0.1
 * 作者                 : 许龙杰
 * 创建日期             : 2014年12月11日
 * 描述                 : 
 ****************************************************************************/
#include "common_define.h"
#include <linux/if.h>
#include <arpa/inet.h>

#include "../inc/common.h"
#include "openssl/sha.h"
#define SHA1_MAC_LEN 20

/**
 * struct wpa_ctrl - Internal structure for control interface library
 *
 * This structure is used by the wpa_supplicant/hostapd control interface
 * library to store internal data. Programs using the library should not touch
 * this data directly. They can only use the pointer to the data structure as
 * an identifier for the control interface connection and use this as one of
 * the arguments for most of the control interface library functions.
 */
struct wpa_ctrl {
	int s;
	struct sockaddr_un local;
	struct sockaddr_un dest;
};



struct wpa_ctrl * wpa_ctrl_open(const char *ctrl_path)
{
	struct wpa_ctrl *ctrl;
	static int counter = 0;

	ctrl = malloc(sizeof(*ctrl));
	if (ctrl == NULL)
		return NULL;
	memset(ctrl, 0, sizeof(*ctrl));

	ctrl->s = socket(PF_UNIX, SOCK_DGRAM, 0);
	if (ctrl->s < 0) {
		free(ctrl);
		return NULL;
	}

	ctrl->local.sun_family = AF_UNIX;
	snprintf(ctrl->local.sun_path, sizeof(ctrl->local.sun_path),
		    "/tmp/wpa_ctrl_%d-%d", getpid(), counter++);
	if (bind(ctrl->s, (struct sockaddr *) &ctrl->local,
		    sizeof(ctrl->local)) < 0) {
		close(ctrl->s);
		free(ctrl);
		return NULL;
	}

	ctrl->dest.sun_family = AF_UNIX;
	snprintf(ctrl->dest.sun_path, sizeof(ctrl->dest.sun_path), "%s",
		    ctrl_path);
	if (connect(ctrl->s, (struct sockaddr *) &ctrl->dest,
		    sizeof(ctrl->dest)) < 0) {
		close(ctrl->s);
		unlink(ctrl->local.sun_path);
		free(ctrl);
		return NULL;
	}

	return ctrl;
}


void wpa_ctrl_close(struct wpa_ctrl *ctrl)
{
	unlink(ctrl->local.sun_path);
	close(ctrl->s);
	free(ctrl);
}



int wpa_ctrl_request(struct wpa_ctrl *ctrl, const char *cmd, size_t cmd_len,
		     char *reply, size_t *reply_len,
		     void (*msg_cb)(char *msg, size_t len))
{
	struct timeval tv;
	int res;
	fd_set rfds;
	const char *_cmd;
	char *cmd_buf = NULL;
	size_t _cmd_len;
	{
		_cmd = cmd;
		_cmd_len = cmd_len;
	}

	if (send(ctrl->s, _cmd, _cmd_len, 0) < 0) {
		free(cmd_buf);
		return -1;
	}
	free(cmd_buf);

	for (;;) {
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		FD_ZERO(&rfds);
		FD_SET(ctrl->s, &rfds);
		res = select(ctrl->s + 1, &rfds, NULL, NULL, &tv);
		if (FD_ISSET(ctrl->s, &rfds)) {
			res = recv(ctrl->s, reply, *reply_len, 0);
			if (res < 0)
				return res;
			if (res > 0 && reply[0] == '<') {
				/* This is an unsolicited message from
				 * wpa_supplicant, not the reply to the
				 * request. Use msg_cb to report this to the
				 * caller. */
				if (msg_cb) {
					/* Make sure the message is nul
					 * terminated. */
					if ((size_t) res == *reply_len)
						res = (*reply_len) - 1;
					reply[res] = '\0';
					msg_cb(reply, res);
				}
				continue;
			}
			*reply_len = res;
			break;
		} else {
			return -2;
		}
	}
	return 0;
}


static int wpa_ctrl_attach_helper(struct wpa_ctrl *ctrl, int attach)
{
	char buf[10];
	int ret;
	size_t len = 10;

	ret = wpa_ctrl_request(ctrl, attach ? "ATTACH" : "DETACH", 6,
			       buf, &len, NULL);
	if (ret < 0)
		return ret;
	if (len == 3 && memcmp(buf, "OK\n", 3) == 0)
		return 0;
	return -1;
}


int wpa_ctrl_attach(struct wpa_ctrl *ctrl)
{
	return wpa_ctrl_attach_helper(ctrl, 1);
}


int wpa_ctrl_detach(struct wpa_ctrl *ctrl)
{
	return wpa_ctrl_attach_helper(ctrl, 0);
}



int wpa_ctrl_recv(struct wpa_ctrl *ctrl, char *reply, size_t *reply_len)
{
	int res;

	res = recv(ctrl->s, reply, *reply_len, 0);
	if (res < 0)
		return res;
	*reply_len = res;
	return 0;
}


int wpa_ctrl_pending(struct wpa_ctrl *ctrl)
{
	struct timeval tv;
	int res;
	(void) res;
	fd_set rfds;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO(&rfds);
	FD_SET(ctrl->s, &rfds);
	res = select(ctrl->s + 1, &rfds, NULL, NULL, &tv);
	return FD_ISSET(ctrl->s, &rfds);
}


int wpa_ctrl_get_fd(struct wpa_ctrl *ctrl)
{
	return ctrl->s;
}

static void sha1_vector(size_t num_elem, const unsigned char *addr[], const size_t *len, unsigned char *mac)
{
	SHA_CTX ctx;
	size_t i;

	SHA1_Init(&ctx);
	for (i = 0; i < num_elem; i++)
		SHA1_Update(&ctx, addr[i], len[i]);
	SHA1_Final(mac, &ctx);
}
/**
 * hmac_sha1_vector - HMAC-SHA1 over data vector (RFC 2104)
 * @key: Key for HMAC operations
 * @key_len: Length of the key in bytes
 * @num_elem: Number of elements in the data vector
 * @addr: Pointers to the data areas
 * @len: Lengths of the data blocks
 * @mac: Buffer for the hash (20 bytes)
 */
static void hmac_sha1_vector(const unsigned char *key, size_t key_len, size_t num_elem,
		      const unsigned char *addr[], const size_t *len, unsigned char *mac)
{
	unsigned char k_pad[64]; /* padding - key XORd with ipad/opad */
	unsigned char tk[20];
	const unsigned char *_addr[6];
	size_t _len[6], i;

	if (num_elem > 5) {
		/*
		 * Fixed limit on the number of fragments to avoid having to
		 * allocate memory (which could fail).
		 */
		return;
	}

        /* if key is longer than 64 bytes reset it to key = SHA1(key) */
        if (key_len > 64) {
		sha1_vector(1, &key, &key_len, tk);
		key = tk;
		key_len = 20;
        }

	/* the HMAC_SHA1 transform looks like:
	 *
	 * SHA1(K XOR opad, SHA1(K XOR ipad, text))
	 *
	 * where K is an n byte key
	 * ipad is the byte 0x36 repeated 64 times
	 * opad is the byte 0x5c repeated 64 times
	 * and text is the data being protected */

	/* start out by storing key in ipad */
	memset(k_pad, 0, sizeof(k_pad));
	memcpy(k_pad, key, key_len);
	/* XOR key with ipad values */
	for (i = 0; i < 64; i++)
		k_pad[i] ^= 0x36;

	/* perform inner SHA1 */
	_addr[0] = k_pad;
	_len[0] = 64;
	for (i = 0; i < num_elem; i++) {
		_addr[i + 1] = addr[i];
		_len[i + 1] = len[i];
	}
	sha1_vector(1 + num_elem, _addr, _len, mac);

	memset(k_pad, 0, sizeof(k_pad));
	memcpy(k_pad, key, key_len);
	/* XOR key with opad values */
	for (i = 0; i < 64; i++)
		k_pad[i] ^= 0x5c;

	/* perform outer SHA1 */
	_addr[0] = k_pad;
	_len[0] = 64;
	_addr[1] = mac;
	_len[1] = SHA1_MAC_LEN;
	sha1_vector(2, _addr, _len, mac);
}
/**
 * hmac_sha1 - HMAC-SHA1 over data buffer (RFC 2104)
 * @key: Key for HMAC operations
 * @key_len: Length of the key in bytes
 * @data: Pointers to the data area
 * @data_len: Length of the data area
 * @mac: Buffer for the hash (20 bytes)
 */
static void hmac_sha1(const unsigned char *key, size_t key_len, const unsigned char *data, size_t data_len,
		unsigned char *mac)
{
	hmac_sha1_vector(key, key_len, 1, &data, &data_len, mac);
}

/**
 * pbkdf2_sha1 - SHA1-based key derivation function (PBKDF2) for IEEE 802.11i
 * @passphrase: ASCII passphrase
 * @ssid: SSID
 * @ssid_len: SSID length in bytes
 * @interations: Number of iterations to run
 * @buf: Buffer for the generated key
 * @buflen: Length of the buffer in bytes
 *
 * This function is used to derive PSK for WPA-PSK. For this protocol,
 * iterations is set to 4096 and buflen to 32. This function is described in
 * IEEE Std 802.11-2004, Clause H.4. The main construction is from PKCS#5 v2.0.
 */
static void pbkdf2_sha1_f(const char *passphrase, const char *ssid,
			  size_t ssid_len, int iterations, unsigned int count,
			  unsigned char *digest)
{
	unsigned char tmp[SHA1_MAC_LEN], tmp2[SHA1_MAC_LEN];
	int i, j;
	unsigned char count_buf[4];
	const unsigned char *addr[2];
	size_t len[2];
	size_t passphrase_len = strlen(passphrase);

	addr[0] = (unsigned char *) ssid;
	len[0] = ssid_len;
	addr[1] = count_buf;
	len[1] = 4;

	/* F(P, S, c, i) = U1 xor U2 xor ... Uc
	 * U1 = PRF(P, S || i)
	 * U2 = PRF(P, U1)
	 * Uc = PRF(P, Uc-1)
	 */

	count_buf[0] = (count >> 24) & 0xff;
	count_buf[1] = (count >> 16) & 0xff;
	count_buf[2] = (count >> 8) & 0xff;
	count_buf[3] = count & 0xff;
	hmac_sha1_vector((unsigned char *) passphrase, passphrase_len, 2, addr, len, tmp);
	memcpy(digest, tmp, SHA1_MAC_LEN);

	for (i = 1; i < iterations; i++) {
		hmac_sha1((unsigned char *) passphrase, passphrase_len, tmp, SHA1_MAC_LEN,
			  tmp2);
		memcpy(tmp, tmp2, SHA1_MAC_LEN);
		for (j = 0; j < SHA1_MAC_LEN; j++)
			digest[j] ^= tmp2[j];
	}
}
/**
 * pbkdf2_sha1 - SHA1-based key derivation function (PBKDF2) for IEEE 802.11i
 * @passphrase: ASCII passphrase
 * @ssid: SSID
 * @ssid_len: SSID length in bytes
 * @interations: Number of iterations to run
 * @buf: Buffer for the generated key
 * @buflen: Length of the buffer in bytes
 *
 * This function is used to derive PSK for WPA-PSK. For this protocol,
 * iterations is set to 4096 and buflen to 32. This function is described in
 * IEEE Std 802.11-2004, Clause H.4. The main construction is from PKCS#5 v2.0.
 */
static void pbkdf2_sha1(const char *passphrase, const char *ssid, size_t ssid_len,
		 int iterations, unsigned char *buf, size_t buflen)
{
	unsigned int count = 0;
	unsigned char *pos = buf;
	size_t left = buflen, plen;
	unsigned char digest[SHA1_MAC_LEN];

	while (left > 0) {
		count++;
		pbkdf2_sha1_f(passphrase, ssid, ssid_len, iterations, count,
			      digest);
		plen = left > SHA1_MAC_LEN ? SHA1_MAC_LEN : left;
		memcpy(pos, digest, plen);
		pos += plen;
		left -= plen;
	}
}



static void WPAMsgCB(char *pMsg, size_t s32Len)
{
	PRINT("%s\n", pMsg);
}

static int32_t WPACtrlCmd(struct wpa_ctrl *pCtrl, const char *pCmd, char *pBuf, int32_t *pLen)
{
	int32_t s32Ret;

	PRINT("WPACtrlCmd is: %s\n", pCmd);
	s32Ret = wpa_ctrl_request(pCtrl, pCmd, strlen(pCmd), pBuf, (size_t *)pLen, WPAMsgCB);
	if (s32Ret < 0)
	{
		PRINT("'%s' command failed(%d).\n", pCmd, s32Ret);
		return s32Ret;
	}
	pBuf[*pLen] = '\0';
	PRINT("\n%s\n", pBuf);
	return 0;
}

static int32_t WPACmdPing(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	return WPACtrlCmd(pCtrl, "PING", pBuf, pLen);
}

static int32_t WPACmdStatus(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	return WPACtrlCmd(pCtrl, "STATUS", pBuf, pLen);
}

static int32_t WPACmdStatusVerbose(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	return WPACtrlCmd(pCtrl, "STATUS-VERBOSE", pBuf, pLen);
}

static int32_t WPACmdMIB(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	return WPACtrlCmd(pCtrl, "MIB", pBuf, pLen);
}
static int32_t WPACmdInterface(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	return WPACtrlCmd(pCtrl, "INTERFACES", pBuf, pLen);
}
static int32_t WPACmdLevel(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	if((pArgv != NULL) && (pArgv->u32Cnt == 1))
	{
		char c8Cmd[64];
		sprintf(c8Cmd, "LEVLE %s", pArgv->p2Argv[0]);
		return WPACtrlCmd(pCtrl, c8Cmd, pBuf, pLen);
	}
	else
	{
		PRINT("%s, argument error\n", __FUNCTION__);
		return -3;
	}

}
static int32_t WPACmdLogOff(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	return WPACtrlCmd(pCtrl, "LOGOFF", pBuf, pLen);
}
static int32_t WPACmdLogOn(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	return WPACtrlCmd(pCtrl, "LOGON", pBuf, pLen);
}
static int32_t WPACmdSet(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	if((pArgv != NULL) && (pArgv->u32Cnt == 2))
	{
		char c8Cmd[128];
		int32_t s32Ret = snprintf(c8Cmd, 128, "SET %s %s", pArgv->p2Argv[0], pArgv->p2Argv[1]);
		if ((s32Ret < 0) || (s32Ret > 128))
		{
			PRINT("%s, command is too long\n", __FUNCTION__);
			return -3;
		}
		return WPACtrlCmd(pCtrl, c8Cmd, pBuf, pLen);
	}
	else
	{
		PRINT("%s, argument error\n", __FUNCTION__);
		PRINT("set variables:\n"
		       "  EAPOL::heldPeriod (EAPOL state machine held period, "
		       "in seconds)\n"
		       "  EAPOL::authPeriod (EAPOL state machine authentication "
		       "period, in seconds)\n"
		       "  EAPOL::startPeriod (EAPOL state machine start period, in "
		       "seconds)\n"
		       "  EAPOL::maxStart (EAPOL state machine maximum start "
		       "attempts)\n");
		PRINT("  dot11RSNAConfigPMKLifetime (WPA/WPA2 PMK lifetime in "
		       "seconds)\n"
		       "  dot11RSNAConfigPMKReauthThreshold (WPA/WPA2 reauthentication"
		       " threshold\n\tpercentage)\n"
		       "  dot11RSNAConfigSATimeout (WPA/WPA2 timeout for completing "
		       "security\n\tassociation in seconds)\n");
		return -3;
	}
}

static int32_t WPACmdPMKSA(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	return WPACtrlCmd(pCtrl, "PMKSA", pBuf, pLen);
}
static int32_t WPACmdReassociation(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	return WPACtrlCmd(pCtrl, "REASSOCIATE", pBuf, pLen);
}
static int32_t WPACmdReconfigure(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	return WPACtrlCmd(pCtrl, "RECONFIGURE", pBuf, pLen);
}
static int32_t WPACmdPreauthenticate(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	if((pArgv != NULL) && (pArgv->u32Cnt == 1))
	{
		char c8Cmd[128];
		int32_t s32Ret = snprintf(c8Cmd, 128, "PREAUTH %s", pArgv->p2Argv[0]);
		if ((s32Ret < 0) || (s32Ret > 128))
		{
			PRINT("%s, command is too long\n", __FUNCTION__);
			return -3;
		}
		return WPACtrlCmd(pCtrl, c8Cmd, pBuf, pLen);
	}
	else
	{
		PRINT("%s, argument error\n", __FUNCTION__);
		return -3;
	}
}
static int32_t WPACmdIdentity(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	if((pArgv != NULL) && (pArgv->u32Cnt == 2))
	{
		char c8Cmd[128];
		int32_t s32Ret = snprintf(c8Cmd, 128, "IDENTITY-%s:%s", pArgv->p2Argv[0], pArgv->p2Argv[1]);
		if ((s32Ret < 0) || (s32Ret > 128))
		{
			PRINT("%s, command is too long\n", __FUNCTION__);
			return -3;
		}
		return WPACtrlCmd(pCtrl, c8Cmd, pBuf, pLen);
	}
	else
	{
		PRINT("%s, argument error\n", __FUNCTION__);
		return -3;
	}
}
static int32_t WPACmdPassword(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	if((pArgv != NULL) && (pArgv->u32Cnt == 2))
	{
		char c8Cmd[128];
		int32_t s32Ret = snprintf(c8Cmd, 128, "PASSWORD-%s:%s", pArgv->p2Argv[0], pArgv->p2Argv[1]);
		if ((s32Ret < 0) || (s32Ret > 128))
		{
			PRINT("%s, command is too long\n", __FUNCTION__);
			return -3;
		}
		return WPACtrlCmd(pCtrl, c8Cmd, pBuf, pLen);
	}
	else
	{
		PRINT("%s, argument error\n", __FUNCTION__);
		return -3;
	}
}
static int32_t WPACmdNewPassword(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	if((pArgv != NULL) && (pArgv->u32Cnt == 2))
	{
		char c8Cmd[128];
		int32_t s32Ret = snprintf(c8Cmd, 128, "NEW_PASSWORD-%s:%s", pArgv->p2Argv[0], pArgv->p2Argv[1]);
		if ((s32Ret < 0) || (s32Ret > 128))
		{
			PRINT("%s, command is too long\n", __FUNCTION__);
			return -3;
		}
		return WPACtrlCmd(pCtrl, c8Cmd, pBuf, pLen);
	}
	else
	{
		PRINT("%s, argument error\n", __FUNCTION__);
		return -3;
	}
}
static int32_t WPACmdPin(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	if((pArgv != NULL) && (pArgv->u32Cnt == 2))
	{
		char c8Cmd[128];
		int32_t s32Ret = snprintf(c8Cmd, 128, "PIN-%s:%s", pArgv->p2Argv[0], pArgv->p2Argv[1]);
		if ((s32Ret < 0) || (s32Ret > 128))
		{
			PRINT("%s, command is too long\n", __FUNCTION__);
			return -3;
		}
		return WPACtrlCmd(pCtrl, c8Cmd, pBuf, pLen);
	}
	else
	{
		PRINT("%s, argument error\n", __FUNCTION__);
		return -3;
	}
}
static int32_t WPACmdOTP(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	if((pArgv != NULL) && (pArgv->u32Cnt == 2))
	{
		char c8Cmd[128];
		int32_t s32Ret = snprintf(c8Cmd, 128, "OTP-%s:%s", pArgv->p2Argv[0], pArgv->p2Argv[1]);
		if ((s32Ret < 0) || (s32Ret > 128))
		{
			PRINT("%s, command is too long\n", __FUNCTION__);
			return -3;
		}
		return WPACtrlCmd(pCtrl, c8Cmd, pBuf, pLen);
	}
	else
	{
		PRINT("%s, argument error\n", __FUNCTION__);
		return -3;
	}
}
static int32_t WPACmdPassphrase(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	if((pArgv != NULL) && (pArgv->u32Cnt == 2))
	{
		char c8Cmd[128];
		int32_t s32Ret = snprintf(c8Cmd, 128, "PASSPHRASE-%s:%s", pArgv->p2Argv[0], pArgv->p2Argv[1]);
		if ((s32Ret < 0) || (s32Ret > 128))
		{
			PRINT("%s, command is too long\n", __FUNCTION__);
			return -3;
		}
		return WPACtrlCmd(pCtrl, c8Cmd, pBuf, pLen);
	}
	else
	{
		PRINT("%s, argument error\n", __FUNCTION__);
		return -3;
	}
}
static int32_t WPACmdBSSID(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	if((pArgv != NULL) && (pArgv->u32Cnt == 2))
	{
		char c8Cmd[128];
		int32_t s32Ret = snprintf(c8Cmd, 128, "BSSID-%s:%s", pArgv->p2Argv[0], pArgv->p2Argv[1]);
		if ((s32Ret < 0) || (s32Ret > 128))
		{
			PRINT("%s, command is too long\n", __FUNCTION__);
			return -3;
		}
		return WPACtrlCmd(pCtrl, c8Cmd, pBuf, pLen);
	}
	else
	{
		PRINT("%s, argument error\n", __FUNCTION__);
		return -3;
	}
}
static int32_t WPACmdListNetwork(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	return WPACtrlCmd(pCtrl, "LIST_NETWORKS", pBuf, pLen);
}
static int32_t WPACmdSelectNetwork(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	if((pArgv != NULL) && (pArgv->u32Cnt == 1))
	{
		char c8Cmd[128];
		int32_t s32Ret = snprintf(c8Cmd, 128, "SELECT_NETWORK %s", pArgv->p2Argv[0]);
		if ((s32Ret < 0) || (s32Ret > 128))
		{
			PRINT("%s, command is too long\n", __FUNCTION__);
			return -3;
		}
		return WPACtrlCmd(pCtrl, c8Cmd, pBuf, pLen);
	}
	else
	{
		PRINT("%s, argument error\n", __FUNCTION__);
		return -3;
	}
}

static int32_t WPACmdEnableNetwork(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	if((pArgv != NULL) && (pArgv->u32Cnt == 1))
	{
		char c8Cmd[128];
		int32_t s32Ret = snprintf(c8Cmd, 128, "ENABLE_NETWORK %s", pArgv->p2Argv[0]);
		if ((s32Ret < 0) || (s32Ret > 128))
		{
			PRINT("%s, command is too long\n", __FUNCTION__);
			return -3;
		}
		return WPACtrlCmd(pCtrl, c8Cmd, pBuf, pLen);
	}
	else
	{
		PRINT("%s, argument error\n", __FUNCTION__);
		return -3;
	}
}
static int32_t WPACmdDisableNetwork(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	if((pArgv != NULL) && (pArgv->u32Cnt == 1))
	{
		char c8Cmd[128];
		int32_t s32Ret = snprintf(c8Cmd, 128, "DISABLE_NETWORK %s", pArgv->p2Argv[0]);
		if ((s32Ret < 0) || (s32Ret > 128))
		{
			PRINT("%s, command is too long\n", __FUNCTION__);
			return -3;
		}
		return WPACtrlCmd(pCtrl, c8Cmd, pBuf, pLen);
	}
	else
	{
		PRINT("%s, argument error\n", __FUNCTION__);
		return -3;
	}
}
static int32_t WPACmdAddNetwork(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	return WPACtrlCmd(pCtrl, "ADD_NETWORK", pBuf, pLen);
}
static int32_t WPACmdRemoveNetwork(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	if((pArgv != NULL) && (pArgv->u32Cnt == 1))
	{
		char c8Cmd[128];
		int32_t s32Ret = snprintf(c8Cmd, 128, "REMOVE_NETWORK %s", pArgv->p2Argv[0]);
		if ((s32Ret < 0) || (s32Ret > 128))
		{
			PRINT("%s, command is too long\n", __FUNCTION__);
			return -3;
		}
		return WPACtrlCmd(pCtrl, c8Cmd, pBuf, pLen);
	}
	else
	{
		PRINT("%s, argument error\n", __FUNCTION__);
		return -3;
	}
}
static int32_t WPACmdSetNetwork(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	if((pArgv != NULL) && (pArgv->u32Cnt == 3))
	{
		char c8Cmd[256];
		int32_t s32Ret = snprintf(c8Cmd, 256, "SET_NETWORK %s %s %s", pArgv->p2Argv[0], pArgv->p2Argv[1], pArgv->p2Argv[2]);
		if ((s32Ret < 0) || (s32Ret > 256))
		{
			PRINT("%s, command is too long\n", __FUNCTION__);
			return -3;
		}
		return WPACtrlCmd(pCtrl, c8Cmd, pBuf, pLen);
	}
	else
	{
		PRINT("%s, argument error\n", __FUNCTION__);
		PRINT("set_network variables:\n"
		       "  ssid (network name, SSID)\n"
		       "  psk (WPA passphrase or pre-shared key)\n"
		       "  key_mgmt (key management protocol)\n"
		       "  identity (EAP identity)\n"
		       "  password (EAP password)\n"
		       "  ...\n"
		       "\n"
		       "Note: Values are entered in the same format as the "
		       "configuration file is using,\n"
		       "i.e., strings values need to be inside double quotation "
		       "marks.\n"
		       "For example: set_network 1 ssid \"network name\"\n"
		       "\n"
		       "Please see wpa_supplicant.conf documentation for full list "
		       "of\navailable variables.\n");
		return -3;
	}
}
static int32_t WPACmdGetNetwork(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	if((pArgv != NULL) && (pArgv->u32Cnt == 2))
	{
		char c8Cmd[256];
		int32_t s32Ret = snprintf(c8Cmd, 256, "GET_NETWORK %s %s", pArgv->p2Argv[0], pArgv->p2Argv[1]);
		if ((s32Ret < 0) || (s32Ret > 256))
		{
			PRINT("%s, command is too long\n", __FUNCTION__);
			return -3;
		}
		return WPACtrlCmd(pCtrl, c8Cmd, pBuf, pLen);
	}
	else
	{
		PRINT("%s, argument error\n", __FUNCTION__);
		return -3;
	}
}
static int32_t WPACmdDisconnect(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	return WPACtrlCmd(pCtrl, "DISCONNECT", pBuf, pLen);
}
static int32_t WPACmdSaveConfig(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	return WPACtrlCmd(pCtrl, "SAVE_CONFIG", pBuf, pLen);
}
static int32_t WPACmdScan(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	return WPACtrlCmd(pCtrl, "SCAN", pBuf, pLen);
}
static int32_t WPACmdScanResults(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	return WPACtrlCmd(pCtrl, "SCAN_RESULTS", pBuf, pLen);
}
static int32_t WPACmdCapability(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	if((pArgv != NULL) && ((pArgv->u32Cnt == 2) || (pArgv->u32Cnt == 1)))
	{
		char c8Cmd[256];
		int32_t s32Ret;
		if (pArgv->u32Cnt == 2)
		{
			if (strcmp(pArgv->p2Argv[1], "strict") != 0)
			{
				PRINT("%s, command is too long\n", __FUNCTION__);
				return -3;
			}
			s32Ret = snprintf(c8Cmd, 256, "GET_CAPABILITY %s %s", pArgv->p2Argv[0], pArgv->p2Argv[1]);
			if ((s32Ret < 0) || (s32Ret > 256))
			{
				PRINT("%s, command is too long\n", __FUNCTION__);
				return -3;
			}
		}
		else
		{
			s32Ret = snprintf(c8Cmd, 256, "GET_CAPABILITY %s", pArgv->p2Argv[0]);
			if ((s32Ret < 0) || (s32Ret > 256))
			{
				PRINT("%s, command is too long\n", __FUNCTION__);
				return -3;
			}
		}

		return WPACtrlCmd(pCtrl, c8Cmd, pBuf, pLen);
	}
	else
	{
		PRINT("%s, argument error\n", __FUNCTION__);
		return -3;
	}
}
static int32_t WPACmdAPScan(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	if((pArgv != NULL) && (pArgv->u32Cnt == 1))
	{
		char c8Cmd[256];
		int32_t s32Ret = snprintf(c8Cmd, 256, "AP_SCAN %s", pArgv->p2Argv[0]);
		if ((s32Ret < 0) || (s32Ret > 256))
		{
			PRINT("%s, command is too long\n", __FUNCTION__);
			return -3;
		}
		return WPACtrlCmd(pCtrl, c8Cmd, pBuf, pLen);
	}
	else
	{
		PRINT("%s, argument error\n", __FUNCTION__);
		return -3;
	}
}
static int32_t WPACmdSTKStart(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	if((pArgv != NULL) && (pArgv->u32Cnt == 1))
	{
		char c8Cmd[256];
		int32_t s32Ret = snprintf(c8Cmd, 256, "STKSTART %s", pArgv->p2Argv[0]);
		if ((s32Ret < 0) || (s32Ret > 256))
		{
			PRINT("%s, command is too long\n", __FUNCTION__);
			return -3;
		}
		return WPACtrlCmd(pCtrl, c8Cmd, pBuf, pLen);
	}
	else
	{
		PRINT("%s, argument error\n", __FUNCTION__);
		return -3;
	}
}
static int32_t WPACmdTerminate(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	return WPACtrlCmd(pCtrl, "TERMINATE", pBuf, pLen);
}
static int32_t WPACmdPSKSHA(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen)
{
	if((pArgv != NULL) && (pArgv->u32Cnt == 2))
	{
		uint8_t u8PSK[32];
		uint32_t i;

		pbkdf2_sha1(pArgv->p2Argv[1], pArgv->p2Argv[0], strlen(pArgv->p2Argv[0]), 4096, u8PSK, 32);

		for (i = 0; i < 32; i++)
		{
			sprintf(pBuf + i * 2, "%02hhx", u8PSK[i]);
		}
		*pLen = 64;
		pBuf[*pLen] = '\0';
		PRINT("\n%s\n", pBuf);
		return 0;
	}
	else
	{
		PRINT("%s, argument error\n", __FUNCTION__);
		return -3;
	}

}

typedef int32_t (*PFUN_WpaCmd)(struct wpa_ctrl *pCtrl, StArgv *pArgv, char *pBuf, int32_t *pLen);
typedef struct _tagStWPACmdFun
{
	PFUN_WpaCmd pFunWpaCmd;
#if defined _DEBUG
	struct
	{
		const char *pCmd;
		const char *pHelp;
	};
#endif
}StWPACmdFun;
const StWPACmdFun c_stWPACmdFun[_WPA_CIC_Reserved] =
{
	{
		WPACmdPing,
#if defined _DEBUG
		{"ping", "= get PONG if wpa_supplicant is running"}
#endif
	},
	{
		WPACmdStatus,
#if defined _DEBUG
		{"status", "= get current WPA/EAPOL/EAP status"}
#endif
	},
	{
		WPACmdStatusVerbose,
#if defined _DEBUG
		{"status verbose", "= get current WPA/EAPOL/EAP status"}
#endif
	},
	{
		WPACmdMIB,
#if defined _DEBUG
		{"mib", "= get MIB variables (dot1x, dot11)"}
#endif
	},
	{
		WPACmdInterface,
#if defined _DEBUG
		{"interface", "[ifname] = show interfaces/select interface"}
#endif
	},
	{
		WPACmdLevel,
#if defined _DEBUG
		{"level", "<debug level> = change debug level"}
#endif
	},
	{
		WPACmdLogOff,
#if defined _DEBUG
		{"logoff", " = IEEE 802.1X EAPOL state machine logoff"}
#endif
	},
	{
		WPACmdLogOn,
#if defined _DEBUG
		{"logon", " = IEEE 802.1X EAPOL state machine logon"}
#endif
	},
	{
		WPACmdSet,
#if defined _DEBUG
		{"set", " = set variables (shows list of variables when run without arguments)"}
#endif
	},
	{
		WPACmdPMKSA,
#if defined _DEBUG
		{"pmksa",  " = show PMKSA cache"}
#endif
	},
	{
		WPACmdReassociation,
#if defined _DEBUG
		{"reassociate", "= force reassociation"}
#endif
	},
	{
		WPACmdReconfigure,
#if defined _DEBUG
		{"reconfigure", " = force wpa_supplicant to re-read its configuration file"}
#endif
	},
	{
		WPACmdPreauthenticate,
#if defined _DEBUG
		{"preauthenticate", "<BSSID> = force preauthentication"}
#endif
	},
	{
		WPACmdIdentity,
#if defined _DEBUG
		{"identity", "<network id> <identity> = configure identity for an SSID"}
#endif
	},
	{
		WPACmdPassword,
#if defined _DEBUG
		{"password", "<network id> <password> = configure password for an SSID"}
#endif
	},
	{
		WPACmdNewPassword,
#if defined _DEBUG
		{"new_password", "<network id> <password> = change password for an SSID"}
#endif
	},
	{
		WPACmdPin,
#if defined _DEBUG
		{"pin", "<network id> <pin> = configure pin for an SSID"}
#endif
	},
	{
		WPACmdOTP,
#if defined _DEBUG
		{"otp", "<network id> <password> = configure one-time-password for an SSID"}
#endif
	},
	{
		WPACmdPassphrase,
#if defined _DEBUG
		{"passphrase", "<network id> <passphrase> = configure private key passphrase for an SSID"}
#endif
	},
	{
		WPACmdBSSID,
#if defined _DEBUG
		{"bssid", "<network id> <BSSID> = set preferred BSSID for an SSID"}
#endif
	},
	{
		WPACmdListNetwork,
#if defined _DEBUG
		{"list_networks", "= list configured networks"}
#endif
	},
	{
		WPACmdSelectNetwork,
#if defined _DEBUG
		{"select_network", "<network id> = select a network (disable others)"}
#endif
	},
	{
		WPACmdEnableNetwork,
#if defined _DEBUG
		{"enable_network", "<network id> = enable a network"}
#endif
	},
	{
		WPACmdDisableNetwork,
#if defined _DEBUG
		{"disable_network", "<network id> = disable a network"}
#endif
	},
	{
		WPACmdAddNetwork,
#if defined _DEBUG
		{"add_network", "= add a network"}
#endif
	},
	{
		WPACmdRemoveNetwork,
#if defined _DEBUG
		{"remove_network", "<network id> = remove a network"}
#endif
	},
	{
		WPACmdSetNetwork,
#if defined _DEBUG
		{"set_network", "<network id> <variable> <value> = set network variables (shows list of variables when run without arguments)"}
#endif
	},
	{
		WPACmdGetNetwork,
#if defined _DEBUG
		{"get_network", "<network id> <variable> = get network variables"}
#endif
	},
	{
		WPACmdDisconnect,
#if defined _DEBUG
		{"disconnect", "= disconnect and wait for reassociate command before connecting"}
#endif
	},
	{
		WPACmdSaveConfig,
#if defined _DEBUG
		{"save_config", "= save the current configuration"}
#endif
	},
	{
		WPACmdScan,
#if defined _DEBUG
		{"scan", "= request new BSS scan"}
#endif
	},
	{
		WPACmdScanResults,
#if defined _DEBUG
		{"scan_results", "= get latest scan results"}
#endif
	},
	{
		WPACmdCapability,
#if defined _DEBUG
		{"get_capability", "<eap/pairwise/group/key_mgmt/proto/auth_alg> = get capabilies"}
#endif
	},
	{
		WPACmdAPScan,
#if defined _DEBUG
		{"ap_scan", "<value> = set ap_scan parameter"}
#endif
	},
	{
		WPACmdSTKStart,
#if defined _DEBUG
		{"stkstart", "<addr> = request STK negotiation with <addr>"}
#endif
	},
	{
		WPACmdTerminate,
#if defined _DEBUG
		{"terminate", "= terminate wpa_supplicant"}
#endif
	},
	{
		WPACmdPSKSHA,
#if defined _DEBUG
		{"psk_sha", "<ssid> <psk> = get sha1 of psk for ssid"}
#endif
	},

	/* {_WPA_CIC_Reserved, NULL, }, */
};

const char *WPARequest(EmWPACIC emCmd, const char *pInterface, StArgv *pArgv, int32_t *pResult)
{
	int32_t s32Res = 0;
	char *pBuf = NULL;
	struct wpa_ctrl *pCtrl;
	if (emCmd >= _WPA_CIC_Reserved)
	{
		s32Res = -1;
		goto end;
	}
	do
	{
		char c8Path[_POSIX_PATH_MAX];
		if (pInterface != NULL)
		{
			sprintf(c8Path, "%s%s", CTRL_INTERFACE_DIR, pInterface);
		}
		else
		{
			sprintf(c8Path, "%s%s", CTRL_INTERFACE_DIR, WIFI_INTERFACE);
		}
		pCtrl = wpa_ctrl_open(c8Path);
	}while(0);
	if (pCtrl == NULL)
	{
		s32Res = MY_ERR(_Err_WPA_Open);
		goto end;
	}

	do
	{
		int32_t s32Len = PAGE_SIZE;
		pBuf = calloc(1, s32Len);
		if (pBuf == NULL)
		{
			s32Res = MY_ERR(_Err_Mem);
			goto end1;
		}
		s32Res = c_stWPACmdFun[emCmd].pFunWpaCmd(pCtrl, pArgv, pBuf, &s32Len);
		if (s32Res < 0)
		{
			free(pBuf);
			pBuf = NULL;
		}
		else
		{
			s32Res = s32Len;
		}
	}while(0);

end1:
	wpa_ctrl_close(pCtrl);
end:
	if (pResult != NULL)
	{
		if (s32Res == -1)
		{
			s32Res = MY_ERR(_Err_WPA_Send);
		}
		else if (s32Res == -2)
		{
			s32Res = MY_ERR(_Err_WPA_TimeOut);
		}
		else if (s32Res == -3)
		{
#if defined _DEBUG
			PRINT("%s %s\n", c_stWPACmdFun[emCmd].pCmd, c_stWPACmdFun[emCmd].pHelp);
#endif
			s32Res = MY_ERR(_Err_InvalidParam);
		}
		*pResult = s32Res;
	}
	return pBuf;
}

#if defined _DEBUG

int WpaClientTest(int argc, const char *argv[])
{
	const char *pBuf;
	int32_t s32Rslt = 0;

	if (argc == 1)
	{
		for(s32Rslt = 0; s32Rslt < _WPA_CIC_Reserved; s32Rslt++)
		{
			PRINT("%s %s\n", c_stWPACmdFun[s32Rslt].pCmd, c_stWPACmdFun[s32Rslt].pHelp);
		}
		return 0;
	}
	for(s32Rslt = 0; s32Rslt < _WPA_CIC_Reserved; s32Rslt++)
	{
		if (strncmp(c_stWPACmdFun[s32Rslt].pCmd, argv[1], strlen(argv[1])) == 0)
		{
			StArgv stArgv;
			stArgv.u32Cnt = argc - 2;
			stArgv.p2Argv = argv + 2;

			pBuf = WPARequest(s32Rslt, NULL, &stArgv, &s32Rslt);
			if (pBuf == NULL)
			{
				PRINT("WPARequest error: 0x%08x\n", s32Rslt);
			}
			else
			{
				free((void *)pBuf);
			}
			return 0;
		}
	}
	for(s32Rslt = 0; s32Rslt < _WPA_CIC_Reserved; s32Rslt++)
	{
		PRINT("%s %s\n", c_stWPACmdFun[s32Rslt].pCmd, c_stWPACmdFun[s32Rslt].pHelp);
	}
	return 0;
}
#endif

