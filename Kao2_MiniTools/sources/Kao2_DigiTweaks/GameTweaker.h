/**************************************************************/
#ifndef HEADER_KAO2_DIGITWEAKS_GAMETWEAKER
#define HEADER_KAO2_DIGITWEAKS_GAMETWEAKER

/**************************************************************/
#include "Kao2_DigiTweaks.h"
#include "GameProcess.h"
#include "PatchExList.h"
#include "TextObjectList.h"

/**************************************************************/
extern PatchExList g_patches;
extern TextObjectList g_languages;

/**************************************************************/
extern BOOL
GameTweaker_globalOpen(
    void);

/**************************************************************/
extern void
GameTweaker_globalClose(
    void);

/**************************************************************/
extern BOOL
GameTweaker_loadPatches(
    void);

/**************************************************************/
extern BOOL
GameTweaker_loadLanguages(
    void);

/**************************************************************/
extern void const *
GameTweaker_guiCallback(
    uint32_t command);

/**************************************************************/
#endif  /* HEADER_KAO2_DIGITWEAKS_GAMETWEAKER */

/**************************************************************/
