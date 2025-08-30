/**
 * @file uapi.h
 *
 * @brief low level ports related universal api functions
 */

#ifndef __uapi_h
#define __uapi_h

#include <linux/kernel.h>

#include "../av_config.h"
#include "hal.h"

#if GSV1K
#include "gsv1k.h"
#endif
#if GSV2K11
#include "gsv2k11.h"
#endif


/* define uapi as internal identification key word */
#define uapi

/* macro to delcare and implement uapi functions for each device */
#define DeclareUapi(device, uapiName)   uapiName
#define ImplementUapi(device, uapiName) uapiName
#define CallUapi(device, uapiName)      uapiName

extern  AvFpI2cRead        AvHookI2cRead;
extern  AvFpI2cWrite       AvHookI2cWrite;
extern  AvFpUartSendByte   AvHookUartTxByte;
extern  AvFpUartGetByte    AvHookUartRxByte;
extern  AvFpGetMilliSecond AvHookGetMilliSecond;
extern  AvFpGetKey         AvHookGetKey;
extern  AvFpGetIrda        AvHookGetIrda;
/* Video Interrupt */
typedef struct
{
    uint8 AvMute;
    uint8 HdmiModeChg;
    uint8 DeRegenLck;
    uint8 VsyncLck;
    uint8 Vid3dDet;
    uint8 NewTmds;
    uint8 BadTmdsClk;
    uint8 DeepClrChg;
    uint8 PktErr;
    uint8 AvIfValid;
    uint8 SpdValid;
    uint8 HdrValid;
    uint8 MsValid;
    uint8 VsValid;
    uint8 Isrc1Valid;
    uint8 Isrc2Valid;
    uint8 GamutValid;
    uint8 GcValid;
    uint8 EmpValid;
}   VideoInterrupt;

/* Audio Interrupt */
typedef struct
{
    uint8 AudChanMode;
    uint8 InternMute;
    uint8 CsDataValid;
    uint8 NChange;
    uint8 CtsThresh;
    uint8 AudFifoOv;
    uint8 AudFifoUn;
    uint8 AudFifoNrOv;
    uint8 AudioPktErr;
    uint8 AudModeChg;
    uint8 AudFifoNrUn;
    uint8 AudFlatLine;
    uint8 AudSampChg;
    uint8 AudPrtyErr;
    uint8 AudIfValid;
    uint8 AcpValid;
}   AudioInterrupt;

/* Hdcp Interrupt */
typedef struct
{
    uint8 AksvUpdate;
    uint8 Encrypted;
}   HdcpInterrupt;


