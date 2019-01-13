/******************************************************************************
  A simple program of Hisilicon HI3531 video encode implementation.
  Copyright (C), 2010-2011, Hisilicon Tech. Co., Ltd.
 ******************************************************************************
    Modification:  2011-2 Created
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "sample_comm.h"

#include <iostream>
#include "BasicUsageEnvironment.hh"
#include "UsageEnvironment.hh"
#include "live_H264VideoSource.hh"
#include "live_H264VideoServerMediaSubsession.hh"
#include "liveMedia_version.hh"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extern HI_S32 PostH264Stream(HI_S32 s32Cnt);

#define MKFIFO_PATH "/tmp/H264_fifo"
#define MEDIA_SERVER_VERSION_STRING "0.91"

VIDEO_NORM_E gs_enNorm = VIDEO_ENCODING_MODE_NTSC;

#ifdef hi3518ev201

HI_U32 g_u32BlkCnt = 4;
#endif

#ifdef hi3518ev200

HI_U32 g_u32BlkCnt = 4;

#endif

#ifdef hi3516cv200

HI_U32 g_u32BlkCnt = 10;

#endif

/******************************************************************************
* function : show usage
******************************************************************************/
void SAMPLE_VENC_Usage(char *sPrgNm)
{
    printf("Usage : %s <index>\n", sPrgNm);
    printf("index:\n");
    printf("\t 0) 1*1080p H264 + 1*VGA H264.\n");
    printf("\t 1) 1*1080p MJPEG encode + 1*1080p jpeg.\n");
    printf("\t 2) low delay encode(only vi-vpss online).\n");
    printf("\t 3) roi background framerate.\n");
    printf("\t 4) Thumbnail of 1*1080p jpeg.\n");
#ifndef hi3518ev201
    printf("\t 5) svc-t H264\n");
#endif
    return;
}

/******************************************************************************
* function : to process abnormal case                                         
******************************************************************************/
void SAMPLE_VENC_HandleSig(HI_S32 signo)
{
    if (SIGINT == signo || SIGTERM == signo)
    {
        SAMPLE_COMM_ISP_Stop();
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }
    exit(-1);
}

/******************************************************************************
* function : to process abnormal case - the case of stream venc
******************************************************************************/
void SAMPLE_VENC_StreamHandleSig(HI_S32 signo)
{

    if (SIGINT == signo || SIGTSTP == signo)
    {
        SAMPLE_COMM_ISP_Stop();
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram exit abnormally!\033[0;39m\n");
    }

    exit(0);
}

/******************************************************************************
* function :  H.264@1080p@30fps+H.264@VGA@30fps
******************************************************************************/
HI_S32 SAMPLE_VENC_1080P_CLASSIC(HI_VOID)
{
    PAYLOAD_TYPE_E enPayLoad[3] = {PT_H264, PT_H264, PT_H264};
    PIC_SIZE_E enSize[3] = {PIC_HD1080, PIC_VGA, PIC_QVGA};
    HI_U32 u32Profile = 0;

    VB_CONF_S stVbConf;
    SAMPLE_VI_CONFIG_S stViConfig = {0};

    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr;
    VPSS_CHN_ATTR_S stVpssChnAttr;
    VPSS_CHN_MODE_S stVpssChnMode;

    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode = SAMPLE_RC_CBR;

    HI_S32 s32ChnNum = 0;

    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    char c;

    /******************************************
     step  1: init sys variable 
    ******************************************/
    memset(&stVbConf, 0, sizeof(VB_CONF_S));

    SAMPLE_COMM_VI_GetSizeBySensor(&enSize[0]);
    if (PIC_HD1080 == enSize[0])
    {
        enSize[1] = PIC_VGA;
        s32ChnNum = 2;
    }
    else if (PIC_HD720 == enSize[0])
    {
        enSize[1] = PIC_VGA;
        enSize[2] = PIC_QVGA;
        s32ChnNum = 3;
    }
    else
    {
        printf("not support this sensor\n");
        return HI_FAILURE;
    }
#ifdef hi3518ev201
    s32ChnNum = 1;
#endif
    printf("s32ChnNum = %d\n", s32ChnNum);

    stVbConf.u32MaxPoolCnt = 128;

    /*video buffer*/
    if (s32ChnNum >= 1)
    {
        u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,
                                                      enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
        stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
        stVbConf.astCommPool[0].u32BlkCnt = g_u32BlkCnt;
    }
    if (s32ChnNum >= 2)
    {
        u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,
                                                      enSize[1], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
        stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
        stVbConf.astCommPool[1].u32BlkCnt = g_u32BlkCnt;
    }
    if (s32ChnNum >= 3)
    {
        u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,
                                                      enSize[2], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
        stVbConf.astCommPool[2].u32BlkSize = u32BlkSize;
        stVbConf.astCommPool[2].u32BlkCnt = g_u32BlkCnt;
    }

    /******************************************
     step 2: mpp system init. 
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_1080P_CLASSIC_0;
    }

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    stViConfig.enViMode = SENSOR_TYPE;
    stViConfig.enRotate = ROTATE_NONE;
    stViConfig.enNorm = VIDEO_ENCODING_MODE_AUTO;
    stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
    stViConfig.enWDRMode = WDR_MODE_NONE;
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }

    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[0], &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }
    if (s32ChnNum >= 1)
    {
        VpssGrp = 0;
        stVpssGrpAttr.u32MaxW = stSize.u32Width;
        stVpssGrpAttr.u32MaxH = stSize.u32Height;
        stVpssGrpAttr.bIeEn = HI_FALSE;
        stVpssGrpAttr.bNrEn = HI_TRUE;
        stVpssGrpAttr.bHistEn = HI_FALSE;
        stVpssGrpAttr.bDciEn = HI_FALSE;
        stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
        stVpssGrpAttr.enPixFmt = PIXEL_FORMAT_YUV_SEMIPLANAR_420;

        s32Ret = SAMPLE_COMM_VPSS_StartGroup(VpssGrp, &stVpssGrpAttr);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Vpss failed!\n");
            goto END_VENC_1080P_CLASSIC_2;
        }

        s32Ret = SAMPLE_COMM_VI_BindVpss(stViConfig.enViMode);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Vi bind Vpss failed!\n");
            goto END_VENC_1080P_CLASSIC_3;
        }

        VpssChn = 0;
        stVpssChnMode.enChnMode = VPSS_CHN_MODE_USER;
        stVpssChnMode.bDouble = HI_FALSE;
        stVpssChnMode.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
        stVpssChnMode.u32Width = stSize.u32Width;
        stVpssChnMode.u32Height = stSize.u32Height;
        stVpssChnMode.enCompressMode = COMPRESS_MODE_SEG;
        memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
        stVpssChnAttr.s32SrcFrameRate = -1;
        stVpssChnAttr.s32DstFrameRate = -1;
        s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Enable vpss chn failed!\n");
            goto END_VENC_1080P_CLASSIC_4;
        }
    }

    if (s32ChnNum >= 2)
    {
        s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[1], &stSize);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
            goto END_VENC_1080P_CLASSIC_4;
        }
        VpssChn = 1;
        stVpssChnMode.enChnMode = VPSS_CHN_MODE_USER;
        stVpssChnMode.bDouble = HI_FALSE;
        stVpssChnMode.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
        stVpssChnMode.u32Width = stSize.u32Width;
        stVpssChnMode.u32Height = stSize.u32Height;
        stVpssChnMode.enCompressMode = COMPRESS_MODE_SEG;
        stVpssChnAttr.s32SrcFrameRate = -1;
        stVpssChnAttr.s32DstFrameRate = -1;
        s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Enable vpss chn failed!\n");
            goto END_VENC_1080P_CLASSIC_4;
        }
    }

    if (s32ChnNum >= 3)
    {
        s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[2], &stSize);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
            goto END_VENC_1080P_CLASSIC_4;
        }
        VpssChn = 2;
        stVpssChnMode.enChnMode = VPSS_CHN_MODE_USER;
        stVpssChnMode.bDouble = HI_FALSE;
        stVpssChnMode.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
        stVpssChnMode.u32Width = stSize.u32Width;
        stVpssChnMode.u32Height = stSize.u32Height;
        stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;

        stVpssChnAttr.s32SrcFrameRate = -1;
        stVpssChnAttr.s32DstFrameRate = -1;

        s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Enable vpss chn failed!\n");
            goto END_VENC_1080P_CLASSIC_4;
        }
    }
    /******************************************
     step 5: start stream venc
    ******************************************/
    /*** HD1080P **/
    printf("\t c) cbr.\n");
    printf("\t v) vbr.\n");
    printf("\t f) fixQp\n");
    printf("please input choose rc mode!\n");
    // c = (char)getchar();
    c = 'c';
    switch (c)
    {
    case 'c':
        enRcMode = SAMPLE_RC_CBR;
        break;
    case 'v':
        enRcMode = SAMPLE_RC_VBR;
        break;
    case 'f':
        enRcMode = SAMPLE_RC_FIXQP;
        break;
    default:
        printf("rc mode! is invaild!\n");
        goto END_VENC_1080P_CLASSIC_4;
    }

    /*** enSize[0] **/
    if (s32ChnNum >= 1)
    {
        VpssGrp = 0;
        VpssChn = 0;
        VencChn = 0;
        s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[0],
                                        gs_enNorm, enSize[0], enRcMode, u32Profile);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_1080P_CLASSIC_5;
        }

        s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_1080P_CLASSIC_5;
        }
    }

    /*** enSize[1] **/
    if (s32ChnNum >= 2)
    {
        VpssChn = 1;
        VencChn = 1;
        s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[1],
                                        gs_enNorm, enSize[1], enRcMode, u32Profile);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_1080P_CLASSIC_5;
        }

        s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_1080P_CLASSIC_5;
        }
    }
    /*** enSize[2] **/
    if (s32ChnNum >= 3)
    {
        VpssChn = 2;
        VencChn = 2;
        s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[2],
                                        gs_enNorm, enSize[2], enRcMode, u32Profile);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_1080P_CLASSIC_5;
        }

        s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_1080P_CLASSIC_5;
        }
    }
    /******************************************
     step 6: stream venc process -- get stream, then save it to file. 
    ******************************************/
    PostH264Stream(s32ChnNum);
    // s32Ret = SAMPLE_COMM_VENC_StartGetStream(s32ChnNum);
    // if (HI_SUCCESS != s32Ret)
    // {
    //     SAMPLE_PRT("Start Venc failed!\n");
    //     goto END_VENC_1080P_CLASSIC_5;
    // }

    printf("please press twice ENTER to exit this sample\n");
    getchar();
    getchar();

    /******************************************
     step 7: exit process
    ******************************************/
    SAMPLE_COMM_VENC_StopGetStream();

