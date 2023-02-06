/**************************************************************/
#ifndef HEADER_KAO2_DIGITWEAKS_PATCHEXLIST
#define HEADER_KAO2_DIGITWEAKS_PATCHEXLIST

/**************************************************************/
#include "ReferableObjectList.h"
#include "PatchEx.h"

/**************************************************************/
typedef struct PatchExListTag PatchExList;

struct PatchExListTag
{
    ReferableObjectList list;
};

/**************************************************************/
extern BOOL
PatchExList_init(
    PatchExList *self);

/**************************************************************/
extern void
PatchExList_destroy(
    PatchExList *self);

/**************************************************************/
extern BOOL
PatchExList_append(
    PatchExList *self,
    PatchEx *patchExRef);

/**************************************************************/
extern BOOL
PatchExList_findByName(
    PatchExList *self,
    PatchEx **result,
    ReadOnlyUTF8String name);

/**************************************************************/
#endif  /* HEADER_KAO2_DIGITWEAKS_PATCHEXLIST */

/**************************************************************/