/* exported functions */
#ifdef __cplusplus
extern "C" {
#endif

uapi AvRet AvUapiInit(void);
uapi AvRet AvUapiHookBspFunctions(pin AvFpI2cRead i2cRd, pin AvFpI2cWrite i2cWr, pin AvFpUartSendByte uartTxB, pin AvFpUartGetByte uartRxB, pin AvFpGetMilliSecond getMs, pin AvFpGetKey getKey,pin AvFpGetIrda getIrda);
uapi AvRet AvUapiOuputDbgMsg(pin schar *FormattedString, ...); /* do not call this api, call macro "AvUapiOutputDebugMessage()" */

#if  AvEnableDebugMessage
//#define AvUapiOutputDebugMessage AvUapiOuputDbgMsg
#define AvUapiOutputDebugMessage printk
#else
#define AvUapiOutputDebugMessage(...)
#endif
#if AvEnableDebugFsm
//#define AvUapiOutputDebugFsm AvUapiOuputDbgMsg
#define AvUapiOutputDebugFsm printk
#else
#define AvUapiOutputDebugFsm(...)
#endif

uapi AvRet AvUapiAllocateMemory(pin uint32 bytes, pout uint64 *bufferAddress);

uapi AvRet AvUapiConnectPort(pin AvPort *FromPort, pin AvPort *ToPort, AvConnectType type);
uapi AvRet AvUapiDisconnectPort(pin AvPort *Port);

uapi AvRet AvUapiInitDevice(pio AvDevice *device);
uapi AvRet AvUapiResetPort(pio AvPort *port);
uapi AvRet AvUapiEnablePort(pio AvPort *port);

uapi AvRet AvUapiRxPortInit(pio AvPort *port);
uapi AvRet AvUapiRxGetStatus(pio AvPort *port);
uapi AvRet AvUapiRxEnableFreeRun(pio AvPort *port, bool enable);
uapi AvRet AvUapiRxGetHdcpStatus(pio AvPort *port, HdcpInterrupt* Intpt);
uapi AvRet AvUapiRxGetVideoPacketStatus(pio AvPort *port, VideoInterrupt* Intpt);
uapi AvRet AvUapiRxGetAudioPacketStatus(pio AvPort *port, AudioInterrupt* Intpt);
uapi AvRet AvUapiRxGet5VStatus(pio AvPort *port);
uapi AvRet AvUapiRxSetHpdDown(pio AvPort *port);
uapi AvRet AvUapiRxSetHpdUp(pio AvPort *port);
uapi AvRet AvUapiRxClearFlags(pio AvPort *port);
uapi AvRet AvUapiRxGetPacketContent(pin AvPort *port, PacketType Pkt, uint8 *Content);
uapi AvRet AvUapiRxGetHdmiAcrInfo(pio AvPort *port);
uapi AvRet AvUapiRxClearAudioInterrupt(pio AvPort *port, AudioInterrupt* Intpt);
uapi AvRet AvUapiRxClearVideoInterrupt(pio AvPort *port, VideoInterrupt* Intpt);
uapi AvRet AvUapiRxClearHdcpInterrupt(pio AvPort *port,  HdcpInterrupt* Intpt);

uapi AvRet AvUapiTxPortInit(pio AvPort *port);
uapi AvRet AvUapiTxEnableCore(pin AvPort *port);
uapi AvRet AvUapiTxDisableCore(pin AvPort *port);
uapi AvRet AvUapiTxGetStatus(pio AvPort *port);
uapi AvRet AvUapiTxMuteTmds(pio AvPort *port, bool mute);
uapi AvRet AvUapiTxGetSinkHdcpCapability(pio AvPort *port);
uapi AvRet AvUapiTxSetAudioPackets(pin AvPort *port);
uapi AvRet AvUapiTxVideoManage(pio AvPort *port);
uapi AvRet AvUapiTxAudioManage(pio AvPort *port);
uapi AvRet AvUapiTxSetPacketContent(pin AvPort *port, PacketType Pkt, uint8 *Content, uint8 PktEn);
uapi AvRet AvUapiTxEnableInfoFrames(pin AvPort* port, uint16 InfoFrames, uint8 Enable);
uapi AvRet AvUapiTxGetHdcpStatus(pin AvPort *port);
uapi AvRet AvUapiTxGetBksvReady(pin AvPort *port);

uapi AvRet AvUapiRxGetAvMute(pio AvPort *port);
uapi AvRet AvUapiTxSetAvMute(pio AvPort *port);
uapi AvRet AvUapiTxSetBlackMute(pio AvPort *port);
uapi AvRet AvUapiRxGetHdmiModeSupport(pio AvPort *port);
uapi AvRet AvUapiTxSetHdmiModeSupport(pio AvPort *port);
uapi AvRet AvUapiTxSetFeatureSupport(pio AvPort *port);
uapi AvRet AvUapiRxGetVideoLock(pio AvPort *port);
uapi AvRet AvUapiRxGetVideoTiming(pio AvPort *port);
uapi AvRet AvUapiTxSetVideoTiming(pio AvPort *port);
uapi AvRet AvUapiRxGetHdmiDeepColor(pio AvPort *port);
uapi AvRet AvUapiTxSetHdmiDeepColor(pio AvPort *port);
uapi AvRet AvUapiRxGetAudioInternalMute(pio AvPort *port);
uapi AvRet AvUapiRxSetAudioInternalMute(pio AvPort *port);
uapi AvRet AvUapiTxSetAudNValue(pio AvPort *port);
uapi AvRet AvUapiRxGetPacketType(pio AvPort *port);
uapi AvRet AvUapiTxReadBksv(pio AvPort *port, uint8 *Value, uint8 Count);
uapi AvRet AvUapiRxAddBksv(pio AvPort *port, uint8 *Value, uint8 Position);
uapi AvRet AvUapiTxGetBksvTotal(pio AvPort *port, uint8 *Value);
uapi AvRet AvUapiTxEncryptSink(pin AvPort *port);
uapi AvRet AvUapiTxDecryptSink(pin AvPort *port);
uapi AvRet AvUapiRxSetHdcpEnable(pin AvPort *port);
uapi AvRet AvUapiRxSetBksvListReady(pin AvPort *port);
uapi AvRet AvUapiRxSetHdcpMode(pin AvPort *port);
uapi AvRet AvUapiHdcp2p2Mode(pin AvPort *port);
uapi AvRet AvUapiTxClearBksvReady(pin AvPort *port);
uapi AvRet AvUapiTxClearRxidReady(pin AvPort *port);
uapi AvRet AvUapiTxClearHdcpError(pin AvPort *port);
uapi AvRet AvUapiRxCheckBksvExisted(pin AvPort *port, uint8 *Bksv);
uapi AvRet AvUapiCecSendMessage(pin AvPort *port);
uapi AvRet AvUapiGetNackCount(pin AvPort *port);
uapi AvRet AvUapiTxCecInit(pin AvPort *port);
uapi AvRet AvUapiCecRxGetStatus(pin AvPort *port);
uapi AvRet AvUapiCecTxGetStatus(pin AvPort *port);
uapi AvRet AvUapiTxCecSetPhysicalAddr(AvPort *port);
uapi AvRet AvUapiTxCecSetLogicalAddr(AvPort *port);
uapi AvRet AvUapiRxReadEdid(pio AvPort *port, uint8 *Value, uint16 Count);
uapi AvRet AvUapiRxWriteEdid(pio AvPort *port, uint8 *Value, uint16 Count);
uapi AvRet AvUapiRxSetSpa(pio AvPort *port, uint8 SpaLocation, uint8 *SpaValue);
uapi AvRet AvUapiTxReadEdid(pio AvPort *port, uint8 *Value, uint16 Count);
uapi AvRet AvUapiRxEnableInternalEdid(pio AvPort *port);
uapi AvRet AvUapiTxArcEnable(pio AvPort *port, uint8 value);
uapi AvRet AvUapiRxHdcp2p2Manage(pin AvPort *port);
uapi AvRet AvUapiTxHdcp2p2Manage(pin AvPort *port);
uapi AvRet AvUapiCheckLogicVideoTx(pio AvPort *port);
uapi AvRet AvUapiCheckLogicVideoRx(pio AvPort *port);
uapi AvRet AvUapiCheckLogicAudioTx(pio AvPort *port);
uapi AvRet AvUapiCheckLogicAudioRx(pio AvPort *port);
uapi AvRet AvUapiCheckVideoScaler(pio AvPort *port);
uapi AvRet AvUapiCheckVideoColor(pio AvPort *port);
uapi AvRet AvUapiCheckVideoGen(pio AvPort *port);
uapi AvRet AvUapiCheckAudioGen(pio AvPort *port);
uapi AvRet AvUapiCheckClockGen(pio AvPort *port);
uapi AvRet AvUapiRxEdidStat(pin AvPort *port);
uapi AvRet AvUapiTxDetectMode(pin AvPort *port);
uapi AvRet AvUapiRxVideoManage(pin AvPort *port, VideoInterrupt* Intpt);
uapi AvRet AvUapiRxAudioManage(pin AvPort *port, AudioInterrupt* Intpt);
uapi AvRet AvUapiRxReadStdi(pin AvPort *port);
uapi AvRet AvUapiRxReadInfo(pin AvPort *port);

#define Gsv2k11AvUapiTxClearRxidReady           AvUapiTxClearRxidReady
#define Gsv2k11AvUapiTxDecryptSink              AvUapiTxDecryptSink
#define Gsv2k11AvUapiTxDisableCore              AvUapiTxDisableCore
#define Gsv2k11AvUapiTxEnableCore               AvUapiTxEnableCore
#define Gsv2k11AvUapiRxGetVideoLock             AvUapiRxGetVideoLock
#define Gsv2k11AvUapiRxGetAudioInternalMute     AvUapiRxGetAudioInternalMute
#define Gsv2k11AvUapiDisconnectPort             AvUapiDisconnectPort
#define Gsv2k11AvUapiTxEnableInfoFrames         AvUapiTxEnableInfoFrames
#define Gsv2k11AvUapiTxSetAudNValue             AvUapiTxSetAudNValue
#define Gsv2k11AvUapiRxGetPacketType            AvUapiRxGetPacketType
#define Gsv2k11AvUapiRxGetPacketContent         AvUapiRxGetPacketContent
#define Gsv2k11AvUapiTxGetSinkHdcpCapability    AvUapiTxGetSinkHdcpCapability
#define Gsv2k11AvUapiRxSetAudioInternalMute     AvUapiRxSetAudioInternalMute
#define Gsv2k11AvUapiTxCecInit                  AvUapiTxCecInit
#define Gsv2k11AvUapiTxCecSetLogicalAddr        AvUapiTxCecSetLogicalAddr

#define MathAbs(a,b)     ((a)>=(b) ? (a-b) : (b-a))
#define MathMin(a,b)     ((a)<=(b) ? (a) : (b))
#define MathMax(a,b)     ((a)>=(b) ? (a) : (b))

/*========================================
 * AVI info frame macros
 *=======================================*/
#define SET_AVIF_BYTE1(Y,A,B,S)         ((Y<<5) | (A<<4) | (B<<2) | S)
#define SET_AVIF_BYTE2(C,M,R)           ((C<<6) | (M<<4) | R)
#define SET_AVIF_BYTE3(ITC,EC,Q,SC)     ((ITC<<7)| (EC<<4) | (Q<<2) | SC)
#define SET_AVIF_BYTE4(VIC)             (VIC)
#define SET_AVIF_BYTE5(PR)              (PR)

#define SET_AVIF_VERSION(Ptr, Ver)      (Ptr[1]=Ver)
#define SET_AVIF_LEN(Ptr, Len)          (Ptr[2]=Len)
#define SET_AVIF_Y(Ptr,Y)               (Ptr[4]=(Ptr[4]&(~0xE0))|(Y<<5))
#define SET_AVIF_PR(Ptr,PR)             (Ptr[8]=(Ptr[8]&(~0x0f))|PR)
#define SET_AVIF_VIC(Ptr,VIC)           (Ptr[7]=VIC)
#define SET_AVIF_C(Ptr,C)               (Ptr[5]=(Ptr[5]&(~0xc0))|(C<<6))
#define SET_AVIF_M(Ptr,M)               (Ptr[5]=(Ptr[5]&(~0x30))|(M<<4))
#define SET_AVIF_R(Ptr,R)               (Ptr[5]=(Ptr[5]&(~0x0f))|R)
#define SET_AVIF_EC(Ptr,EC)             (Ptr[6]=(Ptr[6]&(~0x70))|(EC<<4))
#define SET_AVIF_Q(Ptr,Q)               (Ptr[6]=(Ptr[6]&(~0x0c))|(Q<<2))
#define SET_AVIF_YQ(Ptr,YQ)             (Ptr[8]=(Ptr[8]&(~0xC0))|(YQ<<6))
#define SET_AVIF_A(Ptr,A)               (Ptr[4]=(Ptr[4]&(~0x10))|(A<<4))
#define SET_AVIF_B(Ptr,B)               (Ptr[4]=(Ptr[4]&(~0x0c))|(B<<2))
#define SET_AVIF_SC(Ptr,SC)             (Ptr[4]=(Ptr[4]&(~0x03))|SC)

#define GET_AVIF_VERSION(Ptr)           ( Ptr[1] & 0xff)
#define GET_AVIF_LEN(Ptr)               ( Ptr[2] & 0x1f)
#define GET_AVIF_Y(Ptr)                 ((Ptr[4] & 0xE0) >> 5)
#define GET_AVIF_A(Ptr)                 ((Ptr[4] & 0x10) >> 4)
#define GET_AVIF_B(Ptr)                 ((Ptr[4] & 0x0c) >> 2)
#define GET_AVIF_S(Ptr)                 ( Ptr[4] & 0x03)
#define GET_AVIF_C(Ptr)                 ((Ptr[5] & 0xc0) >> 6)
#define GET_AVIF_M(Ptr)                 ((Ptr[5] & 0x30) >> 4)
#define GET_AVIF_R(Ptr)                 ( Ptr[5] & 0x0f)
#define GET_AVIF_ITC(Ptr)               ((Ptr[6] & 0x80) >> 7)
#define GET_AVIF_EC(Ptr)                ((Ptr[6] & 0x70) >> 4)
#define GET_AVIF_Q(Ptr)                 ((Ptr[6] & 0x0c) >> 2)
#define GET_AVIF_SC(Ptr)                ( Ptr[6] & 0x03)
#define GET_AVIF_VIC(Ptr)               ( Ptr[7] & 0xff)
#define GET_AVIF_PR(Ptr)                ( Ptr[8] & 0x0f)
#define GET_AVIF_YQ(Ptr)                ((Ptr[8] & 0xC0) >> 6)
/*===========================================================
 * Vendor Specific info frame macros
 * 3D and Extended Resolution (4Kx2K) do not happen together
 *===========================================================*/
#define GET_VSIF_HVFRMT(Ptr)            ((Ptr[7] & 0xE0) >> 5)
#define GET_VSIF_3D_STRUCT(Ptr)         ((Ptr[8] & 0xF0) >> 4)
#define GET_VSIF_HDMI_VIC(Ptr)          ( Ptr[8] )
#define GET_VSIF_3D_STATUS(Ptr)         ((Ptr[7] & 0x40) >> 6)

#define GET_VSIF_LEN(Ptr)               ( Ptr[2] & 0x1F)
#define GET_3D_STATUS(Ptr)              ((Ptr[7] & 0x40) >> 6)
#define GET_3D_FORMAT(Ptr)              ((Ptr[8] & 0xF0) >> 4)
#define GET_4K_STATUS(Ptr)              ((Ptr[7] & 0x20) >> 5)
#define GET_4K_FORMAT(Ptr)              ( Ptr[8] )

#define SET_VSIF_LEN(Ptr, Len)          (Ptr[2] = Len)
#define SET_VSIF_CHECKSUM(Ptr, Chk)     (Ptr[3] = Chk)
#define SET_3D_STATUS(Ptr, Sts)         (Ptr[7] = (Ptr[7]&(~0x40))|(Sts<<6))
#define SET_3D_FORMAT(Ptr, Fmt)         (Ptr[8] = (Fmt & 0x0F) << 4)
#define SET_4K_STATUS(Ptr, Sts)         (Ptr[7] = (Ptr[7]&(~0x20))|(Sts<<5))
#define SET_4K_FORMAT(Ptr, Vic)         (Ptr[8] = Vic)
/*========================================
 * Audio info frame macros
 *=======================================*/
#define SET_AUDIF_CC(Ptr,cc)            (Ptr[4]=(Ptr[4]&(~0x07))|((cc)&7))
#define SET_AUDIF_CA(Ptr,ca)            (Ptr[7] = ca)

#define GET_AUDIF_VER(Ptr)              (Ptr[1])
#define GET_AUDIF_CC(Ptr)               (Ptr[4]&0x7)
#define GET_AUDIF_CT(Ptr)               ((Ptr[4]>>4)&0xf)
#define GET_AUDIF_SS(Ptr)               (Ptr[5]&0x3)
#define GET_AUDIF_SF(Ptr)               ((Ptr[5]>>2)&0x7)
#define GET_AUDIF_CT_X(Ptr)             (Ptr[6])
#define GET_AUDIF_CA(Ptr)               (Ptr[7])
#define GET_AUDIF_LSV(Ptr)              ((Ptr[8]>>3)&0xf)
#define GET_AUDIF_DM_INH(Ptr)           ((Ptr[8]>>7)&0x1)

/*========================================
 * Channel status macros
 *=======================================*/
#define GET_CHST_PROFF_APP(Ptr)         (Ptr[0]&3)
    /*#define GET_CHST_AUD_SAMP_TYPE(Ptr)     ((Ptr[0]>>1) & 0x01) */
#define GET_CHST_COPYRIGHT(Ptr)         ((Ptr[0]>>2) & 0x01)
#define GET_CHST_EMPHASIS(Ptr)          ((Ptr[0]>>3) & 0x07)
#define GET_CHST_MODE(Ptr)              ((Ptr[0]>>6) & 0x03)
#define GET_CHST_CATG_CODE(Ptr)         (Ptr[1])
#define GET_CHST_SRC_NUM(Ptr)           (Ptr[2] & 0x0f)
#define GET_CHST_CH_NUM(Ptr)            ((Ptr[2]>>4) & 0x0f)
#define GET_CHST_SAMP_FREQ(Ptr)         (Ptr[3] & 0x0f)
#define GET_CHST_CLK_ACCUR(Ptr)         ((Ptr[3]>>4) & 0x03)
#define GET_CHST_RESERV0(Ptr)           ((Ptr[3]>>6) & 0x03)
#define GET_CHST_MAX_WORD_LEN(Ptr)      (Ptr[4] & 0x01)
#define GET_CHST_WORD_LEN(Ptr)          ((Ptr[4]>>1) & 0x07)
#define GET_CHST_4BIT_WORD_LEN(Ptr)     (Ptr[4] & 0x0f)

#define GET_N_VALUE(Ptr)                ((((UINT32)(Ptr[0]))<<16) | (((UINT32)(Ptr[1]))<< 8) | ((UINT32)(Ptr[1])))


#ifdef __cplusplus
}
#endif
#endif
