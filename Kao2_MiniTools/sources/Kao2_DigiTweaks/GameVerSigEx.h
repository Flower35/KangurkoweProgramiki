/**************************************************************/
#ifndef HEADER_KAO2_DIGITWEAKS_GAMEVERSIGEX
#define HEADER_KAO2_DIGITWEAKS_GAMEVERSIGEX

/**************************************************************/
#include "Kao2_DigiTweaks.h"
#include "TextHelper.h"
#include "GameVerSig.h"

/**************************************************************/
typedef struct GameVerSigExTag GameVerSigEx;

struct GameVerSigExTag
{
    BOOL supported;
    ReadOnlyUTF8String name;
    GameVerSig signature;
    uint32_t gameletAddr;
    uint32_t gameletVptr;
    uint32_t langAddr;
    uint32_t langVptr;
};

/**************************************************************/
#endif  /* HEADER_KAO2_DIGITWEAKS_GAMEVERSIGEX */

/**************************************************************/
