#include "kernel_fsm.h"
#include "kapi.h"
#include "../uapi/uapi.h"

void PrintHdcpFsm(AvPort *Port, uint8 OldState)
{
    if(*Port->content.is_HdcpFsm != OldState)
    {
        switch(*Port->content.is_HdcpFsm)
        {
           case AvFsmHdcpDefault:
            AvUapiOutputDebugFsm("Port%d: HdcpDefault",Port->index);
            break;

           case AvFsmHdcpDisable:
            AvUapiOutputDebugFsm("Port%d: HdcpDisable",Port->index);
            break;

           case AvFsmHdcpFail:
            AvUapiOutputDebugFsm("Port%d: HdcpFail",Port->index);
            break;

           case AvFsmHdcp2p2State:
            AvUapiOutputDebugFsm("Port%d: Hdcp2p2State",Port->index);
            break;

           case AvFsmHdcpReAuthentication:
            AvUapiOutputDebugFsm("Port%d: HdcpReAuthentication",Port->index);
            break;

           case AvFsmHdcpReadBksv:
            AvUapiOutputDebugFsm("Port%d: HdcpReadBksv",Port->index);
            break;

           case AvFsmHdcpReceiverMode:
            AvUapiOutputDebugFsm("Port%d: HdcpReceiverMode",Port->index);
            break;

           case AvFsmHdcpRepeaterMode:
            AvUapiOutputDebugFsm("Port%d: HdcpRepeaterMode",Port->index);
            break;

           case AvFsmHdcpRepeaterReset:
            AvUapiOutputDebugFsm("Port%d: HdcpRepeaterReset",Port->index);
            break;

           case AvFsmHdcpRequestSinkHdcp:
            AvUapiOutputDebugFsm("Port%d: HdcpRequestSinkHdcp",Port->index);
            break;

           case AvFsmHdcpStartAuthentication:
            AvUapiOutputDebugFsm("Port%d: HdcpStartAuthentication",Port->index);
            break;

           case AvFsmHdcpUpstreamConnected:
            AvUapiOutputDebugFsm("Port%d: HdcpUpstreamConnected",Port->index);
            break;
        }
    }
}

void PrintPlugTxFsm(AvPort *Port, uint8 OldState)
{
    if(*Port->content.is_PlugTxFsm != OldState)
    {
        switch (*Port->content.is_PlugTxFsm)
        {
          case AvFsmPlugTxEdidError:
              AvUapiOutputDebugFsm("Port%d: PlugTxEdidError", Port->index);
            break;
          case AvFsmPlugTxDefault:
              AvUapiOutputDebugFsm("Port%d: PlugTxDefault", Port->index);
            break;
          case AvFsmPlugTxDefaultEdid:
              AvUapiOutputDebugFsm("Port%d: PlugTxDefaultEdid", Port->index);
            break;
          case AvFsmPlugTxEdid:
              AvUapiOutputDebugFsm("Port%d: PlugTxEdid", Port->index);
            break;
          case AvFsmPlugTxEdidManage:
              AvUapiOutputDebugFsm("Port%d: PlugTxEdidManage", Port->index);
            break;
          case AvFsmPlugTxEnableTxCore:
              AvUapiOutputDebugFsm("Port%d: PlugTxEnableTxCore", Port->index);
            break;
          case AvFsmPlugTxHdcp:
              AvUapiOutputDebugFsm("Port%d: PlugTxHdcp", Port->index);
            break;
          case AvFsmPlugTxHpdAntiDither:
              AvUapiOutputDebugFsm("Port%d: PlugTxHpdAntiDither", Port->index);
            break;
          case AvFsmPlugTxReset:
              AvUapiOutputDebugFsm("Port%d: PlugTxReset", Port->index);
            break;
          case AvFsmPlugTxStable:
              AvUapiOutputDebugFsm("Port%d: PlugTxStable", Port->index);
            break;
          case AvFsmPlugTxTransmitVideo:
              AvUapiOutputDebugFsm("Port%d: PlugTxTransmitVideo", Port->index);
            break;
          case AvFsmPlugTxVideoUnlocked:
              AvUapiOutputDebugFsm("Port%d: PlugTxVideoUnlocked", Port->index);
            break;
        }
    }
}

