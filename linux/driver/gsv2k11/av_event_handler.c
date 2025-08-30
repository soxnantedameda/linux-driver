/**
 * @file av_event_handler.c
 *
 * @brief av event handler to handle the events for customers use
 */
#include "av_event_handler.h"

uint8  EdidHdmi2p0     = 1;
uint8  LogicOutputSel  = 1;

#if AvEnableCecFeature /* CEC Related */
extern uchar  DevicePowerStatus;
extern char   DeviceName[20];
extern uchar  AudioStatus;
extern CEC_AUDIO_STATUS Cec_Tx_Audio_Status;
uint16 AvCecFindRequiredInput (AvPort *port, uint16 SrcPhys);
#endif

/* Edid Related Declare Start */
extern AvEdidReg DevEdidReg;
extern uint16    AvEdidSupportFreq;
extern uint8     AvEdidVesaParamForce;
extern uint8     AvEdidCeaParamForce;
extern uint16    AvEdidCeabParamForce;
extern uint8     AvEdidVesaParamRemove;
extern uint8     AvEdidCeaParamRemove;
extern uint16    AvEdidCeabParamRemove;
uint8 InEdid[256];
uint8 SinkEdid[256];
uint8 OutEdid[256];
AvRet AvEdidPortManage(AvPort *RxPort);
AvRet AvEdidPortAnalysis(AvPort *port);
/* Edid Related Declare End */

/**
 * @brief  process audio/video events
 * @return none
 */