END_VENC_1080P_CLASSIC_5:

    VpssGrp = 0;
    switch (s32ChnNum)
    {
    case 3:
        VpssChn = 2;
        VencChn = 2;
        SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
        SAMPLE_COMM_VENC_Stop(VencChn);
    case 2:
        VpssChn = 1;
        VencChn = 1;
        SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
        SAMPLE_COMM_VENC_Stop(VencChn);
    case 1:
        VpssChn = 0;
        VencChn = 0;
        SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
        SAMPLE_COMM_VENC_Stop(VencChn);
        break;
    }
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);

END_VENC_1080P_CLASSIC_4: //vpss stop

    VpssGrp = 0;
    switch (s32ChnNum)
    {
    case 3:
        VpssChn = 2;
        SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
    case 2:
        VpssChn = 1;
        SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
    case 1:
        VpssChn = 0;
        SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
        break;
    }

END_VENC_1080P_CLASSIC_3: //vpss stop
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_1080P_CLASSIC_2: //vpss stop
    SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
END_VENC_1080P_CLASSIC_1: //vi stop
    SAMPLE_COMM_VI_StopVi(&stViConfig);
END_VENC_1080P_CLASSIC_0: //system exit
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}

/******************************************************************************
* function :  1*1080p MJPEG encode + 1*1080p jpeg
******************************************************************************/
HI_S32 SAMPLE_VENC_1080P_MJPEG_JPEG(HI_VOID)
{
    PAYLOAD_TYPE_E enPayLoad = PT_MJPEG;
    PIC_SIZE_E enSize = PIC_HD1080;

    HI_U32 u32Profile = 0;

    VB_CONF_S stVbConf;
    SAMPLE_VI_CONFIG_S stViConfig = {0};

    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr;
    VPSS_CHN_ATTR_S stVpssChnAttr;
    VPSS_CHN_MODE_S stVpssChnMode;

    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode = SAMPLE_RC_CBR;
    HI_S32 s32ChnNum = 1;

    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    HI_S32 i = 0;
    char ch;

    /******************************************
     step  1: init sys variable 
    ******************************************/
    memset(&stVbConf, 0, sizeof(VB_CONF_S));

    stVbConf.u32MaxPoolCnt = 128;
    SAMPLE_COMM_VI_GetSizeBySensor(&enSize);

    /*video buffer*/
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,
                                                  enSize, SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);

    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt = g_u32BlkCnt;

    /******************************************
     step 2: mpp system init. 
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_MJPEG_JPEG_0;
    }

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    stViConfig.enViMode = SENSOR_TYPE;
    stViConfig.enRotate = ROTATE_NONE;
    stViConfig.enNorm = VIDEO_ENCODING_MODE_AUTO;
    stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_MJPEG_JPEG_1;
    }

    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_MJPEG_JPEG_1;
    }

    VpssGrp = 0;
    stVpssGrpAttr.u32MaxW = stSize.u32Width;
    stVpssGrpAttr.u32MaxH = stSize.u32Height;
    stVpssGrpAttr.bIeEn = HI_FALSE;
    stVpssGrpAttr.bNrEn = HI_TRUE;
    stVpssGrpAttr.bHistEn = HI_FALSE;
    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
    stVpssGrpAttr.enPixFmt = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    stVpssGrpAttr.bDciEn = HI_FALSE;
    s32Ret = SAMPLE_COMM_VPSS_StartGroup(VpssGrp, &stVpssGrpAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Vpss failed!\n");
        goto END_VENC_MJPEG_JPEG_2;
    }

    s32Ret = SAMPLE_COMM_VI_BindVpss(stViConfig.enViMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Vi bind Vpss failed!\n");
        goto END_VENC_MJPEG_JPEG_3;
    }

    VpssChn = 0;
    stVpssChnMode.enChnMode = VPSS_CHN_MODE_USER;
    stVpssChnMode.bDouble = HI_FALSE;
    stVpssChnMode.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    stVpssChnMode.u32Width = stSize.u32Width;
    stVpssChnMode.u32Height = stSize.u32Height;
    stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;

    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
    stVpssChnAttr.s32SrcFrameRate = -1;
    stVpssChnAttr.s32DstFrameRate = -1;
    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Enable vpss chn failed!\n");
        goto END_VENC_MJPEG_JPEG_4;
    }

    VpssChn = 1;
    stVpssChnMode.enChnMode = VPSS_CHN_MODE_USER;
    stVpssChnMode.bDouble = HI_FALSE;
    stVpssChnMode.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    stVpssChnMode.u32Width = stSize.u32Width;
    stVpssChnMode.u32Height = stSize.u32Height;
    stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;

    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
    stVpssChnAttr.s32SrcFrameRate = -1;
    stVpssChnAttr.s32DstFrameRate = -1;
    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Enable vpss chn failed!\n");
        goto END_VENC_MJPEG_JPEG_4;
    }

    /******************************************
     step 5: start stream venc
    ******************************************/
    VpssGrp = 0;
    VpssChn = 0;
    VencChn = 0;
    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad,
                                    gs_enNorm, enSize, enRcMode, u32Profile);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_MJPEG_JPEG_5;
    }

    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_MJPEG_JPEG_5;
    }

    VpssGrp = 0;
    VpssChn = 1;
    VencChn = 1;
    s32Ret = SAMPLE_COMM_VENC_SnapStart(VencChn, &stSize, HI_FALSE);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start snap failed!\n");
        goto END_VENC_MJPEG_JPEG_5;
    }

    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_MJPEG_JPEG_5;
    }

    /******************************************
     step 6: stream venc process -- get stream, then save it to file. 
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(s32ChnNum);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_MJPEG_JPEG_5;
    }

    printf("press 'q' to exit sample!\nperess ENTER to capture one picture to file\n");
    i = 0;
    while ((ch = (char)getchar()) != 'q')
    {
        s32Ret = SAMPLE_COMM_VENC_SnapProcess(VencChn, HI_TRUE, HI_FALSE);
        if (HI_SUCCESS != s32Ret)
        {
            printf("%s: sanp process failed!\n", __FUNCTION__);
            break;
        }
        printf("snap %d success!\n", i);
        i++;
    }

    printf("please press ENTER to exit this sample\n");
    getchar();
    getchar();

    /******************************************
     step 8: exit process
    ******************************************/
    SAMPLE_COMM_VENC_StopGetStream();