void PrintPlugRxFsm(AvPort *Port, uint8 OldState)
{
    if(*Port->content.is_PlugRxFsm != OldState)
    {
        switch (*Port->content.is_PlugRxFsm)
        {
          case AvFsmPlugRxDefault:
              AvUapiOutputDebugFsm("Port%d: PlugRxDefault", Port->index);
            break;
          case AvFsmPlugRxDetect:
              AvUapiOutputDebugFsm("Port%d: PlugRxDetect", Port->index);
            break;
          case AvFsmPlugRxInfoUpdate:
              AvUapiOutputDebugFsm("Port%d: PlugRxInfoUpdate", Port->index);
            break;
          case AvFsmPlugRxInputLock:
              AvHandleEvent(Port, AvEventPortUpStreamConnected, NULL, NULL);
              AvUapiOutputDebugFsm("Port%d: PlugRxInputLock", Port->index);
            break;
          case AvFsmPlugRxPlugged:
              AvUapiOutputDebugFsm("Port%d: PlugRxPlugged", Port->index);
            break;
          case AvFsmPlugRxPullDownHpd:
              if(Port->content.rx->Input5V == 0)
                  AvHandleEvent(Port, AvEventPortUpStreamDisconnected, NULL, NULL);
              AvUapiOutputDebugFsm("Port%d: PlugRxPullDownHpd", Port->index);
            break;
          case AvFsmPlugRxReadTiming:
              AvUapiOutputDebugFsm("Port%d: PlugRxReadTiming", Port->index);
            break;
          case AvFsmPlugRxReset:
              AvUapiOutputDebugFsm("Port%d: PlugRxReset", Port->index);
            break;
          case AvFsmPlugRxStable:
              AvUapiOutputDebugFsm("Port%d: PlugRxStable", Port->index);
            break;
        }
    }
}

void PrintReceiverFsm(AvPort *Port, uint8 OldState)
{
    if(*Port->content.is_ReceiverFsm != OldState)
    {
        switch (*Port->content.is_ReceiverFsm)
        {
          case AvFsmRxDefault:
            AvUapiOutputDebugFsm("Port%d: RxDefault", Port->index);
            break;
          case AvFsmRxDetect:
            AvUapiOutputDebugFsm("Port%d: RxDetect", Port->index);
            break;
          case AvFsmRxFreerun:
            AvUapiOutputDebugFsm("Port%d: RxFreerun", Port->index);
            break;
          case AvFsmRxReceiving:
            AvUapiOutputDebugFsm("Port%d: RxReceiving", Port->index);
            break;
          case AvFsmRxReset:
            AvUapiOutputDebugFsm("Port%d: RxReset", Port->index);
            break;
        }
    }
}

#if AvEnableCecFeature /* CEC Related */
void PrintCecFsm(AvPort *Port, uint8 OldState)
{
    if(*Port->content.is_CecFsm != OldState)
    {
        switch (*Port->content.is_CecFsm)
        {
          case AvFsmCecDefault:
            AvUapiOutputDebugFsm("Port%d: CecDefault", Port->index);
            break;
          case AvFsmCecIdle:
            AvUapiOutputDebugFsm("Port%d: CecIdle", Port->index);
            break;
          case AvFsmCecNotConnected:
            AvUapiOutputDebugFsm("Port%d: CecNotConnected", Port->index);
            break;
          case AvFsmCecReset:
            AvUapiOutputDebugFsm("Port%d: CecReset", Port->index);
            break;
          case AvFsmCecTxLogAddr:
            AvUapiOutputDebugFsm("Port%d: CecTxLogAddr", Port->index);
            break;
          case AvFsmCecAudioControl:
            AvUapiOutputDebugFsm("Port%d: CecAudioControl", Port->index);
            break;
          case AvFsmCecAudioFormat:
            AvUapiOutputDebugFsm("Port%d: CecAudioFormat", Port->index);
            break;
          case AvFsmCecAudioManage:
            AvUapiOutputDebugFsm("Port%d: CecAudioManage", Port->index);
            break;
          case AvFsmCecCmdARC:
            AvUapiOutputDebugFsm("Port%d: CecCmdARC", Port->index);
            break;
          case AvFsmCecCmdAudioARC:
            AvUapiOutputDebugFsm("Port%d: CecCmdAudioARC", Port->index);
            break;
          case AvFsmCecCmdSystemAudioModetoAll:
            AvUapiOutputDebugFsm("Port%d: CecCmdSystemAudioModetoAll", Port->index);
            break;
          case AvFsmCecFunctionalDefault:
            AvUapiOutputDebugFsm("Port%d: CecFunctionalDefault", Port->index);
            break;
          case AvFsmCecCmdActiveSource:
            AvUapiOutputDebugFsm("Port%d: CecCmdActiveSource", Port->index);
            break;
        }
    }
}
#endif /* CEC Related */
