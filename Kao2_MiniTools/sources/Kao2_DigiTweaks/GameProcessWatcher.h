/**************************************************************/
#ifndef HEADER_KAO2_DIGITWEAKS_GAMEPROCESSWATCHER
#define HEADER_KAO2_DIGITWEAKS_GAMEPROCESSWATCHER

/**************************************************************/
#include "Kao2_DigiTweaks.h"
#include "GameProcess.h"
#include "GameVerSigEx.h"

/**************************************************************/
extern GameProcess g_gameProcess;

/**************************************************************/
extern void
GameProcessWatcher_globalOpen(
    void);

/**************************************************************/
extern void
GameProcessWatcher_globalClose(
    void);

/**************************************************************/
extern BOOL
GameProcessWatcher_attachFirstMatching(
    void);

/**************************************************************/
extern BOOL
GameProcessWatcher_testSingleProcess(
    DWORD processId);

/**************************************************************/
extern BOOL
GameProcessWatcher_validateImageFileName(
    HANDLE dummyProcess);

/**************************************************************/
extern BOOL
GameProcessWatcher_getProcWSig(
    GameProcess **processResult,
    GameVerSigEx **signatureResult);

/**************************************************************/
#endif  /* HEADER_KAO2_DIGITWEAKS_GAMEPROCESSWATCHER */

/**************************************************************/
