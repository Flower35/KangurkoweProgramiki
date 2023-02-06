/**************************************************************/
#ifndef HEADER_KAO2_DIGITWEAKS_KAO2OPERATIONS
#define HEADER_KAO2_DIGITWEAKS_KAO2OPERATIONS

/**************************************************************/
#include "Kao2_DigiTweaks.h"
#include "GameProcess.h"
#include "GameVerSigEx.h"
#include "TextObjectList.h"

/**************************************************************/
extern BOOL
Kao2_StringWrapper_read(
    UTF8String destination,
    int bufferSize,
    GameProcess *game,
    uint32_t stringWrapperAddr);

/**************************************************************/
extern BOOL
Kao2_StringWrapper_write(
    ReadOnlyUTF8String source,
    int sourceLength,
    GameProcess *game,
    uint32_t stringWrapperAddr);

/**************************************************************/
extern BOOL
Kao2_String_read(
    UTF8String destination,
    int bufferSize,
    GameProcess *game,
    uint32_t stringBaseAddr);

/**************************************************************/
extern BOOL
Kao2_String_write(
    ReadOnlyUTF8String source,
    int sourceLength,
    GameProcess *game,
    uint32_t stringBaseAddr);

/**************************************************************/
extern BOOL
Kao2_StringWrappersArray_read(
    TextObjectList *testList,
    GameProcess *game,
    uint32_t arrayAddr);

/**************************************************************/
extern BOOL
Kao2_Lang_readLangList(
    GameProcess *game,
    GameVerSigEx *sig);

/**************************************************************/
extern int
Kao2_Lang_getSubsLang(
    GameProcess *game,
    GameVerSigEx *sig);

/**************************************************************/
extern int
Kao2_Lang_getVoicesLang(
    GameProcess *game,
    GameVerSigEx *sig);

/**************************************************************/
extern BOOL
Kao2_Lang_setSubsLang(
    int idx,
    GameProcess *game,
    GameVerSigEx *sig);

/**************************************************************/
extern BOOL
Kao2_Lang_setVoicesLang(
    int idx,
    GameProcess *game,
    GameVerSigEx *sig);

/**************************************************************/
#endif  /* HEADER_KAO2_DIGITWEAKS_KAO2OPERATIONS */

/**************************************************************/
