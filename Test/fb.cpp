/*
 * fb.cpp
 *
 *  Created on: 2017年3月26日
 *      Author: lyndon
 */

#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>

#include "sample_comm.h"

#include "hi_comm_vo.h"
#include "mpi_vo.h"
#include "hifb.h"

#define IMAGE_WIDTH 640
#define IMAGE_HEIGHT 352
#define IMAGE_SIZE (640*352*2)
#define IMAGE_NUM 14
#define IMAGE_PATH "./res/%d.bits"

static struct fb_bitfield s_a32 = {24,8,0};
static struct fb_bitfield s_r32 = {16,8,0};
static struct fb_bitfield s_g32 = {8,8,0};
static struct fb_bitfield s_b32 = {0,8,0};

#define WIDTH                  1920
#define HEIGHT                 1080

bool g_boIsExit = false;
HI_VOID SAMPLE_HIFB_HandleSig(HI_S32 signo)
{
    if (SIGINT == signo || SIGTERM == signo)
    {
    	g_boIsExit = true;
        printf("program exit abnormally!\n");
    }
}
HI_S32 SAMPLE_HIFB(HI_VOID)
{
	HI_S32 s32Ret = HI_SUCCESS;
	VO_DEV VoDev = SAMPLE_VO_DEV_DHD0;

	HI_U32 u32PicWidth = WIDTH;
	HI_U32 u32PicHeight = HEIGHT;
	SIZE_S stSize;

	VO_LAYER VoLayer = 0;
	VO_PUB_ATTR_S stPubAttr;
	VO_VIDEO_LAYER_ATTR_S stLayerAttr;
	HI_U32 u32VoFrmRate;

	VB_CONF_S stVbConf;
	HI_U32 u32BlkSize;
	int fd = -1;

    signal(SIGINT, SAMPLE_HIFB_HandleSig);
    signal(SIGTERM, SAMPLE_HIFB_HandleSig);

	/******************************************
	 step  1: init variable
	 ******************************************/
	memset(&stVbConf, 0, sizeof(VB_CONF_S));

	u32BlkSize = CEILING_2_POWER(u32PicWidth, SAMPLE_SYS_ALIGN_WIDTH)
			* CEILING_2_POWER(u32PicHeight, SAMPLE_SYS_ALIGN_WIDTH) * 2;

	stVbConf.u32MaxPoolCnt = 128;

	stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
	stVbConf.astCommPool[0].u32BlkCnt = 6;

	/******************************************
	 step 2: mpp system init.
	 ******************************************/
	s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("system init failed with %d!\n", s32Ret);
		goto SAMPLE_HIFB_NoneBufMode_0;
	}

	/******************************************
	 step 3:  start vo hd0.
	 *****************************************/
	s32Ret = HI_MPI_VO_UnBindGraphicLayer(GRAPHICS_LAYER_HC0, VoDev);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("UnBindGraphicLayer failed with %d!\n", s32Ret);
		goto SAMPLE_HIFB_NoneBufMode_0;
	}

	s32Ret = HI_MPI_VO_BindGraphicLayer(GRAPHICS_LAYER_HC0, VoDev);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("BindGraphicLayer failed with %d!\n", s32Ret);
		goto SAMPLE_HIFB_NoneBufMode_0;
	}
	stPubAttr.enIntfSync = VO_OUTPUT_1080P60;
	stPubAttr.enIntfType = VO_INTF_HDMI | VO_INTF_VGA;
	stPubAttr.u32BgColor = 0x00ff00;

	s32Ret = SAMPLE_COMM_VO_StartDev(VoDev, &stPubAttr);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("start vo dev failed with %d!\n", s32Ret);
		goto SAMPLE_HIFB_NoneBufMode_0;
	}

	s32Ret = SAMPLE_COMM_VO_GetWH(stPubAttr.enIntfSync, &stSize.u32Width, &stSize.u32Height, &u32VoFrmRate);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("get vo wh failed with %d!\n", s32Ret);
		goto SAMPLE_HIFB_NoneBufMode_0;
	}
	SAMPLE_PRT("get vo wh Width: %d,  Height %d!\n", stSize.u32Width, stSize.u32Height);
	memset(&(stLayerAttr), 0, sizeof(VO_VIDEO_LAYER_ATTR_S));
	memcpy(&stLayerAttr.stImageSize, &stSize, sizeof(stSize));

	stLayerAttr.enPixFormat = SAMPLE_PIXEL_FORMAT;
	stLayerAttr.u32DispFrmRt = u32VoFrmRate;
	stLayerAttr.stDispRect.s32X = 0;
	stLayerAttr.stDispRect.s32Y = 0;
	stLayerAttr.stDispRect.u32Width = stSize.u32Width;
	stLayerAttr.stDispRect.u32Height = stSize.u32Height;

	s32Ret = SAMPLE_COMM_VO_StartLayer(VoLayer, &stLayerAttr);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("start vo layer failed with %d!\n", s32Ret);
		goto SAMPLE_HIFB_NoneBufMode_1;
	}

	if (stPubAttr.enIntfType & VO_INTF_HDMI)
	{
		s32Ret = SAMPLE_COMM_VO_HdmiStart(stPubAttr.enIntfSync);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("start HDMI failed with %d!\n", s32Ret);
			goto SAMPLE_HIFB_NoneBufMode_2;
		}
	}

	/******************************************
	 step 4:  start hifb.
	 *****************************************/




	do
	{
		fb_var_screeninfo stVarInfo;
		HIFB_DDRZONE_S stDDRZonePara;
		HIFB_LAYER_INFO_S stLayerinfo;

		/* 1. open framebuffer device overlay 0 */
		fd = open("/dev/fb0", O_RDWR, 0);
		if (fd < 0)
		{
			SAMPLE_PRT("open %s failed!\n", "/dev/fb0");
			break;
		}

		s32Ret = ioctl(fd, FBIOGET_VSCREENINFO, &stVarInfo);
		if (s32Ret < 0)
		{
			SAMPLE_PRT("FBIOGET_VSCREENINFO failed!\n");
			close(fd);
			break;
		}

		stVarInfo.red = s_r32;
		stVarInfo.green = s_g32;
		stVarInfo.blue = s_b32;
		stVarInfo.transp = s_a32;
		stVarInfo.bits_per_pixel = 32;
		stVarInfo.xres = WIDTH;
		stVarInfo.yres = HEIGHT;
		stVarInfo.activate = FB_ACTIVATE_NOW;
		stVarInfo.xres_virtual = WIDTH;
		stVarInfo.yres_virtual = HEIGHT;
		stVarInfo.xoffset = 0;
		stVarInfo.yoffset = 0;

		s32Ret = ioctl(fd, FBIOPUT_VSCREENINFO, &stVarInfo);
		if (s32Ret < 0)
		{
			SAMPLE_PRT("FBIOPUT_VSCREENINFO failed!\n");
			close(fd);
			break;
		}

		s32Ret = ioctl(fd, FBIOGET_LAYER_INFO, &stLayerinfo);
		if (s32Ret < 0)
		{
			SAMPLE_PRT("FBIOGET_LAYER_INFO failed!\n");
			close(fd);
			return HI_NULL;
		}

		stLayerinfo.u32Mask = 0;
		stLayerinfo.BufMode = HIFB_LAYER_BUF_NONE;
		stLayerinfo.u32Mask |= HIFB_LAYERMASK_BUFMODE;
		s32Ret = ioctl(fd, FBIOPUT_LAYER_INFO, &stLayerinfo);
		if (s32Ret < 0)
		{
			SAMPLE_PRT("FBIOPUT_LAYER_INFO failed!\n");
			close(fd);
			break;
		}
		stDDRZonePara.u32StartSection = 0;
		stDDRZonePara.u32ZoneNums = 15;
		s32Ret = ioctl(fd, FBIOPUT_MDDRDETECT_HIFB, &stDDRZonePara);
		if (s32Ret < 0)
		{
			SAMPLE_PRT("FBIOPUT_MDDRDETECT_HIFB failed!\n");
			close(fd);
			break;
		}

		/*9. close the devices*/
		//close(fd);
	} while (0);

	while (!g_boIsExit)
	{
		sleep(1);
	}

	if (fd >= 0)
	{
		close(fd);
	}

SAMPLE_HIFB_NoneBufMode_2:
	SAMPLE_COMM_VO_StopLayer(VoLayer);
SAMPLE_HIFB_NoneBufMode_1:
	SAMPLE_COMM_VO_StopDev(VoDev);
SAMPLE_HIFB_NoneBufMode_0:
	SAMPLE_COMM_SYS_Exit();

	return s32Ret;
}

int main()
{
	SAMPLE_HIFB();
	return 0;
}

