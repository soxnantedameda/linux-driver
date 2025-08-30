#include "av_key_cmd.h"
#include "av_config.h"
#include "uapi/hal.h"
#include "kapi/kapi.h"

extern uint8  EdidHdmi2p0;
extern uint8  LogicOutputSel;

#if AvEnableKeyInput /* Enable UART */

extern uchar AudioStatus;

void ProcessKey(AvPort *port);
#define FoundKeyPress(p) uChar&p

#endif /* Enable UART */

void ProcessKey(AvPort *port)
{
#if AvEnableKeyInput
    uint8 value = 0x00;
    uint8 uChar = 0x00;
    if(AvHalGetKey(&uChar) == AvNotAvailable)
        return;

    /* Logic Video/Audio Direction */
    value = (uChar>>0) & 0x01;
    if(value != 0x00)
    {
        LogicOutputSel = 1 - LogicOutputSel;
    }
    /* Logic LED */
    LogicLedOut(LogicOutputSel);
#endif
}

void ListenToKeyCommand(AvPort *port)
{
#if AvEnableKeyInput
    ProcessKey(port);
#endif
}

#if AvEnableKeyInput
void RxInLedOut(uint8 enable)
{
    if(enable == 1)
        HAL_GPIO_WritePin(GPIOA, LED0_Pin, GPIO_PIN_RESET);
    else
        HAL_GPIO_WritePin(GPIOA, LED0_Pin, GPIO_PIN_SET);
}

void TxOutLedOut(uint8 index, uint8 enable)
{
    if(enable == 1)
        HAL_GPIO_WritePin(GPIOA, LED1_Pin, GPIO_PIN_RESET);
    else
        HAL_GPIO_WritePin(GPIOA, LED1_Pin, GPIO_PIN_SET);
}

void LogicLedOut(uint8 enable)
{
    if(enable == 1)
        HAL_GPIO_WritePin(GPIOA, LED2_Pin, GPIO_PIN_RESET);
    else
        HAL_GPIO_WritePin(GPIOA, LED2_Pin, GPIO_PIN_SET);
}
#else
void RxInLedOut(uint8 enable)
{

}

void TxOutLedOut(uint8 index, uint8 enable)
{

}

void LogicLedOut(uint8 enable)
{

}
#endif
