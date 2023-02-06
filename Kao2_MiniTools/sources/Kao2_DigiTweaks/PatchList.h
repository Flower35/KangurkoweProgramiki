/**************************************************************/
#ifndef HEADER_KAO2_DIGITWEAKS_PATCHLIST
#define HEADER_KAO2_DIGITWEAKS_PATCHLIST

/**************************************************************/
#include "ReferableObjectList.h"
#include "Patch.h"

/**************************************************************/
typedef struct PatchListTag PatchList;

struct PatchListTag
{
    ReferableObjectList list;
};

/**************************************************************/
extern BOOL
PatchList_init(
    PatchList *self);

/**************************************************************/
extern void
PatchList_destroy(
    PatchList *self);

/**************************************************************/
extern BOOL
PatchList_append(
    PatchList *self,
    Patch *patchRef);

/**************************************************************/
extern BOOL
PatchList_appendRaw(
    PatchList *self,
    Patch_Data *patchParams);

/**************************************************************/
#endif  /* HEADER_KAO2_DIGITWEAKS_PATCHLIST */

/**************************************************************/