AvRet AvHandleEvent(AvPort *port, AvEvent event, uint8 *wparam, uint8 *pparam)
{
    AvRet ret = AvOk;
    AvPort *TempPort;
#if AvEnableCecFeature /* CEC Related */
    uint8 *RxContent = port->content.cec->RxContent;
    uint8 *TxContent = port->content.cec->TxContent;
    int i;
    uint8 MsgLenValue = 2;
    uint8 *Msg_Len = &MsgLenValue;
    uint16 ActiveSource = 0;
    uint16 InpIndex;
    uint16 Recv_Phy_Addr;
    uint8  DeviceType;
    uint8  Len;
    uint8  Spa[2];
#endif

    switch(event)
    {
        case AvEventRxSetHdcpStyle:
            /* Rx Hdcp Selection:
            AV_HDCP_RX_NOT_SUPPORT = 0,
            AV_HDCP_RX_AUTO        = 1,
            AV_HDCP_RX_FOLLOW_SINK = 2,
            AV_HDCP_RX_1P4_ONLY    = 3,
            AV_HDCP_RX_2P2_ONLY    = 4,
             */
            port->content.hdcp->HdcpNeeded = AV_HDCP_RX_AUTO;
            break;

        case AvEventTxSetHdcpStyle:
            /* Tx Hdcp Selection:
            AV_HDCP_TX_ILLEGAL_NO_HDCP = 0,
            AV_HDCP_TX_AUTO            = 1,
            AV_HDCP_TX_AUTO_FAIL_OUT   = 2,
            AV_HDCP_TX_1P4_ONLY        = 3,
            AV_HDCP_TX_1P4_FAIL_OUT    = 4,
            AV_HDCP_TX_2P2_ONLY        = 5,
            AV_HDCP_TX_2P2_FAIL_OUT    = 6
               Tx Dvi Selection:
            AV_HDCP_TX_DVI_STRICT      = 0,
            AV_HDCP_TX_DVI_LOOSE       = 1
            */
            port->content.hdcptx->HdmiStyle = AV_HDCP_TX_AUTO;
            port->content.hdcptx->DviStyle  = AV_HDCP_TX_DVI_LOOSE;
            break;

        case AvEventPortAudioInfoChanged:
            break;

        case AvEventPortVideoInfoChanged:
            break;

        case AvEventUpStreamConnectNewDownStream:
#if AvNoLinkageMode
#else
            if(KfunFindVideoRxFront(port, &TempPort) == AvOk)
                TempPort->content.rx->EdidStatus = AV_EDID_RESEND;
#endif
            break;

        case AvEventPortDownStreamDisconnected:
#if AvNoLinkageMode
#else
            if(KfunFindVideoRxFront(port, &TempPort) == AvOk)
                TempPort->content.rx->EdidStatus = AV_EDID_SINKLOST;
#endif
            TxOutLedOut(port->index-4, 0);
            break;

        case AvEventPortDownStreamConnected:
            TxOutLedOut(port->index-4, 1);
            break;

        case AvEventPortDownStreamSending:
            if(KfunFindVideoRxFront(port, &TempPort) == AvOk)
            {
                if(TempPort->type == HdmiRx)
                {
                    /* HDMI Mode Input Judge */
                    if(TempPort->content.rx->IsInputStable == 1)
                    {
                        if((port->content.tx->IgnoreEdidError == 1) || /* sink is default as HDMI */
                           (port->content.tx->PhyAddr != 0x0000))      /* sink is HDMI */
                            port->content.tx->HdmiMode = TempPort->content.rx->HdmiMode;
                        else
                            port->content.tx->HdmiMode = 0; /* sink is DVI as only option */
                    }
                    else
                        port->content.tx->HdmiMode = 1; /* sink is default to be HDMI */
                    KfunTxSetHdmiModeSupport(port);
                }
            }
            break;

        case AvEventPortUpStreamDisconnected:
            RxInLedOut(0);
            break;

        case AvEventPortUpStreamConnected:
            RxInLedOut(1);
            break;

        case AvEventRxPrepareEdid:
            if(port->content.rx->EdidStatus != AV_EDID_UPDATED)
            {
                AvEdidPortManage(port);
                port->content.rx->EdidStatus = AV_EDID_UPDATED;
            }
            break;

        case AvEventPortEdidReady:
            AvEdidPortAnalysis(port);
            /* Resend Edid */
            if(KfunFindVideoRxFront(port, &TempPort) == AvOk)
            {
                if(TempPort->type == HdmiRx)
                    TempPort->content.rx->EdidStatus = AV_EDID_RESEND;
            }
            break;

        case AvEventPortEdidReadFail:
            break;

        case AvEventTxDefaultEdid:
            port->content.tx->EdidSupportFeature = 0xffffffff;
            break;

        case AvEventPortUpStreamEncrypted:
            /* Upstream input is encrypted */
            break;

        case AvEventPortUpStreamDecrypted:
            /* Upstream input is not encrypted */
            break;

#if AvEnableCecFeature /* CEC Related */

        case AvEventCecRxMessage:
            break;

        case AvEventCecTxDone:
            break;

        case AvEventCecTxTimeout:
            AvHandleEvent(port, AvEventCecTxError, 0, NULL);
            break;

        case AvEventCecTxArbLost:
            AvHandleEvent(port, AvEventCecTxError, 0, NULL);
            break;

            /* when kernel layer finds a valid LogAddr, it tells */
        case AvEventCecLogAddrAlloc:
            /* user can modify the LogAddr by themselves, in this example,
               LogAddr will be allocated to the 1st none-zero value, all the
               unallocated address will be poped here */
            if(*wparam == AvCecLogicAddress)
            {
                port->content.cec->LogAddr = *wparam;
                AvKapiCecSetLogicalAddr(port);
                AvKapiOutputDebugMessage("CEC Port %d: Set Logical Address = %d",
                                         port->index, port->content.cec->LogAddr);
                /* Manual Stop Logic Address Search */
                port->content.cec->AddrIndex = 15;
            }
            break;

        case AvEventCecTxError:
            /* to be inserted */
            break;

        case AvEventCecRxMessageRespond:
            ret = AvOk;
            break;

        case AvEventCecArcManage:
            if(port->content.cec->TxSendFlag == AV_CEC_TX_SEND_SUCCESS)
            {
                if(port->content.cec->ARCTryCount < 5)
                {
                    port->content.cec->ARCTryCount = port->content.cec->ARCTryCount + 1;
                    if(port->content.cec->EnableARC == AV_CEC_ARC_TO_INITIATE)
                        AvHandleEvent(port, AvEventCecSendInitiateARC, NULL, NULL);
                    else if(port->content.cec->EnableARC == AV_CEC_ARC_TO_TERMINATE)
                        AvHandleEvent(port, AvEventCecSendTerminateARC, NULL, NULL);
                }
            }
            break;
#endif

#if AvCecDataSendingOutEvent

        case AvEventCecSendRoutingChange:
            /* 0x80, init of 2.2 routing control */
            /* pparam = NewPort, wparam = OldPort */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 6;
            AV_CEC_SET_HDR_BC(TxContent, port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_ROUTE_CHANGE);
            AV_CEC_SET_PHYS_ADDR1(TxContent, ((wparam[0]<<8)+wparam[1]));
            AV_CEC_SET_PHYS_ADDR2(TxContent, ((pparam[0]<<8)+pparam[1]));
            AvKapiOutputDebugMessage("CEC: Sending routing change. From Inp=%04x to Inp=%04x",
                                     ((wparam[0]<<8)+wparam[1]), ((pparam[0]<<8)+pparam[1]));
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendRoutingInformation:
            /* 0x81, response for 2.2 routing control */
            /* after EDID allocation of upper level chain, enable this feature */
            Spa[0] = (port->content.tx->PhyAddr)>>8;
            Spa[1] = (port->content.tx->PhyAddr)&0xff;
            KfunGenerateSourceSpa(port, Spa, (port->content.cec->InputCount>>4));
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 4;
            AV_CEC_SET_HDR_BC(TxContent, port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_ROUTE_INFO);
            AV_CEC_SET_PHYS_ADDR1(TxContent, ((Spa[0]<<8)+Spa[1]));
            AvKapiOutputDebugMessage("CEC: Sending routing information. Inp=%04x", ((Spa[0]<<8)+Spa[1]));
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendReportPhysAddress:
            /* 0x84, report physical address */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen=5;
            switch(port->content.cec->LogAddr)
            {
                case AV_CEC_PLAYBACK1_LOG_ADDRESS:
                case AV_CEC_PLAYBACK2_LOG_ADDRESS:
                case AV_CEC_PLAYBACK3_LOG_ADDRESS:
                   DeviceType = AV_CEC_PLAYBACK1_LOG_ADDRESS;
                   break;
                case AV_CEC_RECORD1_LOG_ADDRESS:
                case AV_CEC_RECORD2_LOG_ADDRESS:
                   DeviceType = AV_CEC_RECORD1_LOG_ADDRESS;
                   break;
                case AV_CEC_TUNER1_LOG_ADDRESS:
                case AV_CEC_TUNER2_LOG_ADDRESS:
                case AV_CEC_TUNER3_LOG_ADDRESS:
                case AV_CEC_TUNER4_LOG_ADDRESS:
                   DeviceType = AV_CEC_TUNER1_LOG_ADDRESS;
                   break;
                default:
                   DeviceType = port->content.cec->LogAddr;
                   break;
            }
            AV_CEC_SET_HDR_BC(TxContent, port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_REPORT_PHYS_ADDR);
            AV_CEC_SET_PHYS_ADDR1(TxContent, port->content.tx->PhyAddr);
            TxContent[4] = DeviceType;
            AvKapiOutputDebugMessage("CEC: Sending Report phys address=%x", port->content.tx->PhyAddr);
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendActiveSource:
            /* 0x82, send active source, step 2 of 2.1 one touch play */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 4;
            AV_CEC_SET_HDR_BC(TxContent, port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_ACTIVE_SRC);
            AV_CEC_SET_PHYS_ADDR1(TxContent, port->content.tx->PhyAddr);
            AvKapiOutputDebugMessage("CEC: Sending active source=%04x", port->content.tx->PhyAddr);
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendInactiveSource:
            /* 0x82, send active source, step 2 of 2.1 one touch play */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 4;
            AV_CEC_SET_HDR_BC(TxContent, port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_INACTIVE_SOURCE);
            AV_CEC_SET_PHYS_ADDR1(TxContent, port->content.tx->PhyAddr);
            AvKapiOutputDebugMessage("CEC: Sending Inactive source=%04x", port->content.tx->PhyAddr);
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendFeatureAbort:
            /* 0x00, default response */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 4;
            port->content.cec->TxMsg = AV_CEC_MSG_FEATURE_ABORT;
            /* reverse logical addr */
            AV_CEC_SET_HDR_DA(TxContent, AV_CEC_SRC(RxContent), AV_CEC_DST(RxContent));
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_FEATURE_ABORT);
            TxContent[2] = AV_CEC_OPCODE(RxContent);
            TxContent[3] = AV_CEC_ABORT_REASON_REFUSED;
            AvKapiOutputDebugMessage("CEC: Sending feature abort");
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendPowerStatus:
            /* 0x90 ,14.2 report power status, response of give power status */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 3;
            port->content.cec->TxMsg = AV_CEC_MSG_REPORT_PWR_STATUS;
            AV_CEC_SET_HDR_DA(TxContent, AV_CEC_SRC(RxContent), AV_CEC_DST(RxContent));
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_REPORT_PWR_STATUS);
            TxContent[2] = DevicePowerStatus;
            AvKapiOutputDebugMessage("CEC: Sending report power status");
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendSetOsdName:
            /* 0x47, 10.2 response for give osd name */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 16;
            AV_CEC_SET_HDR_DA(TxContent, AV_CEC_SRC(RxContent), AV_CEC_DST(RxContent)); /* Directly addressed */
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_SET_OSD_NAME);
            for (Len=0; (Len<14) && DeviceName[Len]; Len++)
            {
                TxContent[Len+2] = DeviceName[Len];
            }
            AvKapiOutputDebugMessage("CEC: Sending Set OSD name");
            AvKapiCecSendMessage(port);
            break;
        case AvEventCecSendDeviceVendorID:
            /* 0x87, 9.2 vendor id broadcast */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 5;
            AV_CEC_SET_HDR_BC(TxContent, port->content.cec->LogAddr); /* Broadcast */
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_DEVICE_VENDOR_ID);
            TxContent[2] = AvCecGsvVendorIdByte1;
            TxContent[3] = AvCecGsvVendorIdByte2;
            TxContent[4] = AvCecGsvVendorIdByte3;
            AvKapiOutputDebugMessage("CEC: Sending Vendor ID");
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendCecVersion:
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 3;
            AV_CEC_SET_HDR_DA(TxContent, AV_CEC_SRC(RxContent), AV_CEC_DST(RxContent));
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_CEC_VERSION);
            TxContent[2] = 0x05; /* only support CEC 1.4 */
            AvKapiOutputDebugMessage("CEC: Sending CEC Version 1.4");
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendFeatureAbortReason:
            /* Abort Reason = wparam */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 4;
            port->content.cec->TxMsg = AV_CEC_MSG_FEATURE_ABORT;
            AV_CEC_SET_HDR_DA(TxContent, AV_CEC_SRC(RxContent), AV_CEC_DST(RxContent));
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_FEATURE_ABORT);
            TxContent[2] = AV_CEC_OPCODE(RxContent);
            TxContent[3] = (uint8)*wparam;
            AvKapiOutputDebugMessage("CEC: Sending feature abort. Abort Reason=%02x", *wparam);
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendReqActiveDevice:
            /* TV sends request active source */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen=2;
            AV_CEC_SET_HDR_BC(TxContent, port->content.tx->PhyAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_REQ_ACTIVE_SRC);
            AvKapiOutputDebugMessage("CEC MSG: Sending active source request");
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendSetSystemAudioModeToTv:
            /* Audio 1.2: response to system audio mode request */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen=3;
            AV_CEC_SET_HDR_DA(TxContent, AV_CEC_TV_LOG_ADDRESS, AvCecLogicAddress);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_SET_SYSTEM_AUDIO_MODE);
            TxContent[2] = AudioStatus;
            AvKapiCecSendMessage(port);
            Cec_Tx_Audio_Status.AudioMode = AudioStatus;
            break;

        case AvEventCecSendSetSystemAudioModeToAll:
            /* Audio 1.2: response to system audio mode request */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen=3;
            AV_CEC_SET_HDR_BC(TxContent, AvCecLogicAddress);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_SET_SYSTEM_AUDIO_MODE);
            TxContent[2] = AudioStatus;
            AvKapiCecSendMessage(port);
            Cec_Tx_Audio_Status.AudioMode = AudioStatus;
            break;

        case AvEventCecSendReportAudioStatus:
            /* response of AvEventCecMsgGiveAudioStatus */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 3;
            /* Directly addressed */
            AV_CEC_SET_HDR_DA(TxContent, AV_CEC_SRC(RxContent), port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_REPORT_AUDIO_STATUS);
            /* AV_CEC_AUDIO_MUTE_SHIFTER = bit 7 */
            TxContent[2] = Cec_Tx_Audio_Status.Mute << 7;
            /* CEC_AUDIO_MASK_MUTE_BIT = 0x7f */
            TxContent[2] = TxContent[2] | (Cec_Tx_Audio_Status.Volume & 0x7f);
            AvKapiOutputDebugMessage("CEC: Sending audio status");
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendReportSystemAudioModeStatus:
            /* response of AvEventCecMsgGiveSystemAudioModeStatus */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 3;
            /* Directly addressed */
            AV_CEC_SET_HDR_DA(TxContent, AV_CEC_SRC(RxContent), port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_SYSTEM_AUDIO_MODE_STATUS);
            TxContent[2] = Cec_Tx_Audio_Status.AudioMode;        /* audio mode status      */
            AvKapiOutputDebugMessage("CEC: Sending audio mode status");
            AvKapiCecSendMessage(port);
            break;

        case AvEventCecSendReportShortAudioDecriptor:
            /* response of AvEventCecMsgRequestShortAudioDescriptor */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 5;

            /* Directly addressed */
            AV_CEC_SET_HDR_DA(TxContent, AV_CEC_SRC(RxContent), port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_REPORT_SHORT_AUDIO_DESCRIPTOR);

            TxContent[2] = (Cec_Tx_Audio_Status.MaxNumberOfChannels-1) | ( (Cec_Tx_Audio_Status.AudioFormatCode) << 3);
            TxContent[3] = Cec_Tx_Audio_Status.AudioSampleRate;          /* audio  capability 2      */
            TxContent[4] = Cec_Tx_Audio_Status.AudioBitLen;
            AvKapiOutputDebugMessage("CEC: Sending audio capability");
            AvKapiCecSendMessage(port);
            break;

