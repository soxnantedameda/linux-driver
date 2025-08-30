#include "av_config.h"
#include "uapi/hal.h"
#include "kapi/kapi.h"
#include "av_uart_cmd.h"

#define CommandNameIs(data) (!AvStrcmp((const char*) cCommand, data))
#define UartCommandLength 8

#if AvEnableUartInput /* Enable UART */
void ProcessChar(AvPort *port);
void DoUartCommand(AvPort *port);
void ReturnIdle(void);

typedef enum
{
  P_IDLE=0,
  P_CMD,
  P_ARG1,
  P_ARG2,
  P_ARG3,
  P_REP
}ParseState;

extern uchar AudioStatus;

static ParseState eParser = P_IDLE;
static uint8 cCommand[UartCommandLength];
static uint8 uCmdIndex;
static uint8 cArg1[4];
static uint8 uArg1Index;
static uint8 cArg2[4];
static uint8 uArg2Index;
static uint8 cArg3[4];
static uint8 uArg3Index;
static uint32 uArg1;
static uint32 uArg2;
static uint32 uArg3;

static uint8 RegTx1From = 1;
static uint8 RegTx2From = 2;
static uint8 RegPause   = 0;

#endif /* Enable UART */

void ProcessChar(AvPort *port)
{
#if AvEnableUartInput
    uint8 uChar = 0x00;
    uint8 uReturn = 0x0a;
    /* Get Uart Data, if no data, return */
    if(AvHalUartGetByte(&uChar) == AvError)
        return;

    AvHalUartSendByte(&uChar, 1);
    if(uChar == 0x0d)
        AvHalUartSendByte(&uReturn, 1);

    switch (eParser)
    {
        case P_IDLE:
            //AvKapiOutputDebugMessage("\n\rKeyIn List:\n\r's'-Current Connection");
            //AvKapiOutputDebugMessage("'c * *'-Connect Rx* to Tx*, e.g. 'c 4 1'");
            uChar = IsLetter(uChar);
            if (uChar)
            {
                cCommand[uCmdIndex] = uChar;
                uCmdIndex++;
                eParser = P_CMD;
            }
            break;

        case P_CMD:
            if (IsCR(uChar)) //receive end char 0x0D
            {
                cCommand[uCmdIndex] = 0;
                uCmdIndex++;
                DoUartCommand(port); //process data
                ReturnIdle();
            }
            else if (IsSpace(uChar)) //receive space key
            {
                cCommand[uCmdIndex] = 0;
                uCmdIndex++;
                if CommandNameIs("REP")
                    eParser = P_REP;
                else
                    eParser = P_ARG1;
            }
            else if (IsBackSpace(uChar))
            {
                if (uCmdIndex>0)
                    uCmdIndex--;
            }
            else
            {
                uChar = IsLetter(uChar);
                if (uChar)
                {
                    cCommand[uCmdIndex] = uChar;
                    uCmdIndex++;
                }
            }
            break;

        case P_ARG1:
            if (IsCR(uChar))
            {
                cArg1[uArg1Index] = 0;
                uArg1 = AsciiToNumber(cArg1, uArg1Index);
                DoUartCommand(port);
                ReturnIdle();
            }
            else if (IsSpace(uChar))
            {
                cArg1[uArg1Index] = 0;
                uArg1 = AsciiToNumber(cArg1, uArg1Index);
                eParser = P_ARG2;
            }
            else if (IsBackSpace(uChar))
            {
                if (uArg1Index>0) uArg1Index--;
                else eParser = P_CMD;
            }
            else
            {
                uChar = IsLetter(uChar);
                if (uChar)
                {
                    cArg1[uArg1Index] = uChar;
                    uArg1Index++;
                }
            }
            break;

        case P_ARG2:
            if (IsCR(uChar))
            {
                cArg2[uArg2Index] = 0;
                uArg2 = AsciiToNumber(cArg2, uArg2Index);
                DoUartCommand(port);
                ReturnIdle();
            }
            else if (IsSpace(uChar))
            {
                cArg2[uArg2Index] = 0;
                uArg2 = AsciiToNumber(cArg2, uArg2Index);
                eParser = P_ARG3;
            }
            else if (IsBackSpace(uChar))
            {
                if (uArg2Index>0) uArg2Index--;
                else eParser = P_ARG1;
            }
            else
            {
                uChar = IsLetter(uChar);
                if (uChar)
                {
                    cArg2[uArg2Index] = uChar;
                    uArg2Index++;
                }
            }
            break;

        case P_ARG3:
            if (IsCR(uChar) || IsSpace(uChar))
            {
                cArg3[uArg3Index] = 0;
                uArg3 = AsciiToNumber(cArg3, uArg3Index);
                DoUartCommand(port);
                ReturnIdle();
            }
            else if (IsBackSpace(uChar))
            {
                if (uArg3Index>0) uArg3Index--;
                else eParser = P_ARG2;
            }
            else
            {
                uChar = IsLetter(uChar);
                if (uChar)
                {
                    cArg3[uArg3Index] = uChar;
                    uArg3Index++;
                }
            }
            break;

        case P_REP:
            if (IsCR(uChar))
            {
                cArg1[uArg1Index++] = '\r';
                cArg1[uArg1Index] = 0;
                DoUartCommand(port);
            }
            else if (IsSpace(uChar))
            {
                cArg1[uArg1Index] = ' ';
                uArg1Index++;

            }
            else if (IsBackSpace(uChar))
            {
                if (uArg1Index>0) uArg1Index--;
                else eParser = P_CMD;
            }
            else
            {
                uChar = IsLetter(uChar);
                if (uChar)
                {
                    cArg1[uArg1Index] = uChar;
                    uArg1Index++;
                }
            }
            break;
    }
#endif
}

