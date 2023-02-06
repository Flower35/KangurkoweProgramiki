/**************************************************************/
#include "PatchExList.h"

/**************************************************************/
BOOL
PatchExList_init(
    PatchExList *self)
{
    BOOL test = ReferableObjectList_init(
        &(self->list));

    return test;
}

/**************************************************************/
void
PatchExList_destroy(
    PatchExList *self)
{
    ReferableObjecttList_destroy(
        &(self->list));
}

/**************************************************************/
BOOL
PatchExList_append(
    PatchExList *self,
    PatchEx *patchExRef)
{
    BOOL test = ReferableObjectList_append(
        &(self->list),
        (ReferableObject *) patchExRef);

    return test;
}

/**************************************************************/
BOOL
PatchExList_findByName(
    PatchExList *self,
    PatchEx **result,
    ReadOnlyUTF8String name)
{
    PatchEx *item;

    for (int i = 0; i < (self->list.count); i++)
    {
        item = (PatchEx *) (self->list.itemRefs[i]);

        if (0 == strcmp(name, item->data.name))
        {
            result[0] = item;
            PatchEx_incRef(item);

            return TRUE;
        }
    }

    return FALSE;
}

/**************************************************************/
