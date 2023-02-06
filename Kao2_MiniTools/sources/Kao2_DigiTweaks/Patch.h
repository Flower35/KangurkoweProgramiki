/**************************************************************/
#ifndef HEADER_KAO2_DIGITWEAKS_PATCH
#define HEADER_KAO2_DIGITWEAKS_PATCH

/**************************************************************/
#include "ReferableObject.h"
#include "GameProcess.h"

/**************************************************************/
#define MAX_PATCH_SIZE 64

/**************************************************************/
extern ReferableObject_VirtualMethods
g_Patch_VirtualMethods;

/**************************************************************/
typedef struct Patch_DataTag Patch_Data;

struct Patch_DataTag
{
    ReferableObject_Data _super;

    uint32_t relAddr;
    int patchSize;
    uint8_t *oldBytes;
    uint8_t *newBytes;
};

/**************************************************************/
typedef struct PatchTag Patch;

struct PatchTag
{
    ReferableObject_VirtualMethods *vptr;
    Patch_Data data;
};

/**************************************************************/
extern BOOL
Patch_init(
    Patch *self,
    Patch_Data *params);

/**************************************************************/
extern void
Patch_destroy(
    Patch *self);

/**************************************************************/
extern Patch *
Patch_createOnHeap(
    Patch_Data *params);

/**************************************************************/
extern void
Patch_incRef(
    Patch *self);

/**************************************************************/
extern void
Patch_decRef(
    Patch *self);

/**************************************************************/
extern BOOL
Patch_tryOut(
    Patch *self,
    BOOL applyElseRestore,
    GameProcess *game);

/**************************************************************/
#endif  /* HEADER_KAO2_DIGITWEAKS_PATCH */

/**************************************************************/
