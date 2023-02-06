/**************************************************************/
#include "PatchEx.h"

/**************************************************************/
ReferableObject_VirtualMethods
g_PatchEx_VirtualMethods =
{
    (void (*)(void *)) PatchEx_destroy
};

/**************************************************************/
BOOL
PatchEx_init(
    PatchEx *self,
    ReadOnlyUTF8String name)
{
    BOOL test;

    /* Parent initialization */

    ReferableObject_init((void *) self);

    /* Self initialization */

    self->vptr = &(g_PatchEx_VirtualMethods);

    self->data.name = name;

    test = PatchList_init(&(self->data.patches));

    return test;
}

/**************************************************************/
void
PatchEx_destroy(
    PatchEx *self)
{
    PatchList_destroy(&(self->data.patches));
}

/**************************************************************/
PatchEx *
PatchEx_createOnHeap(
    ReadOnlyUTF8String name)
{
    BOOL test;

    PatchEx *self = malloc(sizeof(PatchEx));

    if (NULL == self)
    {
        return NULL;
    }

    test = PatchEx_init(self, name);

    if (FALSE == test)
    {
        PatchEx_destroy(self);
        free(self);

        return NULL;
    }

    PatchEx_incRef(self);

    return self;
}

/**************************************************************/
void
PatchEx_incRef(
    PatchEx *self)
{
    ReferableObject_incRef((void *) self);
}

/**************************************************************/
void
PatchEx_decRef(
    PatchEx *self)
{
    ReferableObject_decRef((void *) self);
}

/**************************************************************/
BOOL
PatchEx_registerSubpatch(
    PatchEx *self,
    Patch_Data *patchParams)
{
    BOOL test = PatchList_appendRaw(
        &(self->data.patches),
        patchParams);

    return test;
}

/**************************************************************/
BOOL
PatchEx_tryOut(
    PatchEx *self,
    BOOL applyElseRestore,
    GameProcess *game)
{
    BOOL test;

    for (int i = 0; i < (self->data.patches.list.count); i++)
    {
        test = Patch_tryOut(
            (Patch *) (self->data.patches.list.itemRefs[i]),
            applyElseRestore,
            game);

        if (FALSE == test)
        {
            return FALSE;
        }
    }

    return TRUE;
}

/**************************************************************/
