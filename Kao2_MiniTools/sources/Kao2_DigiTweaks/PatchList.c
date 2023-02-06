/**************************************************************/
#include "PatchList.h"

/**************************************************************/
BOOL
PatchList_init(
    PatchList *self)
{
    BOOL test = ReferableObjectList_init(
        &(self->list));

    return test;
}

/**************************************************************/
void
PatchList_destroy(
    PatchList *self)
{
    ReferableObjecttList_destroy(
        &(self->list));
}

/**************************************************************/
BOOL
PatchList_append(
    PatchList *self,
    Patch *patchRef)
{
    BOOL test = ReferableObjectList_append(
        &(self->list),
        (ReferableObject *) patchRef);

    return test;
}

/**************************************************************/
BOOL
PatchList_appendRaw(
    PatchList *self,
    Patch_Data *patchParams)
{
    BOOL test;
    Patch *patchRef = Patch_createOnHeap(patchParams);

    if (NULL == patchRef)
    {
        return FALSE;
    }

    test = PatchList_append(self, patchRef);

    Patch_decRef(patchRef);

    return test;
}

/**************************************************************/