END_VENC_MJPEG_JPEG_5:
    VpssGrp = 0;
    VpssChn = 0;
    VencChn = 0;
    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
    SAMPLE_COMM_VENC_Stop(VencChn);

    VpssChn = 1;
    VencChn = 1;
    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
    SAMPLE_COMM_VENC_Stop(VencChn);
END_VENC_MJPEG_JPEG_4: //vpss stop
    VpssGrp = 0;
    VpssChn = 0;
    SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
    VpssChn = 1;
    SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
END_VENC_MJPEG_JPEG_3: //vpss stop
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_MJPEG_JPEG_2: //vpss stop
    SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
END_VENC_MJPEG_JPEG_1: //vi stop
    SAMPLE_COMM_VI_StopVi(&stViConfig);
END_VENC_MJPEG_JPEG_0: //system exit
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}

/******************************************************************************
* function :  low delay encode(only vi-vpss online).
******************************************************************************/
HI_S32 SAMPLE_VENC_LOW_DELAY(HI_VOID)
{
    PAYLOAD_TYPE_E enPayLoad[2] = {PT_H264, PT_H264};
    PIC_SIZE_E enSize[2] = {PIC_HD1080, PIC_VGA};
    HI_U32 u32Profile = 0;

    VB_CONF_S stVbConf;
    SAMPLE_VI_CONFIG_S stViConfig = {0};
    HI_U32 u32Priority;

    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr;
    VPSS_CHN_ATTR_S stVpssChnAttr;
    VPSS_CHN_MODE_S stVpssChnMode;
    VPSS_LOW_DELAY_INFO_S stLowDelayInfo;

    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode = SAMPLE_RC_CBR;
    HI_S32 s32ChnNum = 2;

    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    char c;

    /******************************************
     step  1: init sys variable 
    ******************************************/
    memset(&stVbConf, 0, sizeof(VB_CONF_S));

    SAMPLE_COMM_VI_GetSizeBySensor(&enSize[0]);
    if (PIC_HD1080 == enSize[0])
    {
        enSize[1] = PIC_VGA;
        s32ChnNum = 2;
    }
    else if (PIC_HD720 == enSize[0])
    {
        enSize[1] = PIC_VGA;
        s32ChnNum = 2;
    }
    else
    {
        printf("not support this sensor\n");
        return HI_FAILURE;
    }
#ifdef hi3518ev201

    s32ChnNum = 1;

#endif
    stVbConf.u32MaxPoolCnt = 128;

    /*video buffer*/
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,
                                                  enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);

    if (s32ChnNum >= 1)
    {
        printf("u32BlkSize: %d\n", u32BlkSize);
        stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
        stVbConf.astCommPool[0].u32BlkCnt = g_u32BlkCnt;
    }
    if (s32ChnNum >= 2)
    {
        u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,
                                                      enSize[1], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
        stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
        stVbConf.astCommPool[1].u32BlkCnt = g_u32BlkCnt;
    }
    /******************************************
     step 2: mpp system init. 
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_LOW_DELAY_0;
    }

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    stViConfig.enViMode = SENSOR_TYPE;
    stViConfig.enRotate = ROTATE_NONE;
    stViConfig.enNorm = VIDEO_ENCODING_MODE_AUTO;
    stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_LOW_DELAY_1;
    }

    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[0], &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_LOW_DELAY_1;
    }

    VpssGrp = 0;
    stVpssGrpAttr.u32MaxW = stSize.u32Width;
    stVpssGrpAttr.u32MaxH = stSize.u32Height;
    stVpssGrpAttr.bIeEn = HI_FALSE;
    stVpssGrpAttr.bNrEn = HI_TRUE;
    stVpssGrpAttr.bHistEn = HI_FALSE;
    stVpssGrpAttr.bDciEn = HI_FALSE;
    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
    stVpssGrpAttr.enPixFmt = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    s32Ret = SAMPLE_COMM_VPSS_StartGroup(VpssGrp, &stVpssGrpAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Vpss failed!\n");
        goto END_VENC_LOW_DELAY_2;
    }

    s32Ret = SAMPLE_COMM_VI_BindVpss(stViConfig.enViMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Vi bind Vpss failed!\n");
        goto END_VENC_LOW_DELAY_3;
    }

    VpssChn = 0;
    stVpssChnMode.enChnMode = VPSS_CHN_MODE_USER;
    stVpssChnMode.bDouble = HI_FALSE;
    stVpssChnMode.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    stVpssChnMode.u32Width = stSize.u32Width;
    stVpssChnMode.u32Height = stSize.u32Height;
    stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;
    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
    stVpssChnAttr.s32SrcFrameRate = -1;
    stVpssChnAttr.s32DstFrameRate = -1;
    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Enable vpss chn failed!\n");
        goto END_VENC_LOW_DELAY_4;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[1], &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_LOW_DELAY_4;
    }
    VpssChn = 1;
    stVpssChnMode.enChnMode = VPSS_CHN_MODE_USER;
    stVpssChnMode.bDouble = HI_FALSE;
    stVpssChnMode.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    stVpssChnMode.u32Width = stSize.u32Width;
    stVpssChnMode.u32Height = stSize.u32Height;
    stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;
    stVpssChnAttr.s32SrcFrameRate = -1;
    stVpssChnAttr.s32DstFrameRate = -1;
    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Enable vpss chn failed!\n");
        goto END_VENC_LOW_DELAY_4;
    }

    /******************************************
     step 5: start stream venc
    ******************************************/
    /*** HD1080P **/
    printf("\t c) cbr.\n");
    printf("\t v) vbr.\n");
    printf("\t f) fixQp\n");
    printf("please input choose rc mode!\n");
    c = (char)getchar();
    switch (c)
    {
    case 'c':
        enRcMode = SAMPLE_RC_CBR;
        break;
    case 'v':
        enRcMode = SAMPLE_RC_VBR;
        break;
    case 'f':
        enRcMode = SAMPLE_RC_FIXQP;
        break;
    default:
        printf("rc mode! is invaild!\n");
        goto END_VENC_LOW_DELAY_4;
    }
    if (s32ChnNum >= 1)
    {
        VpssGrp = 0;
        VpssChn = 0;
        VencChn = 0;
        s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[0],
                                        gs_enNorm, enSize[0], enRcMode, u32Profile);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_LOW_DELAY_5;
        }

        s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_LOW_DELAY_5;
        }

        /*set chnl Priority*/
        s32Ret = HI_MPI_VENC_GetChnlPriority(VencChn, &u32Priority);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Get Chnl Priority failed!\n");
            goto END_VENC_LOW_DELAY_5;
        }

        u32Priority = 1;

        s32Ret = HI_MPI_VENC_SetChnlPriority(VencChn, u32Priority);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Set Chnl Priority failed!\n");
            goto END_VENC_LOW_DELAY_5;
        }

