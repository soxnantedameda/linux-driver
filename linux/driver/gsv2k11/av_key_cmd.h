#ifndef __av_key_cmd_h
#define __av_key_cmd_h

#include "av_config.h"

void ListenToKeyCommand(AvPort *port);
void RxInLedOut(uint8 enable);
void TxOutLedOut(uint8 index, uint8 enable);
void Edid2p0LedOut(uint8 enable);
void LogicLedOut(uint8 enable);

#endif