void ReturnIdle(void)
{
#if AvEnableUartInput
    uint8 i;
    eParser = P_IDLE;
    uCmdIndex = 0;
    uArg1Index = 0;
    uArg2Index = 0;
    uArg3Index = 0;
    uArg1 = 0;
    uArg2 = 0;
    uArg3 = 0;
    for(i=0;i<4;i++)
        cArg1[i]=0;
    for(i=0;i<4;i++)
        cArg2[i]=0;
    for(i=0;i<4;i++)
        cArg3[i]=0;

#endif
}

void DoUartCommand(AvPort *port)
{
#if AvEnableUartInput
    if CommandNameIs("A") /* ARC */
    {
        #if AvEnableCecFeature
        if(AudioStatus == 0)
        {
            AudioStatus = 1;
            if((&port[4])->content.cec->EnableAudioAmplifier != AV_CEC_AMP_ENABLED)
                (&port[4])->content.cec->EnableAudioAmplifier = AV_CEC_AMP_TO_ENABLE;
            if((&port[4])->content.cec->EnableARC != AV_CEC_ARC_INITIATED)
                (&port[4])->content.cec->EnableARC = AV_CEC_ARC_TO_INITIATE;
            AvKapiOutputDebugMessage("Software try to enable ARC");
        }
        else
        {
            AudioStatus = 0;
            if((&port[4])->content.cec->EnableAudioAmplifier != AV_CEC_AMP_DISABLED)
                (&port[4])->content.cec->EnableAudioAmplifier = AV_CEC_AMP_TO_DISABLE;
            AvKapiOutputDebugMessage("Software try to disable ARC");
        }
        #endif
        return;
    }
    if CommandNameIs("S") /* STATUS */
    {
        AvKapiOutputDebugMessage("Current Routing Rx%x=>Tx1, Rx%x=>Tx2", RegTx1From, RegTx2From);
        return;
    }
    else if CommandNameIs("P") /* PAUSE */
    {
        RegPause = 1;
        AvKapiOutputDebugMessage("Software Paused");
        return;
    }
    else if CommandNameIs("G") /* GO ON */
    {
        RegPause = 0;
        AvKapiOutputDebugMessage("Software Continued");
        return;
    }
    else if CommandNameIs("E") /* EXTRACT AUDIO */
    {
        if((uArg1 >= 1) && (uArg1 <= 4)) /* Rx Output Audio to AP */
        {
            if((uArg2 >= 1) && (uArg2 <= 2)) /* AP port is correct */
            {
                if((uArg1 == RegTx1From) || (uArg1 == RegTx2From))
                {
                    AvApiConnectPort(&port[uArg1-1], &port[uArg2+7], AvConnectAudio);
                }
                else
                    AvKapiOutputDebugMessage("Rx must connect to Tx before audio extraction! ReTry!");
            }
            else
                AvKapiOutputDebugMessage("Invalid AP port! ReTry!");
        }
        else
        {
            AvKapiOutputDebugMessage("Invalid Rx! ReTry!");
        }
    }
    else if CommandNameIs("I") /* INSERT AUDIO */
    {
        if((uArg1 >= 1) && (uArg1 <= 2)) /* Input Audio from AP */
        {
            if((uArg2 >= 1) && (uArg2 <= 2)) /* Tx port is correct */
                AvApiConnectPort(&port[uArg1+5], &port[uArg2+3], AvConnectAudio);
            else
                AvKapiOutputDebugMessage("Invalid Tx port! ReTry!");
        }
        else
        {
            AvKapiOutputDebugMessage("Invalid AP! ReTry!");
        }
    }
    else if CommandNameIs("C") /* CONNECT HDMI PORT */
    {
        if((uArg1 > 4) || (uArg1 <1))
            AvKapiOutputDebugMessage("Invalid Rx! ReTry!");
        else if(uArg3 != 0)
            AvKapiOutputDebugMessage("Don't input 3rd argument!");
        else
        {
            if(uArg1 <= 2)
            {
                AvApiConnectPort(&port[uArg1-1], &port[2], AvConnectAV);
                AvApiConnectPort(&port[uArg1-1], &port[3], AvConnectAV);
                RegTx1From = uArg1;
                AvKapiOutputDebugMessage("Current Routing Rx%x=>Tx1, Rx%x=>Tx2", RegTx1From, RegTx1From);
            }
            else
            {
                AvKapiOutputDebugMessage("Invalid Tx! Retry!");
            }
        }
    }
    else if CommandNameIs("R") /* Report Integrity */
    {
        /* Reset System */
        HAL_NVIC_SystemReset();
    }

#endif
}

void ListenToUartCommand(AvPort *port)
{
#if AvEnableUartInput
    ProcessChar(port);
    while(RegPause == 1)
        ProcessChar(port);

#endif
}
