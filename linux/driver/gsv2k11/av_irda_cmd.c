#include "av_config.h"
#include "uapi/hal.h"
#include "kapi/kapi.h"
#include "av_irda_cmd.h"

void ProcessIrda(AvPort *port)
{
    return;
}
void ListenToIrdaCommand(AvPort *port)
{
#if AvIrdaFunctionInput
    ProcessIrda(port);
#endif
}