/*set low delay*/
#if 1
        s32Ret = HI_MPI_VPSS_GetLowDelayAttr(VpssGrp, VpssChn, &stLowDelayInfo);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("HI_MPI_VPSS_GetLowDelayAttr failed!\n");
            goto END_VENC_LOW_DELAY_5;
        }
        stLowDelayInfo.bEnable = HI_TRUE;
        stLowDelayInfo.u32LineCnt = stVpssChnMode.u32Height / 2;
        s32Ret = HI_MPI_VPSS_SetLowDelayAttr(VpssGrp, VpssChn, &stLowDelayInfo);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("HI_MPI_VPSS_SetLowDelayAttr failed!\n");
            goto END_VENC_LOW_DELAY_5;
        }
#endif
    }

    /*** 1080p **/
    if (s32ChnNum >= 2)
    {
        VpssChn = 1;
        VencChn = 1;
        s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[1],
                                        gs_enNorm, enSize[1], enRcMode, u32Profile);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_LOW_DELAY_5;
        }

        s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("Start Venc failed!\n");
            goto END_VENC_LOW_DELAY_5;
        }
    }

    /******************************************
     step 6: stream venc process -- get stream, then save it to file. 
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(s32ChnNum);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_LOW_DELAY_5;
    }

    printf("please press twice ENTER to exit this sample\n");
    getchar();
    getchar();

    /******************************************
     step 7: exit process
    ******************************************/
    SAMPLE_COMM_VENC_StopGetStream();

END_VENC_LOW_DELAY_5:
    VpssGrp = 0;

    if (s32ChnNum >= 1)
    {
        VpssChn = 0;
        VencChn = 0;
        SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
        SAMPLE_COMM_VENC_Stop(VencChn);
    }
    if (s32ChnNum >= 2)
    {
        VpssChn = 1;
        VencChn = 1;
        SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
        SAMPLE_COMM_VENC_Stop(VencChn);
    }
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_LOW_DELAY_4: //vpss stop
    VpssGrp = 0;
    VpssChn = 0;
    SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
    VpssChn = 1;
    SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
END_VENC_LOW_DELAY_3: //vpss stop
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_LOW_DELAY_2: //vpss stop
    SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
END_VENC_LOW_DELAY_1: //vi stop
    SAMPLE_COMM_VI_StopVi(&stViConfig);
END_VENC_LOW_DELAY_0: //system exit
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}