#endif

#if AvCecDataReadingInEvent

        case AvEventCecReceiveSystemAudioModeStatus:
            /* reg setting from AvEventCecMsgSystemAudioModeStatus */
            Cec_Tx_Audio_Status.AudioMode = RxContent[2];
            break;

        case AvEventCecReceiveSetSystemAudioMode:
            /* reg setting from AvEventCecMsgSetSystemAudioMode */
            Cec_Tx_Audio_Status.Mute = AV_CEC_AUDIO_MUTE_ON;
            if (RxContent[2])
            {
                Cec_Tx_Audio_Status.Mute = AV_CEC_AUDIO_MUTE_OFF;
            }
            Cec_Tx_Audio_Status.AudioMode = RxContent[2];
            break;

        case AvEventCecReceiveAudioStatus:
            /* reg setting from AvEventCecMsgReportAudioStatus */
            /* AV_CEC_AUDIO_MUTE_SHIFTER = bit 7 */
            Cec_Tx_Audio_Status.Mute = RxContent[2] >> 7;
            /* CEC_AUDIO_MASK_MUTE_BIT = 0x7f */
            Cec_Tx_Audio_Status.Volume = RxContent[2] & 0x7f;
            break;

        case AvEventCecReceiveSetAudioRate:
            /* reg setting from AvEventCecMsgSetAudioRate */
            Cec_Tx_Audio_Status.AudioRate = RxContent[2];
            break;

        case AvEventCecReceiveShortAudioDescriptor:
            /* reg setting from AvEventCecMsgReportShortAudioDescriptor */
            /* AV_CEC_AUDIO_FORMAT_ID_SHIFTER = 3 */
            Cec_Tx_Audio_Status.AudioFormatCode = RxContent[2] >> 3;
            /* CEC_AUDIO_MASK_AUDIOFORMATID_BIT = 0x07 */
            Cec_Tx_Audio_Status.MaxNumberOfChannels = RxContent[2] & 0x07;
            (Cec_Tx_Audio_Status.MaxNumberOfChannels)++;
            /* CEC_AUDIO_ENABLE_AUDIOSAMPLERATE_BIT = 0x7f */
            Cec_Tx_Audio_Status.AudioSampleRate = RxContent[3] & 0x7f;
            if ( Cec_Tx_Audio_Status.AudioFormatCode == AV_AUD_FORMAT_LINEAR_PCM)
            {
                Cec_Tx_Audio_Status.AudioBitLen  = RxContent[4];
            }
            else if ( (Cec_Tx_Audio_Status.AudioFormatCode >= AV_AUD_FORMAT_AC3)
                     && (Cec_Tx_Audio_Status.AudioFormatCode <= AV_AUD_FORMAT_AC3))
            {
                Cec_Tx_Audio_Status.MaxBitRate  = ((uint16)RxContent[4]) << 3;
            }
            break;

        case AvEventCecSendActiveSourceToAudio:
            /* reg setting from AvEventCecMsgReportPhyAddr */
            Cec_Tx_Audio_Status.ActiveSource = *wparam;
            break;

        case AvEventCecSendInitiateARC:
            /* called from AvEventCecArcManage */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen=2;
            AV_CEC_SET_HDR_DA(TxContent, AV_CEC_TV_LOG_ADDRESS, port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_INITIATE_ARC);
            AvKapiOutputDebugMessage("CEC: Sending Initiate ARC");
            AvKapiCecSendMessage(port);
            break;
        case AvEventCecSendTerminateARC:
            /* called from AvEventCecArcManage */
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 2;
            AV_CEC_SET_HDR_DA(TxContent, AV_CEC_TV_LOG_ADDRESS, port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_TERMINATE_ARC);
            AvKapiOutputDebugMessage("CEC: Sending Terminate ARC");
            AvKapiCecSendMessage(port);
            break;
        case AvEventCecSendMenuStatus:
            port->content.cec->TxSendFlag = AV_CEC_TX_TO_SEND;
            port->content.cec->TxLen = 3;
            AV_CEC_SET_HDR_DA(TxContent, AV_CEC_SRC(RxContent), port->content.cec->LogAddr);
            AV_CEC_SET_OPCODE(TxContent, AV_CEC_MSG_MENU_STATUS);
            if(RxContent[2] == 0x00) /* query = deactivate */
                TxContent[2] = 0x00;
            else
                TxContent[2] = 0x01;
            AvKapiOutputDebugMessage("CEC: Sending Menu Status");
            AvKapiCecSendMessage(port);
            break;

#endif

