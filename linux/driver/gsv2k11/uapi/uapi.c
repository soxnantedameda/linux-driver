/**
 * @file uapi.c
 *
 * @brief low level ports related universal api functions
 */
#include <linux/slab.h>

#include "uapi.h"

/* firmware static buffer */
static uint8 AvGlobalStaticBuffer[AvGlobalStaticBufferSize];

/* hal functions variables */
AvFpI2cRead        AvHookI2cRead;
AvFpI2cWrite       AvHookI2cWrite;
AvFpUartSendByte   AvHookUartTxByte;
AvFpUartGetByte    AvHookUartRxByte;
AvFpGetMilliSecond AvHookGetMilliSecond;
AvFpGetKey         AvHookGetKey;
AvFpGetIrda        AvHookGetIrda;
/**
 * @brief  univeral layer inialization function
 * @return AvOk - success
 */
uapi AvRet AvUapiInit()
{
    /* init hardware access function pointer */
    AvHookI2cRead = NULL;
    AvHookI2cWrite = NULL;
    AvHookUartTxByte = NULL;
    AvHookUartRxByte = NULL;
    AvHookGetMilliSecond = NULL;
    AvHookGetKey = NULL;
    AvHookGetIrda = NULL;
    return AvOk;
}

/**
 * @brief  hookup user's hardware access functions
 * @return AvOk - success
 */
uapi AvRet AvUapiHookBspFunctions(pin AvFpI2cRead i2cRd, pin AvFpI2cWrite i2cWr, pin AvFpUartSendByte uartTxB, pin AvFpUartGetByte uartRxB, pin AvFpGetMilliSecond getMs, pin AvFpGetKey getKey,pin AvFpGetIrda getIrda)
{
    AvRet ret = AvOk;
    AvHookI2cRead = i2cRd;
    AvHookI2cWrite = i2cWr;
    AvHookUartTxByte = uartTxB;
    AvHookUartRxByte = uartRxB;
    AvHookGetMilliSecond = getMs;
    AvHookGetKey = getKey;
    AvHookGetIrda = getIrda;
    AvUapiOutputDebugMessage(" ");
    AvUapiOutputDebugMessage("-------------------------------------------------------------------");
    AvUapiOutputDebugMessage("    |> Audio/Video Software %s", AvVersion);
    AvUapiOutputDebugMessage("-------------------------------------------------------------------");
    return ret;
}

/**
 * @brief  output debug message
 * @return AvOk - success
 */
uapi AvRet AvUapiOuputDbgMsg(pin schar *stringToFormat, ...)
{
    AvRet ret = AvOk;
#if AvEnableDebugFsm
    /*
    va_list ap;
    uint8 formattedString[128];
    int32 chars;

    va_start(ap, stringToFormat);
    chars = vsprintf((schar*)formattedString, stringToFormat, ap);
    va_end(ap);

    formattedString[chars+0] = 0x0d;
    formattedString[chars+1] = 0x0a;

    AvUartTxByte(formattedString, chars+2);
    */
#endif
    /*
    if (AvUartTxByte != NULL)
    {
        for(i = 0; i < chars; i ++)
            AvUartTxByte((uint8)formattedString[i]);
    }
    AvUartTxByte(10);
    AvUartTxByte(13);
    */
    return ret;
}

/**
 * @brief  output debug message
 * @return AvOk - success
 */
uapi AvRet AvUapiUartInput(pin schar *stringToFormat, ...)
{
#if AvEnableUartInput
    va_list ap;
    uint8 formattedString[128]; /* max 128 chars in a string */
    int32 chars;
    AvRet ret = AvOk;

    va_start(ap, stringToFormat);
    chars = vsprintf((schar*)formattedString,stringToFormat, ap);
    va_end(ap);

    formattedString[chars+0] = 0x0a;
    formattedString[chars+1] = 0x0d;

    AvUartTxByte(formattedString, chars+2);

    /*
    if (AvUartTxByte != NULL)
    {
        for(i = 0; i < chars; i ++)
            AvUartTxByte((uint8)formattedString[i]);
    }
    AvUartTxByte(10);
    AvUartTxByte(13);
    */
    return ret;
#else
    return AvOk;
#endif
}

/**
 * @brief  allocate memory from global static buffer
 * @param  bytes memory size in bytes
 * @return AvOk - success
 */
uapi AvRet AvUapiAllocateMemory(pin uint32 bytes, pout uint64 *bufferAddress)
{
    AvRet ret = AvOk;
    static uint32 currentPoint = 0;
    uint32 wordbytes = bytes & 0xfffffffc;

    if ((currentPoint + bytes) < AvGlobalStaticBufferSize)
    {
        *bufferAddress = (uint64)(&AvGlobalStaticBuffer[currentPoint]);
        if(bytes != wordbytes)
            bytes = wordbytes + 8;
        currentPoint = currentPoint + bytes;
    }
    else
    {
        AvUapiOutputDebugMessage("ERROR: global static buffer exhausted.");
        ret = AvNotAvailable;
    }
    return ret;
}

uapi AvRet AvUapiRxReadInfo(pin AvPort *port)
{
    return AvOk;
}

uapi AvRet AvUapiRxReadStdi(pin AvPort *port)
{
    return AvOk;
}

uapi AvRet AvUapiTxDetectMode(pin AvPort *port)
{
    return AvOk;
}