HI_S32 SAMPLE_VENC_ROIBG_CLASSIC(HI_VOID)
{
    PAYLOAD_TYPE_E enPayLoad = PT_H264;
    PIC_SIZE_E enSize[3] = {PIC_HD1080, PIC_VGA, PIC_QVGA};
    HI_U32 u32Profile = 0;

    VB_CONF_S stVbConf;
    SAMPLE_VI_CONFIG_S stViConfig = {0};

    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr;
    VPSS_CHN_ATTR_S stVpssChnAttr;
    VPSS_CHN_MODE_S stVpssChnMode;
    VENC_ROI_CFG_S stVencRoiCfg;
    VENC_ROIBG_FRAME_RATE_S stRoiBgFrameRate;

    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode = SAMPLE_RC_CBR;
    HI_S32 s32ChnNum = 1;

    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    char c;

    /******************************************
     step  1: init sys variable 
    ******************************************/
    memset(&stVbConf, 0, sizeof(VB_CONF_S));

    SAMPLE_COMM_VI_GetSizeBySensor(&enSize[0]);
    if (PIC_HD1080 == enSize[0])
    {
        s32ChnNum = 1;
    }
    else if (PIC_HD720 == enSize[0])
    {
        s32ChnNum = 1;
    }
    else
    {
        printf("not support this sensor\n");
        return HI_FAILURE;
    }

    stVbConf.u32MaxPoolCnt = 128;

    /*video buffer*/
    if (s32ChnNum >= 1)
    {
        u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,
                                                      enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
        stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
        stVbConf.astCommPool[0].u32BlkCnt = g_u32BlkCnt;
    }
    if (s32ChnNum >= 2)
    {
        u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,
                                                      enSize[1], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
        stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
        stVbConf.astCommPool[1].u32BlkCnt = g_u32BlkCnt;
    }
    if (s32ChnNum >= 3)
    {
        u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,
                                                      enSize[2], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
        stVbConf.astCommPool[2].u32BlkSize = u32BlkSize;
        stVbConf.astCommPool[2].u32BlkCnt = g_u32BlkCnt;
    }

    /******************************************
     step 2: mpp system init. 
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_1080P_CLASSIC_0;
    }

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    stViConfig.enViMode = SENSOR_TYPE;
    stViConfig.enRotate = ROTATE_NONE;
    stViConfig.enNorm = VIDEO_ENCODING_MODE_AUTO;
    stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }

    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[0], &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }

    VpssGrp = 0;
    stVpssGrpAttr.u32MaxW = stSize.u32Width;
    stVpssGrpAttr.u32MaxH = stSize.u32Height;
    stVpssGrpAttr.bIeEn = HI_FALSE;
    stVpssGrpAttr.bNrEn = HI_TRUE;
    stVpssGrpAttr.bHistEn = HI_FALSE;
    stVpssGrpAttr.bDciEn = HI_FALSE;
    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
    stVpssGrpAttr.enPixFmt = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    s32Ret = SAMPLE_COMM_VPSS_StartGroup(VpssGrp, &stVpssGrpAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Vpss failed!\n");
        goto END_VENC_1080P_CLASSIC_2;
    }

    s32Ret = SAMPLE_COMM_VI_BindVpss(stViConfig.enViMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Vi bind Vpss failed!\n");
        goto END_VENC_1080P_CLASSIC_3;
    }

    VpssChn = 0;
    stVpssChnMode.enChnMode = VPSS_CHN_MODE_USER;
    stVpssChnMode.bDouble = HI_FALSE;
    stVpssChnMode.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    stVpssChnMode.u32Width = stSize.u32Width;
    stVpssChnMode.u32Height = stSize.u32Height;
    stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;
    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
    stVpssChnAttr.s32SrcFrameRate = -1;
    stVpssChnAttr.s32DstFrameRate = -1;
    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Enable vpss chn failed!\n");
        goto END_VENC_1080P_CLASSIC_4;
    }

    /******************************************
     step 5: start stream venc
    ******************************************/
    /*** HD1080P **/
    printf("\t c) cbr.\n");
    printf("\t v) vbr.\n");
    printf("\t f) fixQp\n");
    printf("please input choose rc mode!\n");
    c = (char)getchar();
    switch (c)
    {
    case 'c':
        enRcMode = SAMPLE_RC_CBR;
        break;
    case 'v':
        enRcMode = SAMPLE_RC_VBR;
        break;
    case 'f':
        enRcMode = SAMPLE_RC_FIXQP;
        break;
    default:
        printf("rc mode! is invaild!\n");
        goto END_VENC_1080P_CLASSIC_4;
    }
    VpssGrp = 0;
    VpssChn = 0;
    VencChn = 0;
    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad,
                                    gs_enNorm, enSize[0], enRcMode, u32Profile);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_1080P_CLASSIC_5;
    }

    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_1080P_CLASSIC_5;
    }
    stVencRoiCfg.bAbsQp = HI_TRUE;
    stVencRoiCfg.bEnable = HI_TRUE;
    stVencRoiCfg.s32Qp = 30;
    stVencRoiCfg.u32Index = 0;
    stVencRoiCfg.stRect.s32X = 64;
    stVencRoiCfg.stRect.s32Y = 64;
    stVencRoiCfg.stRect.u32Height = 256;
    stVencRoiCfg.stRect.u32Width = 256;
    s32Ret = HI_MPI_VENC_SetRoiCfg(VencChn, &stVencRoiCfg);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_1080P_CLASSIC_5;
    }

    s32Ret = HI_MPI_VENC_GetRoiBgFrameRate(VencChn, &stRoiBgFrameRate);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VENC_GetRoiBgFrameRate failed!\n");
        goto END_VENC_1080P_CLASSIC_5;
    }
    stRoiBgFrameRate.s32SrcFrmRate = (VIDEO_ENCODING_MODE_PAL == gs_enNorm) ? 25 : 30;
    stRoiBgFrameRate.s32DstFrmRate = (VIDEO_ENCODING_MODE_PAL == gs_enNorm) ? 5 : 15;

    s32Ret = HI_MPI_VENC_SetRoiBgFrameRate(VencChn, &stRoiBgFrameRate);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VENC_SetRoiBgFrameRate!\n");
        goto END_VENC_1080P_CLASSIC_5;
    }
    /******************************************
     step 6: stream venc process -- get stream, then save it to file. 
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(s32ChnNum);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_1080P_CLASSIC_5;
    }

    printf("please press ENTER to exit this sample\n");
    getchar();
    getchar();

    /******************************************
     step 7: exit process
    ******************************************/
    SAMPLE_COMM_VENC_StopGetStream();

END_VENC_1080P_CLASSIC_5:
    VpssGrp = 0;

    VpssChn = 0;
    VencChn = 0;
    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
    SAMPLE_COMM_VENC_Stop(VencChn);

    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_1080P_CLASSIC_4: //vpss stop
    VpssGrp = 0;
    VpssChn = 0;
    SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
END_VENC_1080P_CLASSIC_3: //vpss stop
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_1080P_CLASSIC_2: //vpss stop
    SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
END_VENC_1080P_CLASSIC_1: //vi stop
    SAMPLE_COMM_VI_StopVi(&stViConfig);
END_VENC_1080P_CLASSIC_0: //system exit
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}

HI_S32 SAMPLE_VENC_SVC_H264(HI_VOID)
{
    PAYLOAD_TYPE_E enPayLoad = PT_H264;
    PIC_SIZE_E enSize[3] = {PIC_HD1080, PIC_HD720, PIC_D1};
    HI_U32 u32Profile = 3; /* Svc-t */

    VB_CONF_S stVbConf;
    SAMPLE_VI_CONFIG_S stViConfig = {0};

    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr;
    VPSS_CHN_ATTR_S stVpssChnAttr;
    VPSS_CHN_MODE_S stVpssChnMode;

    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode = SAMPLE_RC_CBR;
    HI_S32 s32ChnNum = 1;

    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    char c;

    /******************************************
	 step  1: init sys variable 
	******************************************/
    memset(&stVbConf, 0, sizeof(VB_CONF_S));

    SAMPLE_COMM_VI_GetSizeBySensor(&enSize[0]);
    if (PIC_HD1080 == enSize[0])
    {
        s32ChnNum = 1;
    }
    else if (PIC_HD720 == enSize[0])
    {
        s32ChnNum = 1;
    }
    else
    {
        printf("not support this sensor\n");
        return HI_FAILURE;
    }

    stVbConf.u32MaxPoolCnt = 128;

    /*video buffer*/
    if (s32ChnNum >= 1)
    {
        u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,
                                                      enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
        stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
        stVbConf.astCommPool[0].u32BlkCnt = g_u32BlkCnt;
    }
    if (s32ChnNum >= 2)
    {
        u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,
                                                      enSize[1], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
        stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
        stVbConf.astCommPool[1].u32BlkCnt = g_u32BlkCnt;
    }

    if (s32ChnNum >= 1)
    {
        u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,
                                                      enSize[2], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
        stVbConf.astCommPool[2].u32BlkSize = u32BlkSize;
        stVbConf.astCommPool[2].u32BlkCnt = g_u32BlkCnt;
    }

    /******************************************
	 step 2: mpp system init. 
	******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_1080P_CLASSIC_0;
    }

    /******************************************
	 step 3: start vi dev & chn to capture
	******************************************/
    stViConfig.enViMode = SENSOR_TYPE;
    stViConfig.enRotate = ROTATE_NONE;
    stViConfig.enNorm = VIDEO_ENCODING_MODE_AUTO;
    stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }

    /******************************************
	 step 4: start vpss and vi bind vpss
	******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[0], &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }

    VpssGrp = 0;
    stVpssGrpAttr.u32MaxW = stSize.u32Width;
    stVpssGrpAttr.u32MaxH = stSize.u32Height;
    stVpssGrpAttr.bIeEn = HI_FALSE;
    stVpssGrpAttr.bNrEn = HI_TRUE;
    stVpssGrpAttr.bHistEn = HI_FALSE;
    stVpssGrpAttr.bDciEn = HI_FALSE;
    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
    stVpssGrpAttr.enPixFmt = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    s32Ret = SAMPLE_COMM_VPSS_StartGroup(VpssGrp, &stVpssGrpAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Vpss failed!\n");
        goto END_VENC_1080P_CLASSIC_2;
    }

    s32Ret = SAMPLE_COMM_VI_BindVpss(stViConfig.enViMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Vi bind Vpss failed!\n");
        goto END_VENC_1080P_CLASSIC_3;
    }

    VpssChn = 0;
    stVpssChnMode.enChnMode = VPSS_CHN_MODE_USER;
    stVpssChnMode.bDouble = HI_FALSE;
    stVpssChnMode.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    stVpssChnMode.u32Width = stSize.u32Width;
    stVpssChnMode.u32Height = stSize.u32Height;
    stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;
    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
    stVpssChnAttr.s32SrcFrameRate = -1;
    stVpssChnAttr.s32DstFrameRate = -1;
    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Enable vpss chn failed!\n");
        goto END_VENC_1080P_CLASSIC_4;
    }

    /******************************************
	 step 5: start stream venc
	******************************************/
    /*** HD1080P **/
    printf("\t c) cbr.\n");
    printf("\t v) vbr.\n");
    printf("\t f) fixQp\n");
    printf("please input choose rc mode!\n");
    c = (char)getchar();
    switch (c)
    {
    case 'c':
        enRcMode = SAMPLE_RC_CBR;
        break;
    case 'v':
        enRcMode = SAMPLE_RC_VBR;
        break;
    case 'f':
        enRcMode = SAMPLE_RC_FIXQP;
        break;
    default:
        printf("rc mode! is invaild!\n");
        goto END_VENC_1080P_CLASSIC_4;
    }
    VpssGrp = 0;
    VpssChn = 0;
    VencChn = 0;
    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad,
                                    gs_enNorm, enSize[0], enRcMode, u32Profile);

    printf("SAMPLE_COMM_VENC_Start is ok\n");

    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_1080P_CLASSIC_5;
    }

    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);

    printf("SAMPLE_COMM_VENC_BindVpss is ok\n");

    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_1080P_CLASSIC_5;
    }

    /******************************************
	 step 6: stream venc process -- get stream, then save it to file. 
	******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream_Svc_t(s32ChnNum);

    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_1080P_CLASSIC_5;
    }

    printf("please press ENTER to exit this sample\n");
    getchar();
    getchar();

    /******************************************
	 step 7: exit process
	******************************************/
    SAMPLE_COMM_VENC_StopGetStream();

    printf("SAMPLE_COMM_VENC_StopGetStream is ok\n");
