/**************************************************************/
#ifndef HEADER_KAO2_DIGITWEAKS_GAMEVERSIG
#define HEADER_KAO2_DIGITWEAKS_GAMEVERSIG

/**************************************************************/
#include "Kao2_DigiTweaks.h"

/**************************************************************/
typedef struct GameVerSigTag GameVerSig;

struct GameVerSigTag
{
    uint32_t unixTimestamp;
    uint32_t entryPoint;
    uint32_t imageSize;
};

/**************************************************************/
extern BOOL
GameVerSig_compare(
    GameVerSig *self,
    GameVerSig *other);

/**************************************************************/
#endif  /* HEADER_KAO2_DIGITWEAKS_GAMEVERSIG */

/**************************************************************/