#if AvCecMessageEvent
        case AvEventCecMsgRouteChange:
            /* from RxMsg AV_CEC_MSG_ROUTE_CHANGE */
            if ((AV_CEC_BROADCAST(RxContent)) &&
                (AV_CEC_PHYS_ADDR2(RxContent) == port->content.tx->PhyAddr))
                /* New address is me */
            {
                Spa[0] = port->content.tx->PhyAddr>>8;
                Spa[1] = port->content.tx->PhyAddr & 0xff;
                AvHandleEvent(port, AvEventCecSendRoutingInformation, Spa, NULL);
                AvKapiOutputDebugMessage("CEC: Routing change. Old PA=%04x  New PA=%04x",
                                         AV_CEC_PHYS_ADDR1(RxContent),
                                         AV_CEC_PHYS_ADDR2(RxContent));
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

        case AvEventCecMsgRouteInfo:
            /* from RxMsg AV_CEC_MSG_ROUTE_INFO */
            if (AV_CEC_BROADCAST(RxContent))
            {
                Spa[0] = port->content.tx->PhyAddr>>8;
                Spa[1] = port->content.tx->PhyAddr & 0xff;
                AvHandleEvent(port, AvEventCecSendRoutingInformation, Spa, NULL);
                AvKapiOutputDebugMessage("CEC: Routing Info. Active Route=%04x",
                                         AV_CEC_PHYS_ADDR1(RxContent));
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

        case AvEventCecMsgActiveSource:
            /* from RxMsg AV_CEC_MSG_ACTIVE_SRC */
            /* when getting ACTIVE_SRC, power up at first */
            if (AV_CEC_BROADCAST(RxContent))
            {
                Recv_Phy_Addr  = AV_CEC_PHYS_ADDR1(RxContent);
                InpIndex = AvCecFindRequiredInput (port, Recv_Phy_Addr);
                /* Low 4 bits = Input Total, High 4 bits = Current Active Input */
                /* InpIndex is within InputCount Range, and not invalid value 0 */
                if((InpIndex <= ((port->content.cec->InputCount)&0xf)) &&
                   (InpIndex != 0))
                {
                    AvKapiOutputDebugMessage("************Active Source Request From %04x (Port %d)",
                                             Recv_Phy_Addr, InpIndex);
#if AvCecLinkageMode
                    /* Active Input needs to be switched */
                    if(InpIndex != (port->content.cec->InputCount>>4))
                    {
                        AvKapiOutputDebugMessage("CEC Source try to connect Rx%d - Tx%d", InpIndex, port->index-3);
                        /* Find Device's first port (Rx Port) */
                        TempPort = (AvPort*)port->device->port;
                        port->content.cec->InputCount = (InpIndex<<4) | (port->content.cec->InputCount & 0xf);
                        AvApiConnectPort(&TempPort[InpIndex-1], &TempPort[2], AvConnectAV);
                    }
                    /* Current Active Input is already streaming  */
                    else
                    {
                        AvKapiOutputDebugMessage("Current Input %d is already streaming", InpIndex);
                    }
#endif
                }
                else
                {
                    AvKapiOutputDebugMessage("CEC: ignore");
                }
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

        case AvEventCecMsgGivePhyAddr:
            /* from RxMsg AV_CEC_MSG_GIVE_PHYS_ADDR */
            /* only report phyaddr of self */
            if (AV_CEC_DST(RxContent) == port->content.cec->LogAddr)
            {
                AvKapiOutputDebugMessage("CEC: Give physical address");
                AvHandleEvent(port, AvEventCecSendReportPhysAddress, 0, NULL);
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

        case AvEventCecMsgSetStreamPath:
            /* from RxMsg AV_CEC_MSG_SET_STRM_PATH */
            if (AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("************Set Stream Path Request to Source=%04x", AV_CEC_PHYS_ADDR1(RxContent));
                if (AV_CEC_PHYS_ADDR1(RxContent)==port->content.tx->PhyAddr)
                {
                    AvHandleEvent(port, AvEventCecSendActiveSource, 0, NULL);
                }
                else
                {
#if AvCecLinkageMode
                    /* Switch to appropriate input */
                    InpIndex = AvCecFindRequiredInput (port, AV_CEC_PHYS_ADDR1(RxContent));
                    if (InpIndex <= port->content.cec->InputCount)
                    {
                        AvHandleEvent(port, AvEventCecSendActiveSource, 0, NULL);
                        /* Set Stream requires a new Source */
                        if(InpIndex != (port->content.cec->InputCount>>4))
                        {
                            /* Find Device's first port (Rx Port) */
                            AvKapiOutputDebugMessage("CEC TV try to connect Rx%d - Tx%d", InpIndex, port->index-3);
                            TempPort = (AvPort*)port->device->port;
                            port->content.cec->InputCount = (InpIndex<<4) | (port->content.cec->InputCount & 0xf);
                            AvApiConnectPort(&TempPort[InpIndex-1], &TempPort[2], AvConnectAV);
                        }
                    }
#endif
                }
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

        case AvEventCecMsgStandby:
            /* from RxMsg AV_CEC_MSG_STANDBY */
            port->content.cec->RxMsg = AV_CEC_MSG_STANDBY;
            port->content.cec->RxLen = 0;
            ret = AvHandleEvent(port, AvEventCecRxMessageRespond,
                                    Msg_Len,
                                    NULL);
            if ( ret == AvOk )
            {
                AvKapiOutputDebugMessage("CEC: To be in Standby mode");
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: is already in Standby mode, ignore");
            }
            break;

        case AvEventCecMsgAbort:
            /* from RxMsg AV_CEC_MSG_ABORT */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: Abort message");
                AvHandleEvent(port, AvEventCecSendFeatureAbort, 0, NULL);
            }
            else
            {
                 AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

        case AvEventCecMsgFeatureAbort:
            /* from RxMsg AV_CEC_MSG_FEATURE_ABORT */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: Feature abort message");
                /*
                if ( RxContent[2] == 0x72 )
                {
                    port->content.cec->SendAudioCecFlag = 0;
                }
                */
            }
            else
            {
                 AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

        case AvEventCecMsgGivePowerStatus:
            /* from RxMsg AV_CEC_MSG_GIVE_PWR_STATUS */
            /* when getting GIVE_PWR_STATUS, first to power up, then to REPORT_PWR_STATUS */
            AvHandleEvent(port, AvEventCecRxMessageRespond,Msg_Len, NULL);
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: Give power status message");
                AvHandleEvent(port, AvEventCecSendPowerStatus, 0, NULL);
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

        case AvEventCecMsgReportPowerStatus:
            /* from RxMsg AV_CEC_MSG_REPORT_PWR_STATUS */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: get reporting power status message");
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

        case AvEventCecMsgGiveOsdName:
            /* from RxMsg AV_CEC_MSG_GIVE_OSD_NAME */
            if ((!AV_CEC_BROADCAST(RxContent)) &&
                (AV_CEC_SRC(RxContent) != AV_CEC_BROADCAST_LOG_ADDRESS ))
            {
                AvKapiOutputDebugMessage("CEC: Give OSD name message");
                AvHandleEvent(port, AvEventCecSendSetOsdName, 0, NULL);
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

        case AvEventCecMsgGetVendorId:
            /* from RxMsg AV_CEC_MSG_GET_VENDOR_ID */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: Get Vendor ID");
                AvHandleEvent(port, AvEventCecSendDeviceVendorID, 0, NULL);
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

       case AvEventCecMsgGetCecVersion:
           /* from RxMsg AV_CEC_MSG_GET_CEC_VERSION */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: Get Cec Version");
                AvHandleEvent(port, AvEventCecSendCecVersion, 0, NULL);
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

        case AvEventCecMsgRequestArcInitiation:
            /* from RxMsg AV_CEC_MSG_REQUEST_ARC_INITIATION */
            i = 0;
            if((port->content.tx->PhyAddr >> 8)&0x000F)i++;
            if((port->content.tx->PhyAddr >> 4)&0x000F)i++;
            if(port->content.tx->PhyAddr & 0x000F)i++;

            /* ARC is directly from TV */
            if ( (AV_CEC_SRC(RxContent) == AV_CEC_TV_LOG_ADDRESS) &&
                 (!AV_CEC_BROADCAST(RxContent)) && ( i == 0))
            {

                AvKapiOutputDebugMessage("CEC: Request ARC Initiation message");
                ret = AvHandleEvent(port, AvEventCecSendInitiateARC, NULL, NULL);
                port->content.cec->EnableARC = AV_CEC_ARC_TO_INITIATE;

                if( ret != AvOk )
                {
                    AvHandleEvent(port, AvEventCecSendFeatureAbortReason, 0, NULL);
                }
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

        case AvEventCecMsgReportArcInitiated:
            /* from RxMsg AV_CEC_MSG_REPORT_ARC_INITIATED */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: Report ARC Initiated message");
                port->content.cec->EnableARC = AV_CEC_ARC_INITIATED;
                if(AudioStatus == 0)
                {
                    port->content.cec->EnableARC = AV_CEC_ARC_TO_TERMINATE;
                    port->content.cec->ARCTryCount = 0;
                }
                else
                    AvHandleEvent(port, AvEventCecSendSetSystemAudioModeToTv, NULL, NULL);
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

        case AvEventCecMsgRequestArcTermination:
            /* from RxMsg AV_CEC_MSG_REQUEST_ARC_TERMINATION */
            if ( (AV_CEC_SRC(RxContent) == AV_CEC_TV_LOG_ADDRESS) &&
                 (!AV_CEC_BROADCAST(RxContent)) )
            {
                AvKapiOutputDebugMessage("CEC: Request ARC Termination message");
                ret = AvHandleEvent(port, AvEventCecSendTerminateARC, NULL, NULL);
                port->content.cec->EnableARC = AV_CEC_ARC_TO_TERMINATE;
                if( ret != AvOk )
                {
                    AvHandleEvent(port, AvEventCecSendFeatureAbortReason, 0, NULL);
                }
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

        case AvEventCecMsgReportArcTerminated:
            /* from RxMsg AV_CEC_MSG_REPORT_ARC_TERMINATED */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: Report ARC Terminated message");
                if(AudioStatus == 1)
                {
                    port->content.cec->EnableAudioAmplifier = AV_CEC_AMP_TO_ENABLE;
                    port->content.cec->EnableARC = AV_CEC_ARC_TO_INITIATE;
                    port->content.cec->ARCTryCount = 0;
                }
                else
                    AvHandleEvent(port, AvEventCecSendSetSystemAudioModeToTv, NULL, NULL);
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

        case AvEventCecMsgUserControlPressed:
            /* from RxMsg AV_CEC_MSG_USER_CONTROL_PRESSED */
            /* when getting USER_CONTROL_PRESSED, UI Command is POWER ON, then to power up */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: User Control Pressed with UI Command = 0x%x", RxContent[2]);
                switch(RxContent[2])
                {
                    case 0x41: /* Volume Up */
                        if(Cec_Tx_Audio_Status.Volume <= 100)
                            Cec_Tx_Audio_Status.Volume = Cec_Tx_Audio_Status.Volume + 10;
                        AvHandleEvent(port, AvEventCecSendReportAudioStatus, 0, NULL);
                        break;
                    case 0x42: /* Volume Down */
                        if(Cec_Tx_Audio_Status.Volume >= 10)
                            Cec_Tx_Audio_Status.Volume = Cec_Tx_Audio_Status.Volume - 10;
                        AvHandleEvent(port, AvEventCecSendReportAudioStatus, 0, NULL);
                        break;
                    default:
                        break;
                }
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

        case AvEventCecMsgUserControlReleased:
            /* from RxMsg AV_CEC_MSG_USER_CONTROL_RELEASED */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: User Control Released with UI Command = 0x%x", RxContent[2]);
                switch(RxContent[2])
                {
                    case 0x41: /* Volume Up */
                        AvHandleEvent(port, AvEventCecSendReportAudioStatus, 0, NULL);
                        break;
                    case 0x42: /* Volume Down */
                        AvHandleEvent(port, AvEventCecSendReportAudioStatus, 0, NULL);
                        break;
                    default:
                        break;
                }
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

        case AvEventCecMsgImageViewOn:
        case AvEventCecMsgTextViewOn:
            /* when getting IMAGE_VIEW_ON or TEXT_VIEW_ON to power up */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: Image/Text View On");
                AvHandleEvent(port, AvEventCecSendReqActiveDevice, 0, NULL);
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

        case AvEventCecMsgSystemAudioModeRequest:
            if (!AV_CEC_BROADCAST(RxContent))
            {
                if ( port->content.cec->RxLen == 2 )
                {
                    AvHandleEvent(port, AvEventCecSendSetSystemAudioModeToTv, NULL, NULL);
                }
                else if ( port->content.cec->RxLen == 4)
                {
                    InpIndex = AvCecFindRequiredInput (port, AV_CEC_PHYS_ADDR1(RxContent));
                    AvKapiOutputDebugMessage("CEC: Active Source is %04x (Port %d)",
                                             AV_CEC_PHYS_ADDR1(RxContent), InpIndex);
                    AvHandleEvent(port, AvEventCecSendSetSystemAudioModeToAll, NULL, NULL);
                }
                else
                {
                    AvKapiOutputDebugMessage("CEC: parameters number is wrong");
                    AvKapiOutputDebugMessage("CEC: ignore");
                }
                /* Audio ARC Protection */
                if(AudioStatus == 0)
                    port->content.cec->EnableAudioAmplifier = AV_CEC_AMP_TO_DISABLE;
                else
                {
                    port->content.cec->EnableAudioAmplifier = AV_CEC_AMP_TO_ENABLE;
                    port->content.cec->EnableARC = AV_CEC_ARC_TO_INITIATE;
                }
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore");

            }
            break;

        case AvEventCecMsgDeckControl:
            /* from RxMsg AV_CEC_MSG_DECK_CONTROL */
            /*================================================
             * when getting DECK_CONTROL
             * to power up
             * no action to [Deck Control Mode], and
             * not to response by <Deck Status> [Deck Info]
             *================================================*/
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: DECK CONTROL message");
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

        case AvEventCecMsgPlay:
            /* from RxMsg AV_CEC_MSG_PLAY */
            /*===============================================
             * when getting PLAY
             * to power up
             * no action to [Play Mode], and
             * not to response by <Deck Status> [Deck Info]
             *===============================================*/
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: PLAY message");
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore");

            }
            break;

        case AvEventCecMsgGiveAudioStatus:
            /* from RxMsg AV_CEC_MSG_GIVE_AUDIO_STATUS */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: give audio status");
                AvHandleEvent(port, AvEventCecSendReportAudioStatus, 0, NULL);
                /* SendReportAudioStatus(RxContent); */
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

        case AvEventCecMsgGiveSystemAudioModeStatus:
            /* from RxMsg AV_CEC_MSG_GIVE_SYSTEM_AUDIO_MODE_STATUS */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: give system audio mode status");
                AvHandleEvent(port, AvEventCecSendReportSystemAudioModeStatus, 0, NULL);
                /* SendReportSystemAudioModeStatus(RxContent); */
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

        case AvEventCecMsgRequestShortAudioDescriptor:
            /* from RxMsg AV_CEC_MSG_REQUEST_SHORT_AUDIO_DESCRIPTOR */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: give short audio descriptor");
                AvHandleEvent(port, AvEventCecSendReportShortAudioDecriptor, 0, NULL);
                /* SendReportShortAudioDecriptor(RxContent); */
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

        case AvEventCecMsgSystemAudioModeStatus:
            /* from RxMsg AV_CEC_MSG_SYSTEM_AUDIO_MODE_STATUS */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvHandleEvent(port, AvEventCecReceiveSystemAudioModeStatus, 0, NULL);
                /* ReceiveSystemAudioModeStatus(RxContent); */
                AvKapiOutputDebugMessage("CEC: receiving message about system audio mode status");
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

        case AvEventCecMsgSetSystemAudioMode:
            /* from RxMsg AV_CEC_MSG_SET_SYSTEM_AUDIO_MODE */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                ret = AvHandleEvent(port, AvEventCecReceiveSetSystemAudioMode, 0, NULL);
                /* ret = ReceiveSetSystemAudioMode(RxContent); */
                /*
                if ( ret == AvOk )
                {
                    port->content.cec->RxMsg = AV_CEC_MSG_SET_SYSTEM_AUDIO_MODE;
                    port->content.cec->RxLen = 0;
                    *Msg_Len = *Msg_Len +  port->content.cec->RxLen;
                    ret = AvHandleEvent(port, AvEventCecRxMessageRespond,
                                               Msg_Len,
                                               NULL);
                }
                */
                AvKapiOutputDebugMessage("CEC receiving message about setting system audio mode");
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

        case AvEventCecMsgReportAudioStatus:
            /* from RxMsg AV_CEC_MSG_REPORT_AUDIO_STATUS */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvHandleEvent(port, AvEventCecReceiveAudioStatus, 0, NULL);
                /* ReceiveAudioStatus(RxContent); */
                AvKapiOutputDebugMessage("CEC: receiving message about audio status");
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

        case AvEventCecMsgSetAudioRate:
            /* from RxMsg AV_CEC_MSG_SET_AUDIO_RATE */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvHandleEvent(port, AvEventCecReceiveSetAudioRate, 0, NULL);
                /* ReceiveSetAudioRate(RxContent); */
                AvKapiOutputDebugMessage("CEC: receiving message about audio rate");
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

        case AvEventCecMsgReportShortAudioDescriptor:
            /* from RxMsg AV_CEC_MSG_REPORT_SHORT_AUDIO_DESCRIPTOR */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: receiving message about short audio descriptor");
                AvHandleEvent(port, AvEventCecReceiveShortAudioDescriptor, 0, NULL);
                /* ReceiveShortAudioDescriptor(RxContent); */
             }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

        case AvEventCecMsgReportPhyAddr:
            /* from RxMsg AV_CEC_MSG_REPORT_PHYS_ADDR */
            if (AV_CEC_BROADCAST(RxContent))
            {
                ActiveSource = (((uint16)(RxContent[2])<<8)+
                                (uint16)(RxContent[3]));
                /*
                param = (uint32)ActiveSource;
                AvHandleEvent(port, AvEventCecSendActiveSourceToAudio, &param, NULL);
                */
                /* SendActiveSourceToAudio(ActiveSource); */
                AvKapiOutputDebugMessage("CEC: source %x reports phy addr",ActiveSource);
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

        case AvEventCecMsgMenuRequest:
            /* from RxMsg AV_CEC_MSG_MENU_REQUEST */
            if (!AV_CEC_BROADCAST(RxContent))
            {
                AvKapiOutputDebugMessage("CEC: Receive menu request %x", RxContent[2]);
                AvHandleEvent(port, AvEventCecSendMenuStatus, NULL, NULL);
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

        case AvEventCecMsgRequestActiveSource:
            /* from RxMsg AV_CEC_MSG_REQ_ACTIVE_SRC */
            if((AV_CEC_BROADCAST(RxContent)) &&
               (port->content.cec->LogAddr != AV_CEC_TV_LOG_ADDRESS))
            {
                AvHandleEvent(port, AvEventCecSendActiveSource, NULL, NULL);
            }
            else
            {
                 AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

        case AvEventCecMsgSetMenuLanguage:
            /* from RxMsg AV_CEC_MSG_SET_MENU_LANGUAGE */
            if(AV_CEC_BROADCAST(RxContent)&&
               (port->content.cec->LogAddr == AV_CEC_TV_LOG_ADDRESS))
            {
                AvKapiOutputDebugMessage("CEC: Set menu language");
            }
            else
            {
                AvKapiOutputDebugMessage("CEC: ignore");
            }
            break;

        case AvEventCecMsgDeviceVendorId:
            AvKapiOutputDebugMessage("CEC: ignore");
            break;

#endif

        default:
            ret = AvNotAvailable;
            break;
    }

    return ret;
}

#if AvEnableCecFeature /* CEC Related */
uint16 AvCecFindRequiredInput (AvPort *port, uint16 SrcPhys)
{
    /* PhyAddr order is fixed */
    uint16 i;
    uint8  shift = 12;
    uint16 Mask=0xf000, Mask2=0;

    for (i=0; i<4; i++)
    {
        if ((port->content.tx->PhyAddr & Mask) == 0)
        {
            break;
        }
        Mask2 |= Mask;
        Mask >>= 4;
        if(shift == 0)
            return (0xff);
        else
            shift = shift - 4;
    }

    if ((port->content.tx->PhyAddr & Mask2) == (SrcPhys & Mask2))
    {
        i = (SrcPhys>>shift) & 0x0f;
        return(i);
    }
    return (0xff);
}
#endif /* CEC Related */

AvRet AvEdidPortManage(AvPort *RxPort)
{
    uint8 SpaLocation = 0;
    uint8 SpaValue[2];
    uint8 SinkNumber = 0; /* default to force mode */
    AvPort *CurrentPort = NULL;
    AvPort *PrevPort = NULL;
    uint8 i=0;

    /* 1. Clear RAM header, save time */
    AvMemset(InEdid, 0, 256);
    AvMemset(SinkEdid, 0, 256);
    AvMemset(OutEdid, 0, 256);
    /* 1.1 Set Edid Parameter */
    AvEdidSupportFreq = AvEdidBitFreq6G    |
                        AvEdidBitFreq4P5G  |
                        AvEdidBitFreq3P75G |
                        AvEdidBitFreq3G    |
                        AvEdidBitFreq2P25G |
                        AvEdidBitFreq1P5G  |
                        AvEdidBitFreq750M  |
                        AvEdidBitFreq270M  |
                        AvEdidBitFreq135M;
    AvEdidCeaParamForce  =  0; // AvEdidBitCeaSVD | AvEdidBitCeaVSDBHF;
    AvEdidCeaParamRemove =  0;

    /* 2. Look up front port */
    CurrentPort = NULL;
    PrevPort = NULL;
    SinkNumber = 0;
    /* loop to the end of the same level connections */
    /* Merge every 2 port Edids a time */
    while(KfunFindVideoNextTxEnd(RxPort, &PrevPort, &CurrentPort) == AvOk)
    {
        if((CurrentPort->type == HdmiTx) &&
           (CurrentPort->content.tx->EdidReadSuccess == AV_EDID_UPDATED) &&
           (CurrentPort->content.tx->IgnoreEdidError == 0))
        {
            if(SinkNumber != 2)
                SinkNumber = SinkNumber + 1;
            AvKapiOutputDebugMessage("Rx %d Edid Manage: Read Tx %d EDID",RxPort->index+1, CurrentPort->index-3);
            /* find the next Edid Ram Ptr */
            if(SinkNumber == 1)
                KfunTxReadEdid(CurrentPort,InEdid);
            else
            {
                KfunTxReadEdid(CurrentPort,SinkEdid);
                /* Merge Edids */
                AvEdidFuncStructInit(&DevEdidReg);
                AvMemset(OutEdid, 0, 256);
                AvEdidFuncVesaProcess(&DevEdidReg,InEdid,SinkEdid,OutEdid);
                AvEdidFuncCeaProcess(&DevEdidReg,InEdid,SinkEdid,OutEdid);
                AvMemcpy(InEdid,OutEdid,256);
                AvMemset(SinkEdid, 0, 256);
            }
        }
        PrevPort = CurrentPort;
    }

    /* 3. Final Merge Process */
    if(SinkNumber <= 1)
    {
        AvEdidFuncStructInit(&DevEdidReg);
        AvEdidFuncVesaProcess(&DevEdidReg,InEdid,SinkEdid,OutEdid);
        AvEdidFuncCeaProcess(&DevEdidReg,InEdid,SinkEdid,OutEdid);
    }
    //AvMemcpy(OutEdid+0,(void*)(&AvDefaultEDID[0]),256);
    AvEdidFuncCheckSum(OutEdid);

    /* 4. Find Spa Location and generate its own SPA for source */
    SpaLocation = KfunFindCecSPAFromEdid(OutEdid, SpaValue);
    KfunGenerateSourceSpa(RxPort, SpaValue, 0);

    AvKapiOutputDebugMessage("Rx %d Generated Edid Start: ",RxPort->index+1);
    for(i=0;i<16;i++)
    {
        AvKapiOutputDebugMessage(" %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x",
                                 OutEdid[i*16+0],OutEdid[i*16+1],OutEdid[i*16+2],OutEdid[i*16+3],
                                 OutEdid[i*16+4],OutEdid[i*16+5],OutEdid[i*16+6],OutEdid[i*16+7],
                                 OutEdid[i*16+8],OutEdid[i*16+9],OutEdid[i*16+10],OutEdid[i*16+11],
                                 OutEdid[i*16+12],OutEdid[i*16+13],OutEdid[i*16+14],OutEdid[i*16+15]);
    }

    /* 5. write Edid, set rx->EdidStatus to AV_EDID_NEEDUPDATE */
    KfunRxWriteEdid(RxPort, OutEdid, SpaLocation, SpaValue);
    RxPort->content.rx->EdidStatus = AV_EDID_UPDATED;

    return AvOk;
}

AvRet AvEdidPortAnalysis(AvPort *port)
{
    uint8 i=0;

    KfunTxReadEdid(port,InEdid);
    AvEdidFuncStructInit(&DevEdidReg);
    AvEdidFunFullAnalysis(&DevEdidReg,InEdid);

    AvKapiOutputDebugMessage("Tx %d Read Edid Start: ",port->index-3);
    for(i=0;i<16;i++)
    {
        AvKapiOutputDebugMessage(" %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x",
                                 InEdid[i*16+0], InEdid[i*16+1], InEdid[i*16+2], InEdid[i*16+3],
                                 InEdid[i*16+4], InEdid[i*16+5], InEdid[i*16+6], InEdid[i*16+7],
                                 InEdid[i*16+8], InEdid[i*16+9], InEdid[i*16+10],InEdid[i*16+11],
                                 InEdid[i*16+12],InEdid[i*16+13],InEdid[i*16+14],InEdid[i*16+15]);
    }

    if(DevEdidReg.MaxTmdsClk == 0)
    {
        DevEdidReg.MaxTmdsClk  = DevEdidReg.VesaMaxClk;
        DevEdidReg.MaxCharRate = DevEdidReg.VesaMaxClk;
    }
    /* Feature Support */
    port->content.tx->EdidSupportFeature = AV_BIT_FEAT_1G5;
    if((DevEdidReg.VsdbHfCheck[3] == 1) || (DevEdidReg.MaxTmdsClk > 340) || (DevEdidReg.MaxCharRate > 340) || (DevEdidReg.VesaMaxClk > 340))
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_SCDC;
    if(DevEdidReg.VsdbHfCheck[8] == 1)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_RR;
    if(DevEdidReg.VsdbHfCheck[7] == 1)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_B340MSCR;
    if(DevEdidReg.VsdbHfCheck[1] == 1)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_36B420;
    if(DevEdidReg.VsdbHfCheck[2] == 1)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_30B420;
    if(DevEdidReg.MaxCharRate >= 600)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_6G;
    if(DevEdidReg.MaxCharRate >= 450)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_4G5;
    if(DevEdidReg.MaxCharRate >= 370)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_3G75;
    if(DevEdidReg.MaxTmdsClk >= 300)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_3G;
    if(DevEdidReg.MaxTmdsClk >= 225)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_2G25;
    if((DevEdidReg.HdrStCheck[0] != 0) ||
       (DevEdidReg.HdrStCheck[1] != 0) ||
       (DevEdidReg.HdrStCheck[2] != 0) ||
       (DevEdidReg.HdrStCheck[3] != 0))
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_HDR;
    if(DevEdidReg.Y420VdbCheck[0] == 1)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_4K50_420;
    if(DevEdidReg.Y420VdbCheck[1] == 1)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_4K60_420;
    if(DevEdidReg.Y420VdbCheck[2] == 1)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_4KS50_420;
    if(DevEdidReg.Y420VdbCheck[3] == 1)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_4KS60_420;
    if(DevEdidReg.CdbCheck[0] == 1)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_YUV_601;
    if(DevEdidReg.CdbCheck[1] == 1)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_YUV_709;
    if((DevEdidReg.CdbCheck[5] == 1) || (DevEdidReg.CdbCheck[6] == 1))
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_YCC_2020;
    if(DevEdidReg.CdbCheck[7] == 1)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_RGB_2020;
    if(DevEdidReg.VcdbCheck[0] == 1)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_YUV_FULL_RANGE;
    if(DevEdidReg.VcdbCheck[1] == 1)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_RGB_FULL_RANGE;
    if(DevEdidReg.VsdbCheckList[11] == 1)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_10B_DEEP_COLOR;
    if(DevEdidReg.VsdbCheckList[10] == 1)
        port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_12B_DEEP_COLOR;
    for(i=1;i<14;i++)
    {
        if(DevEdidReg.EdidCeaAudioCheck[i] == 1)
        {
            port->content.tx->EdidSupportFeature = port->content.tx->EdidSupportFeature | AV_BIT_FEAT_COMPRESS_AUDIO;
            break;
        }
    }

    return AvOk;
}

RouteStat AvPortRoutingPolicy(AvPort *RxPort, AvPort *TxPort)
{
    RouteStat TxConnectStyle = ROUTE_NO_CONNECT;
    /* 1. RxPort - TxPort connection */
    if((RxPort->type == HdmiRx) && (TxPort->type == HdmiTx))
    {
        if((TxPort->content.tx->EdidReadSuccess != AV_EDID_UPDATED) ||
           (TxPort->content.tx->Hpd != AV_HPD_HIGH))
            return ROUTE_NO_CONNECT;
        /* 1.1 6G Tx Port */
        if((TxPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_SCDC) && /* support SCDC */
           (TxPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_6G))     /* support 6G */
        {
            /* 1.1.1 default bypass */
            TxConnectStyle = ROUTE_1_R_T;
        }
        /* 1.2 3G Tx Port */
        else if((TxPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_3G)) /* support 3G */
        {
            /* 1.2.1 Rx is 6G, go into vsp, if no 420, then downscale */
            if(RxPort->content.video->info.TmdsFreq >= 580)
            {
                switch(RxPort->content.video->timing.Vic)
                {
                    case 96:  /* 4K50 */
                    case 106:
                        if(TxPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_4K50_420)
                            TxConnectStyle = ROUTE_2_R_C_T;
                        else
                            TxConnectStyle = ROUTE_3_R_S_T;
                        break;
                    case 97:  /* 4K60 */
                    case 107:
                        if(TxPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_4K60_420)
                            TxConnectStyle = ROUTE_2_R_C_T;
                        else
                            TxConnectStyle = ROUTE_3_R_S_T;
                        break;
                    case 101: /* 4KS50 */
                        if(TxPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_4KS50_420)
                            TxConnectStyle = ROUTE_2_R_C_T;
                        else
                            TxConnectStyle = ROUTE_3_R_S_T;
                        break;
                    case 102: /* 4KS60 */
                        if(TxPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_4KS60_420)
                            TxConnectStyle = ROUTE_2_R_C_T;
                        else
                            TxConnectStyle = ROUTE_3_R_S_T;
                        break;
                    case 0:
                        TxConnectStyle = ROUTE_NO_CONNECT;
                        break;
                    default: /* down scale to output */
                        TxConnectStyle = ROUTE_3_R_S_T;
                        break;
                }
            }
            /* 1.2.2 Rx is 3G */
            else if(RxPort->content.video->info.TmdsFreq >= 290)
            {
                /* 1.2.2.1 bypass 420 if support, or else, downscale */
                if(RxPort->content.video->Y == AV_Y2Y1Y0_YCBCR_420)
                {
                    /* 1.2.2.1.1 only do 420 if support format */
                    if((RxPort->content.video->InCs == AV_CS_YUV_601) ||
                       (RxPort->content.video->InCs == AV_CS_YUV_709) ||
                       (RxPort->content.video->InCs == AV_CS_YCC_601) ||
                       (RxPort->content.video->InCs == AV_CS_YCC_709) ||
                       (RxPort->content.video->InCs == AV_CS_SYCC_601) ||
                       (RxPort->content.video->InCs == AV_CS_ADOBE_YCC_601) ||
                       (RxPort->content.video->InCs == AV_CS_BT2020_YCC) ||
                       (RxPort->content.video->InCs == AV_CS_BT2020_RGB) ||
                       (RxPort->content.video->InCs == AV_CS_LIM_BT2020_YCC) ||
                       (RxPort->content.video->InCs == AV_CS_LIM_BT2020_RGB) ||
                       (RxPort->content.video->InCs == AV_CS_LIM_YUV_601) ||
                       (RxPort->content.video->InCs == AV_CS_LIM_YUV_709) ||
                       (RxPort->content.video->InCs == AV_CS_LIM_YCC_601) ||
                       (RxPort->content.video->InCs == AV_CS_LIM_YCC_709) ||
                       (RxPort->content.video->InCs == AV_CS_LIM_SYCC_601) ||
                       (RxPort->content.video->InCs == AV_CS_LIM_ADOBE_YCC_601))
                    {
                        switch(RxPort->content.video->timing.Vic)
                        {
                            case 96:  /* 4K50 */
                            case 106:
                                if(TxPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_4K50_420)
                                    TxConnectStyle = ROUTE_1_R_T;
                                else
                                    TxConnectStyle = ROUTE_3_R_S_T;
                                break;
                            case 97:  /* 4K60 */
                            case 107:
                                if(TxPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_4K60_420)
                                    TxConnectStyle = ROUTE_1_R_T;
                                else
                                    TxConnectStyle = ROUTE_3_R_S_T;
                                break;
                            case 101: /* 4KS50 */
                                if(TxPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_4KS50_420)
                                    TxConnectStyle = ROUTE_1_R_T;
                                else
                                    TxConnectStyle = ROUTE_3_R_S_T;
                                break;
                            case 102: /* 4KS60 */
                                if(TxPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_4KS60_420)
                                    TxConnectStyle = ROUTE_1_R_T;
                                else
                                    TxConnectStyle = ROUTE_3_R_S_T;
                                break;
                            case 0:
                                TxConnectStyle = ROUTE_NO_CONNECT;
                                break;
                            default: /* down scale to output */
                                TxConnectStyle = ROUTE_3_R_S_T;
                                break;
                        }
                    }
                    /* 1.2.2.1.2 still bypass it */
                    else
                        TxConnectStyle = ROUTE_1_R_T;
                }
                else
                    TxConnectStyle = ROUTE_1_R_T;
            }
            /* 1.2.3 Rx is below 3G */
            else
                TxConnectStyle = ROUTE_1_R_T;
        }
        /* 1.3 1G5 Tx Port */
        else if((TxPort->content.tx->EdidSupportFeature & AV_BIT_FEAT_1G5)) /* support 1G5 */
        {
            /* 1.3.2 Rx is 6G, go into vsp, downscale */
            if(RxPort->content.video->info.TmdsFreq >= 580)
                TxConnectStyle = ROUTE_3_R_S_T;
            /* 1.3.3 Rx is 3G */
            else if(RxPort->content.video->info.TmdsFreq >= 290)
                TxConnectStyle = ROUTE_3_R_S_T;
            /* 1.3.4 Rx is below 3G */
            else
                TxConnectStyle = ROUTE_1_R_T;
        }
        /* 1.4 Don't care, direct output */
        else
            TxConnectStyle = ROUTE_1_R_T;
    }
    /* 2. RxPort - LogicTxPort connection */
    else if((RxPort->type == HdmiRx) && (TxPort->type == LogicVideoTx))
    {
        /* 2.1 UDP mode */
        if((ParallelConfigTable[TxPort->content.lvtx->Config*3 + 2] & 0x20) != 0)
            TxConnectStyle = ROUTE_4_R_V;
        /* 2.2 480i/576i -> 480p/576p conversion for low frequency protection */
        else if((RxPort->content.video->timing.Interlaced == 1) &&
                (RxPort->content.video->info.TmdsFreq < 45))
            TxConnectStyle = ROUTE_6_R_S_V;
        /* 2.3 RxPort->Downscaler->LogicVideoTx with possible CSC for 1 pixel/clk mode */
        else if(((RxPort->content.video->Y == AV_Y2Y1Y0_YCBCR_420) &&
                 (RxPort->content.video->info.TmdsFreq > 340)) &&
                ((ParallelConfigTable[TxPort->content.lvtx->Config*3 + 0] & 0x30) == 0))
            TxConnectStyle = ROUTE_6_R_S_V;
        /* 2.4 HDMI 2.0 -> HDMI 1.4 conversion required */
        else if(RxPort->content.video->info.TmdsFreq > TxPort->content.video->info.TmdsFreq)
            TxConnectStyle = ROUTE_6_R_S_V;
        /* 2.5 YCbCr 422 Output */
        else if(((ParallelConfigTable[TxPort->content.lvtx->Config*3 + 1] & 0x04) != 0) ||
                ((ParallelConfigTable[TxPort->content.lvtx->Config*3 + 2] & 0x40) != 0) ||
                (TxPort->content.video->Y == AV_Y2Y1Y0_YCBCR_422))
        {
            /* 2.5.1 Direct connection between RxPort->LogicVideoTx with no CSC */
            if(((RxPort->content.video->Y == AV_Y2Y1Y0_YCBCR_444) ||
                (RxPort->content.video->Y == AV_Y2Y1Y0_YCBCR_422)) &&
               (TxPort->content.video->InCs == AV_CS_AUTO))
                TxConnectStyle = ROUTE_4_R_V;
            /* 2.5.2 RxPort->CSC->LogicVideoTx */
            else
                TxConnectStyle = ROUTE_5_R_C_V;
        }
        /* 2.6 Color Processing is needed */
        else if(((TxPort->content.video->Y != AV_Y2Y1Y0_INVALID) &&
                 (RxPort->content.video->Y != TxPort->content.video->Y)) ||
                ((TxPort->content.video->InCs != AV_CS_AUTO) &&
                 (RxPort->content.video->InCs != TxPort->content.video->InCs)))
        {
            /* 2.6.1 420 always do downscaling for color space conversion */
            if(RxPort->content.video->Y == AV_Y2Y1Y0_YCBCR_420)
                TxConnectStyle = ROUTE_6_R_S_V;
            else
                TxConnectStyle = ROUTE_5_R_C_V;
        }
        /* 2.7 Direct RxPort->LogicVideoTx */
        else
            TxConnectStyle = ROUTE_4_R_V;
    }
    /* 3. LogicRxPort - TxPort connection */
    else if((RxPort->type == LogicVideoRx) && (TxPort->type == HdmiTx))
    {
        /* 3.1 UDP mode */
        if((ParallelConfigTable[RxPort->content.lvrx->Config*3 + 2] & 0x20) != 0)
            TxConnectStyle = ROUTE_7_V_T;
        /* 3.2 YCbCr 422 Input */
        if((ParallelConfigTable[RxPort->content.lvrx->Config*3 + 1] & 0x04) != 0)
        {
            /* 3.2.2 Direct connection between RxPort->LogicVideoTx with no CSC */
            TxConnectStyle = ROUTE_7_V_T;
        }
        /* 3.3 Direct LogicVideoRx->TxPort */
        else
            TxConnectStyle = ROUTE_7_V_T;
        /* 3.x When input is not locked, keep default routing */
        if(RxPort->content.rx->IsInputStable == 0)
            TxConnectStyle = ROUTE_7_V_T;
    }
    else
    {
        return ROUTE_NO_CONNECT;
    }

    return TxConnectStyle;
}

RouteStat AvPortRoutingMap(AvPort *RxPort, AvPort *TxPort, AvPort *ScalerPort, AvPort *ColorPort)
{

    RouteStat TxCurrentStyle = ROUTE_NO_CONNECT;
    if(TxPort->type == HdmiTx)
    {
        if(TxPort->content.RouteVideoFromPort == (struct AvPort*)RxPort)
        {
            if(RxPort->type == HdmiRx)
                TxCurrentStyle = ROUTE_1_R_T;
            else if(RxPort->type == LogicVideoRx)
                TxCurrentStyle = ROUTE_7_V_T;
        }
        else if(TxPort->content.RouteVideoFromPort == (struct AvPort*)ColorPort)
        {
            if(ColorPort->content.RouteVideoFromPort == (struct AvPort*)RxPort)
            {
                if(RxPort->type == HdmiRx)
                    TxCurrentStyle = ROUTE_2_R_C_T;
                else if(RxPort->type == LogicVideoRx)
                    TxCurrentStyle = ROUTE_8_V_C_T;
            }
        }
        else if(TxPort->content.RouteVideoFromPort == (struct AvPort*)ScalerPort)
        {
            if(ScalerPort->content.RouteVideoFromPort == (struct AvPort*)RxPort)
            {
                if(RxPort->type == HdmiRx)
                    TxCurrentStyle = ROUTE_3_R_S_T;
                else if(RxPort->type == LogicVideoRx)
                    TxCurrentStyle = ROUTE_9_V_S_T;
            }
        }
    }
    else if(TxPort->type == LogicVideoTx)
    {
        if(TxPort->content.RouteVideoFromPort == (struct AvPort*)RxPort)
            TxCurrentStyle = ROUTE_4_R_V;
        else if(TxPort->content.RouteVideoFromPort == (struct AvPort*)ColorPort)
        {
            if(ColorPort->content.RouteVideoFromPort == (struct AvPort*)RxPort)
                TxCurrentStyle = ROUTE_5_R_C_V;
        }
        else if(TxPort->content.RouteVideoFromPort == (struct AvPort*)ScalerPort)
        {
            if(ScalerPort->content.RouteVideoFromPort == (struct AvPort*)RxPort)
                TxCurrentStyle = ROUTE_6_R_S_V;
        }
    }
    return TxCurrentStyle;
}

void AvPortSetRouting(AvPort *RxPort, AvPort *TxPort, AvPort *ColorPort, AvPort *ScalerPort, uint8 TxConnectStyle)
{
    uint8 MessageFlag = 0;
    switch(TxConnectStyle)
    {
        case ROUTE_1_R_T:
        case ROUTE_4_R_V:
        case ROUTE_7_V_T:
            if(TxPort->content.RouteVideoFromPort != (struct AvPort*)RxPort)
            {
                MessageFlag = 1;
                AvApiConnectPort(RxPort,    TxPort,     AvConnectVideo);
            }
            if(MessageFlag == 1)
            {
                if(TxConnectStyle == ROUTE_1_R_T)
                    AvKapiOutputDebugMessage("New Route: Rx%d-Tx%d",RxPort->index+1, TxPort->index-3);
                else if(TxConnectStyle == ROUTE_4_R_V)
                    AvKapiOutputDebugMessage("New Route: Rx%d-LogicVideoTx",RxPort->index+1);
                else
                    AvKapiOutputDebugMessage("New Route: LogicVideoRx-Tx%d",TxPort->index-3);
            }
            break;
        case ROUTE_2_R_C_T:
        case ROUTE_5_R_C_V:
        case ROUTE_8_V_C_T:
            if(ColorPort->content.RouteVideoFromPort != (struct AvPort*)RxPort)
            {
                MessageFlag = 1;
                AvApiConnectPort(RxPort,    ColorPort,  AvConnectVideo);
            }
            if(TxPort->content.RouteVideoFromPort != (struct AvPort*)ColorPort)
            {
                MessageFlag = 1;
                AvApiConnectPort(ColorPort, TxPort,     AvConnectVideo);
            }
            if(MessageFlag == 1)
            {
                if(TxConnectStyle == ROUTE_2_R_C_T)
                    AvKapiOutputDebugMessage("New Route: Rx%d-Color-Tx%d",RxPort->index+1, TxPort->index-3);
                else if(TxConnectStyle == ROUTE_5_R_C_V)
                    AvKapiOutputDebugMessage("New Route: Rx%d-Color-LogicVideoTx",RxPort->index+1);
                else
                    AvKapiOutputDebugMessage("New Route: LogicVideoRx-Color-Tx%d",TxPort->index-3);
            }
            break;
        case ROUTE_3_R_S_T:
        case ROUTE_6_R_S_V:
        case ROUTE_9_V_S_T:
            if(ScalerPort->content.RouteVideoFromPort != (struct AvPort*)RxPort)
            {
                MessageFlag = 1;
                AvApiConnectPort(RxPort,    ScalerPort, AvConnectVideo);
            }
            if(TxPort->content.RouteVideoFromPort != (struct AvPort*)ScalerPort)
            {
                MessageFlag = 1;
                AvApiConnectPort(ScalerPort, TxPort,     AvConnectVideo);
            }
            if(MessageFlag == 1)
            {
                if(TxConnectStyle == ROUTE_3_R_S_T)
                    AvKapiOutputDebugMessage("New Route: Rx%d-Scaler-Tx%d",RxPort->index+1, TxPort->index-3);
                else if(TxConnectStyle == ROUTE_6_R_S_V)
                    AvKapiOutputDebugMessage("New Route: Rx%d-Scaler-LogicVideoTx",RxPort->index+1);
                else
                    AvKapiOutputDebugMessage("New Route: LogicVideoRx-Scaler-Tx%d", TxPort->index-3);
            }
            break;
    }
}

AvRet AvPortConnectUpdate(AvDevice *Device)
{
    AvPort *TxPort;
    AvPort *RxPort;
    AvPort *VideoTxPort;
    AvPort *AudioTxPort;
    AvPort *VideoRxPort;
    AvPort *AudioRxPort;
    AvPort *ColorPort;
    AvPort *ScalerPort;
    /* 1-bypass, 2-color, 3-scale, 4-color-scale */
    volatile RouteStat TxConnectStyle = ROUTE_NO_CONNECT;
    /* 1-bypass, 2-color, 3-scale, 4-color-scale */
    volatile RouteStat TxCurrentStyle = ROUTE_NO_CONNECT;

    /* 0. only process Gsv2k11 device */
    if(Device->type != Gsv2k11)
        return AvNotSupport;

    /* Decide the vsp management */
    /* 1. Prepare the ports */
    RxPort      = (AvPort*)Device->port;
    TxPort      = &RxPort[1];
    VideoTxPort = &RxPort[2];
    AudioTxPort = &RxPort[3];
    VideoRxPort = &RxPort[7];
    AudioRxPort = &RxPort[8];
    ScalerPort  = &RxPort[4];
    ColorPort   = &RxPort[5];
    /* 1.1 Find Valid RxPort */
    /* 1.1.1 Find Current Rx Input Selection */
    if(KfunFindVideoRxFront(TxPort, &RxPort) == AvOk)
    {
        if(RxPort->type == HdmiRx)
        {
            if(LogicOutputSel == 0)
            {
                RxPort = VideoRxPort;
                AvApiConnectPort(VideoRxPort, TxPort, AvConnectVideo);
                AvApiConnectPort(AudioRxPort, TxPort, AvConnectAudio);
                VideoRxPort->content.lvrx->Update     = 1;
            }
        }
        else if(RxPort->type == LogicVideoRx)
        {
            if(LogicOutputSel == 1)
            {
                RxPort = (AvPort*)Device->port;
                AvApiConnectPort(RxPort, TxPort,      AvConnectAV);
                AvApiConnectPort(RxPort, VideoTxPort, AvConnectVideo);
                AvApiConnectPort(RxPort, AudioTxPort, AvConnectAudio);
                VideoTxPort->content.lvtx->Update     = 1;
            }
        }
    }
    /* 1.1.2 Check RxA is valid or not */
    if(RxPort->content.rx->IsInputStable != 1)
        return AvOk;
    /* 2. Find Feasible Routing Solution */
    if(LogicOutputSel == 1)
    {
        /* 2.1 Find RxPort-VideoTxPort Current Routing Map */
        TxConnectStyle = AvPortRoutingPolicy(RxPort, VideoTxPort);
        TxCurrentStyle = AvPortRoutingMap(RxPort, VideoTxPort, ScalerPort, ColorPort);
        if((TxCurrentStyle != TxConnectStyle) && (TxConnectStyle != ROUTE_NO_CONNECT))
            AvPortSetRouting(RxPort, VideoTxPort, ColorPort, ScalerPort, TxConnectStyle);
        /* 2.2 Find RxPort-TxPort Routing Map */
        /*
        TxConnectStyle = AvPortRoutingPolicy(RxPort, TxPort);
        TxCurrentStyle = AvPortRoutingMap(RxPort, TxPort, ScalerPort, ColorPort);
        if((TxCurrentStyle != TxConnectStyle) && (TxConnectStyle != ROUTE_NO_CONNECT))
            AvPortSetRouting(RxPort, TxPort, ColorPort, ScalerPort, TxConnectStyle);
        */
    }
    else
    {
        /* 2.2 Find VideoRxPort-RxPort Current Routing Map */
        TxConnectStyle = AvPortRoutingPolicy(VideoRxPort, TxPort);
        TxCurrentStyle = AvPortRoutingMap(VideoRxPort, TxPort, ScalerPort, ColorPort);
        if((TxCurrentStyle != TxConnectStyle) && (TxConnectStyle != ROUTE_NO_CONNECT))
            AvPortSetRouting(VideoRxPort, TxPort, ColorPort, ScalerPort, TxConnectStyle);
    }

    return AvOk;
}