END_VENC_1080P_CLASSIC_5:
    VpssGrp = 0;

    VpssChn = 0;
    VencChn = 0;
    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
    SAMPLE_COMM_VENC_Stop(VencChn);

    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_1080P_CLASSIC_4: //vpss stop
    VpssGrp = 0;
    VpssChn = 0;
    SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
END_VENC_1080P_CLASSIC_3: //vpss stop
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_1080P_CLASSIC_2: //vpss stop
    SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
END_VENC_1080P_CLASSIC_1: //vi stop
    SAMPLE_COMM_VI_StopVi(&stViConfig);
END_VENC_1080P_CLASSIC_0: //system exit
    SAMPLE_COMM_SYS_Exit();
    return s32Ret;
}

/******************************
 *  Function: Get H264 and post it by pipe
 *******************************/

void *PostH264ByFIFO(void *chnTotal)
{
    HI_S32 i = 0;
    HI_S32 s32Cnt = 0;
    HI_S32 s32ChnTotal;
    VENC_CHN_ATTR_S stVencChnAttr;
    SAMPLE_VENC_GETSTREAM_PARA_S *pstPara;
    HI_S32 maxfd = 0;
    struct timeval TimeoutVal;
    fd_set read_fds;
    HI_S32 VencFd[VENC_MAX_CHN_NUM];
    HI_CHAR aszFileName[VENC_MAX_CHN_NUM][64];
    FILE *pFile[VENC_MAX_CHN_NUM];
    char szFilePostfix[10];
    VENC_CHN_STAT_S stStat;
    VENC_STREAM_S stStream;
    HI_S32 s32Ret;
    VENC_CHN VencChn;
    PAYLOAD_TYPE_E enPayLoadType[VENC_MAX_CHN_NUM];

    s32ChnTotal = *((HI_S32 *)chnTotal);

    /***************************
     * Init FIFO    
    ****************************/
    int pipe_fd = open(MKFIFO_PATH, O_WRONLY | O_CREAT, 0644);
    if (-1 == pipe_fd)
    {
        SAMPLE_PRT("open %s failed\n", MKFIFO_PATH);
        return NULL;
    }
    /******************************************
     step 1:  check & prepare save-file & venc-fd
    ******************************************/
    if (s32ChnTotal >= VENC_MAX_CHN_NUM)
    {
        SAMPLE_PRT("input count invaild\n");
        return NULL;
    }
    SAMPLE_PRT("1\n");
    for (i = 0; i < s32ChnTotal; i++)
    {
        /* decide the stream file name, and open file to save stream */
        VencChn = i;
        s32Ret = HI_MPI_VENC_GetChnAttr(VencChn, &stVencChnAttr);
        if (s32Ret != HI_SUCCESS)
        {
            SAMPLE_PRT("HI_MPI_VENC_GetChnAttr chn[%d] failed with %#x!\n",
                       VencChn, s32Ret);
            return NULL;
        }
        enPayLoadType[i] = stVencChnAttr.stVeAttr.enType;
        strcpy(szFilePostfix, ".h264");

        /* Set Venc Fd. */
        VencFd[i] = HI_MPI_VENC_GetFd(i);
        if (VencFd[i] < 0)
        {
            SAMPLE_PRT("HI_MPI_VENC_GetFd failed with %#x!\n",
                       VencFd[i]);
            return NULL;
        }
        if (maxfd <= VencFd[i])
        {
            maxfd = VencFd[i];
        }
    }
    /******************************************
     step 2:  Start to get streams of each channel.
    ******************************************/
    while(1)
    // while (HI_TRUE == pstPara->bThreadStart)
    {
        FD_ZERO(&read_fds);
        for (i = 0; i < s32ChnTotal; i++)
        {
            FD_SET(VencFd[i], &read_fds);
        }

        TimeoutVal.tv_sec = 2;
        TimeoutVal.tv_usec = 0;
        s32Ret = select(maxfd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0)
        {
            SAMPLE_PRT("select failed!\n");
            break;
        }
        else if (s32Ret == 0)
        {
            SAMPLE_PRT("get venc stream time out, exit thread\n");
            continue;
        }
        else
        {
            for (i = 0; i < s32ChnTotal; i++)
            {
                if (FD_ISSET(VencFd[i], &read_fds))
                {
                    /*******************************************************
                     step 2.1 : query how many packs in one-frame stream.
                    *******************************************************/
                    memset(&stStream, 0, sizeof(stStream));
                    s32Ret = HI_MPI_VENC_Query(i, &stStat);
                    if (HI_SUCCESS != s32Ret)
                    {
                        SAMPLE_PRT("HI_MPI_VENC_Query chn[%d] failed with %#x!\n", i, s32Ret);
                        break;
                    }
                    /*******************************************************
					step 2.2 :suggest to check both u32CurPacks and u32LeftStreamFrames at the same time,for example:
					 if(0 == stStat.u32CurPacks || 0 == stStat.u32LeftStreamFrames)
					 {
						SAMPLE_PRT("NOTE: Current  frame is NULL!\n");
						continue;
					 }
					*******************************************************/
                    if (0 == stStat.u32CurPacks)
                    {
                        SAMPLE_PRT("NOTE: Current  frame is NULL!\n");
                        continue;
                    }
                    /*******************************************************
                     step 2.3 : malloc corresponding number of pack nodes.
                    *******************************************************/
                    stStream.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
                    if (NULL == stStream.pstPack)
                    {
                        SAMPLE_PRT("malloc stream pack failed!\n");
                        break;
                    }
                    /*******************************************************
                     step 2.4 : call mpi to get one-frame stream
                    *******************************************************/
                    stStream.u32PackCount = stStat.u32CurPacks;
                    s32Ret = HI_MPI_VENC_GetStream(i, &stStream, HI_TRUE);
                    if (HI_SUCCESS != s32Ret)
                    {
                        free(stStream.pstPack);
                        stStream.pstPack = NULL;
                        SAMPLE_PRT("HI_MPI_VENC_GetStream failed with %#x!\n",
                                   s32Ret);
                        break;
                    }
                    /*******************************************************
                     step 2.5 : PostH264
                    *******************************************************/
                    for (i = 0; i < stStream.u32PackCount; i++)
                    {
                        SAMPLE_PRT("3\n");
                        SAMPLE_PRT("packindex:%d. len:%ld\n", i, stStream.pstPack[i].u32Len - stStream.pstPack[i].u32Offset);
                        write(pipe_fd, stStream.pstPack[i].pu8Addr + stStream.pstPack[i].u32Offset, stStream.pstPack[i].u32Len - stStream.pstPack[i].u32Offset);
                    }

                    /*******************************************************
                     step 2.6 : release stream
                    *******************************************************/
                    s32Ret = HI_MPI_VENC_ReleaseStream(i, &stStream);
                    if (HI_SUCCESS != s32Ret)
                    {
                        free(stStream.pstPack);
                        stStream.pstPack = NULL;
                        break;
                    }
                    /*******************************************************
                     step 2.7 : free pack nodes
                    *******************************************************/
                    free(stStream.pstPack);
                    stStream.pstPack = NULL;
                }
            }
        }
    }
    close(pipe_fd);

    return NULL;
}

