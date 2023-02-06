/**************************************************************/
#ifndef HEADER_KAO2_DIGITWEAKS_PATCHEX
#define HEADER_KAO2_DIGITWEAKS_PATCHEX

/**************************************************************/
#include "ReferableObject.h"
#include "PatchList.h"
#include "TextHelper.h"
#include "GameProcess.h"

/**************************************************************/
extern ReferableObject_VirtualMethods
g_PatchEx_VirtualMethods;

/**************************************************************/
typedef struct PatchEx_DataTag PatchEx_Data;

struct PatchEx_DataTag
{
    ReferableObject_Data _super;

    ReadOnlyUTF8String name;
    PatchList patches;
};

/**************************************************************/
typedef struct PatchExTag PatchEx;

struct PatchExTag
{
    ReferableObject_VirtualMethods *vptr;
    PatchEx_Data data;
};

/**************************************************************/
extern BOOL
PatchEx_init(
    PatchEx *self,
    ReadOnlyUTF8String name);

/**************************************************************/
extern void
PatchEx_destroy(
    PatchEx *self);

/**************************************************************/
extern PatchEx *
PatchEx_createOnHeap(
    ReadOnlyUTF8String name);

/**************************************************************/
extern void
PatchEx_incRef(
    PatchEx *self);

/**************************************************************/
extern void
PatchEx_decRef(
    PatchEx *self);

/**************************************************************/
extern BOOL
PatchEx_registerSubpatch(
    PatchEx *self,
    Patch_Data *patchParams);

/**************************************************************/
extern BOOL
PatchEx_tryOut(
    PatchEx *self,
    BOOL applyElseRestore,
    GameProcess *game);

/**************************************************************/
#endif  /* HEADER_KAO2_DIGITWEAKS_PATCHEX */

/**************************************************************/
