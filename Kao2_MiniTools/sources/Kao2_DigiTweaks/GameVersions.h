/**************************************************************/
#ifndef HEADER_KAO2_DIGITWEAKS_GAMEVERSIONS
#define HEADER_KAO2_DIGITWEAKS_GAMEVERSIONS

/**************************************************************/
#include "Kao2_DigiTweaks.h"
#include "GameVersList.h"

/**************************************************************/
extern GameVersList g_gameVersList;

/**************************************************************/
extern BOOL
GameVersions_globalOpen(
    void);

/**************************************************************/
extern void
GameVersions_globalClose(
    void);

/**************************************************************/
extern int
GameVersions_findMatchingVersion(
    GameVerSig *dummySignature);

/**************************************************************/
extern BOOL
GameVersions_getSignatureByIdx(
    GameVerSigEx **result,
    int idx);

/**************************************************************/
#endif  /* HEADER_KAO2_DIGITWEAKS_GAMEVERSIONS */

/**************************************************************/