HI_S32 g_chnTotal = 0;
HI_S32 PostH264Stream(HI_S32 s32Cnt)
{
    pthread_t thread_id;
    g_chnTotal = s32Cnt;
    return pthread_create(&thread_id, 0, PostH264ByFIFO, &g_chnTotal);
}

/******************************************************************************
* function :  Thumbnail of 1*1080p jpeg
******************************************************************************/
HI_S32 SAMPLE_VENC_1080P_JPEG_Thumb(HI_VOID)

{
    PIC_SIZE_E enSize = PIC_HD1080;
    ISP_DCF_INFO_S stIspDCF;

    VB_CONF_S stVbConf;
    SAMPLE_VI_CONFIG_S stViConfig = {0};

    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr;
    VPSS_CHN_ATTR_S stVpssChnAttr;
    VPSS_CHN_MODE_S stVpssChnMode;

    VENC_CHN VencChn;

    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    HI_S32 i = 0;
    char ch;

    /******************************************
     step  1: init sys variable 
    ******************************************/
    memset(&stVbConf, 0, sizeof(VB_CONF_S));

    stVbConf.u32MaxPoolCnt = 128;
    SAMPLE_COMM_VI_GetSizeBySensor(&enSize);

    /*video buffer*/
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,
                                                  enSize, SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);

    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt = g_u32BlkCnt;

    /******************************************
     step 2: mpp system init. 
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init_With_DCF(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_MJPEG_JPEG_0;
    }

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    stViConfig.enViMode = SENSOR_TYPE;
    stViConfig.enRotate = ROTATE_NONE;
    stViConfig.enNorm = VIDEO_ENCODING_MODE_AUTO;
    stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_MJPEG_JPEG_1;
    }

    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_MJPEG_JPEG_1;
    }

    VpssGrp = 0;
    stVpssGrpAttr.u32MaxW = stSize.u32Width;
    stVpssGrpAttr.u32MaxH = stSize.u32Height;
    stVpssGrpAttr.bIeEn = HI_FALSE;
    stVpssGrpAttr.bNrEn = HI_TRUE;
    stVpssGrpAttr.bHistEn = HI_FALSE;
    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
    stVpssGrpAttr.enPixFmt = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    stVpssGrpAttr.bDciEn = HI_FALSE;
    s32Ret = SAMPLE_COMM_VPSS_StartGroup(VpssGrp, &stVpssGrpAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Vpss failed!\n");
        goto END_VENC_MJPEG_JPEG_2;
    }

    s32Ret = SAMPLE_COMM_VI_BindVpss(stViConfig.enViMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Vi bind Vpss failed!\n");
        goto END_VENC_MJPEG_JPEG_3;
    }

    VpssChn = 0;
    stVpssChnMode.enChnMode = VPSS_CHN_MODE_USER;
    stVpssChnMode.bDouble = HI_FALSE;
    stVpssChnMode.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    stVpssChnMode.u32Width = stSize.u32Width;
    stVpssChnMode.u32Height = stSize.u32Height;
    stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;

    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
    stVpssChnAttr.s32SrcFrameRate = -1;
    stVpssChnAttr.s32DstFrameRate = -1;
    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Enable vpss chn failed!\n");
        goto END_VENC_MJPEG_JPEG_4;
    }

    /******************************************
     step 5: set CDF info
    ******************************************/

    HI_MPI_ISP_GetDCFInfo(&stIspDCF);

    //description: Thumbnail test
    memcpy(stIspDCF.au8ImageDescription, "Thumbnail test", strlen("Thumbnail test"));

    //manufacturer: Hisilicon
    memcpy(stIspDCF.au8Make, "Hisilicon", strlen("Hisilicon"));

    //model number: Hisilicon IP Camera
    memcpy(stIspDCF.au8Model, "Hisilicon IP Camera", strlen("Hisilicon IP Camera"));

    //firmware version: v.1.1.0
    memcpy(stIspDCF.au8Software, "v.1.1.0", strlen("v.1.1.0"));

    stIspDCF.u16ISOSpeedRatings = 500;
    stIspDCF.u32ExposureBiasValue = 5;
    stIspDCF.u32ExposureTime = 0x00010004;
    stIspDCF.u32FNumber = 0x0001000f;
    stIspDCF.u32FocalLength = 0x00640001;
    stIspDCF.u32MaxApertureValue = 0x00010001;
    stIspDCF.u8Contrast = 5;
    stIspDCF.u8CustomRendered = 0;
    stIspDCF.u8ExposureMode = 0;
    stIspDCF.u8ExposureProgram = 1;
    stIspDCF.u8FocalLengthIn35mmFilm = 1;
    stIspDCF.u8GainControl = 1;
    stIspDCF.u8LightSource = 1;
    stIspDCF.u8MeteringMode = 1;
    stIspDCF.u8Saturation = 1;
    stIspDCF.u8SceneCaptureType = 1;
    stIspDCF.u8SceneType = 0;
    stIspDCF.u8Sharpness = 5;
    stIspDCF.u8WhiteBalance = 1;

    HI_MPI_ISP_SetDCFInfo(&stIspDCF);

    /******************************************
     step 6: start stream venc
    ******************************************/
    VpssGrp = 0;
    VpssChn = 0;
    VencChn = 0;

    s32Ret = SAMPLE_COMM_VENC_SnapStart(VencChn, &stSize, HI_TRUE);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start snap failed!\n");
        goto END_VENC_MJPEG_JPEG_5;
    }

    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_MJPEG_JPEG_5;
    }

    /******************************************
     step 7: stream venc process -- get stream, then save it to file. 
    ******************************************/
    printf("press 'q' to exit sample!\nperess ENTER to capture one picture to file\n");
    i = 0;
    while ((ch = (char)getchar()) != 'q')
    {
        s32Ret = SAMPLE_COMM_VENC_SnapProcess(VencChn, HI_TRUE, HI_TRUE);
        if (HI_SUCCESS != s32Ret)
        {
            printf("%s: sanp process failed!\n", __FUNCTION__);
            break;
        }
        printf("snap %d success!\n", i);
        i++;
    }

    printf("please press ENTER to exit this sample\n");
    getchar();
    getchar();

    /******************************************
     step 8: exit process
    ******************************************/
    SAMPLE_COMM_VENC_StopGetStream();

END_VENC_MJPEG_JPEG_5:
    VpssGrp = 0;
    VpssChn = 0;
    VencChn = 0;
    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
    SAMPLE_COMM_VENC_Stop(VencChn);
END_VENC_MJPEG_JPEG_4: //vpss stop
    VpssGrp = 0;
    VpssChn = 0;
    SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
END_VENC_MJPEG_JPEG_3: //vpss stop
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_MJPEG_JPEG_2: //vpss stop
    SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
END_VENC_MJPEG_JPEG_1: //vi stop
    SAMPLE_COMM_VI_StopVi(&stViConfig);
END_VENC_MJPEG_JPEG_0: //system exit
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}

void *rtsp_thread(void *arg)
{
    TaskScheduler *scheduler = BasicTaskScheduler::createNew();
    UsageEnvironment *env = BasicUsageEnvironment::createNew(*scheduler);

    UserAuthenticationDatabase *authDB = NULL;
#ifdef ACCESS_CONTROL
    // To implement client access control to the RTSP server, do the following:
    authDB = new UserAuthenticationDatabase;
    authDB->addUserRecord("username1", "password1"); // replace these with real strings
    // Repeat the above with each <username>, <password> that you wish to allow
    // access to the server.
#endif

    RTSPServer *rtspServer;
    portNumBits rtspServerPortNum = 554;
    rtspServer = RTSPServer::createNew(*env, rtspServerPortNum, authDB);
    if (rtspServer == NULL)
    {
        rtspServerPortNum = 8554;
        rtspServer = RTSPServer::createNew(*env, rtspServerPortNum, authDB);
    }
    if (rtspServer == NULL)
    {
        *env << "Failed to create RTSP server:" << env->getResultMsg() << "\n";
        exit(1);
    }

    LIVE_H264VideoSource *videoSource = NULL;

    ServerMediaSession *sms = ServerMediaSession::createNew(*env, "live", 0, "live test");
    sms->addSubsession(LIVE_H264VideoServerMediaSubsession::createNew(*env, videoSource, "test.264"));
    rtspServer->addServerMediaSession(sms);

    char *url = rtspServer->rtspURL(sms);
    *env << "using url \"" << url << "\"\n";
    delete[] url;

    *env << "LIVE555 Media Server\n";
    *env << "\tversion " << MEDIA_SERVER_VERSION_STRING
         << " (LIVE555 Streaming Media library version "
         << LIVEMEDIA_LIBRARY_VERSION_STRING << ").\n";

    char *urlPrefix = rtspServer->rtspURLPrefix();
    *env << "Play streams from this server using the URL\n\t"
         << urlPrefix << "<filename>\nwhere <filename> is a file present in the current directory.\n";
    *env << "Each file's type is inferred from its name suffix:\n";
    *env << "\t\".264\" => a H.264 Video Elementary Stream file\n";
    *env << "\t\".265\" => a H.265 Video Elementary Stream file\n";
    *env << "\t\".aac\" => an AAC Audio (ADTS format) file\n";
    *env << "\t\".ac3\" => an AC-3 Audio file\n";
    *env << "\t\".amr\" => an AMR Audio file\n";
    *env << "\t\".dv\" => a DV Video file\n";
    *env << "\t\".m4e\" => a MPEG-4 Video Elementary Stream file\n";
    *env << "\t\".mkv\" => a Matroska audio+video+(optional)subtitles file\n";
    *env << "\t\".mp3\" => a MPEG-1 or 2 Audio file\n";
    *env << "\t\".mpg\" => a MPEG-1 or 2 Program Stream (audio+video) file\n";
    *env << "\t\".ogg\" or \".ogv\" or \".opus\" => an Ogg audio and/or video file\n";
    *env << "\t\".ts\" => a MPEG Transport Stream file\n";
    *env << "\t\t(a \".tsx\" index file - if present - provides server 'trick play' support)\n";
    *env << "\t\".vob\" => a VOB (MPEG-2 video with AC-3 audio) file\n";
    *env << "\t\".wav\" => a WAV Audio file\n";
    *env << "\t\".webm\" => a WebM audio(Vorbis)+video(VP8) file\n";
    *env << "See http://www.live555.com/mediaServer/ for additional documentation.\n";

    // Also, attempt to create a HTTP server for RTSP-over-HTTP tunneling.
    // Try first with the default HTTP port (80), and then with the alternative HTTP
    // port numbers (8000 and 8080).

    if (rtspServer->setUpTunnelingOverHTTP(80) || rtspServer->setUpTunnelingOverHTTP(8000) || rtspServer->setUpTunnelingOverHTTP(8080))
    {
        *env << "(We use port " << rtspServer->httpServerPortNum() << " for optional RTSP-over-HTTP tunneling, or for HTTP live streaming (for indexed Transport Stream files only).)\n";
    }
    else
    {
        *env << "(RTSP-over-HTTP tunneling is not available.)\n";
    }
    env->taskScheduler().doEventLoop(); // does not return

    return NULL;
}

/******************************************************************************
* function    : main()
* Description : video venc sample
******************************************************************************/
int main(int argc, char *argv[])
{
    pthread_t threadid;
    HI_S32 s32Ret;

    if ((argc < 2) || (1 != strlen(argv[1])))
    {
        SAMPLE_VENC_Usage(argv[0]);
        return HI_FAILURE;
    }

    signal(SIGINT, SAMPLE_VENC_HandleSig);
    signal(SIGTERM, SAMPLE_VENC_HandleSig);

    pthread_create(&threadid, NULL, rtsp_thread, NULL);

    switch (*argv[1])
    {
    case '0': /* H.264@1080p@30fps+H.265@1080p@30fps+H.264@D1@30fps */
        s32Ret = SAMPLE_VENC_1080P_CLASSIC();
        break;
    case '1': /* 1*1080p mjpeg encode + 1*1080p jpeg  */
        s32Ret = SAMPLE_VENC_1080P_MJPEG_JPEG();
        break;
    case '2': /* low delay */
        s32Ret = SAMPLE_VENC_LOW_DELAY();
        break;
    case '3': /* roibg framerate */
        s32Ret = SAMPLE_VENC_ROIBG_CLASSIC();
        break;
    case '4': /* Thumbnail of 1*1080p jpeg  */
        s32Ret = SAMPLE_VENC_1080P_JPEG_Thumb();
        break;
#ifndef hi3518ev201
    case '5': /* H.264 Svc-t */
        s32Ret = SAMPLE_VENC_SVC_H264();
        break;
#endif
    default:
        printf("the index is invaild!\n");
        SAMPLE_VENC_Usage(argv[0]);
        return HI_FAILURE;
    }

    if (HI_SUCCESS == s32Ret)
        printf("program exit normally!\n");
    else
        printf("program exit abnormally!\n");
    exit(s32Ret);
}