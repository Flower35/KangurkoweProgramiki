/**************************************************************/
#include "ReferableObjectList.h"
#include "ErrorsQueue.h"

/**************************************************************/
BOOL
ReferableObjectList_init(
    ReferableObjectList *self)
{
    self->count = 0;

    self->maxAlloc = 16;

    self->itemRefs = malloc((self->maxAlloc) * sizeof(void *));

    if (NULL == (self->itemRefs))
    {
        ErrorsQueue_push(
            "List_init(): malloc failed...\n");

        return FALSE;
    }

    return TRUE;
}

/**************************************************************/
void
ReferableObjecttList_destroy(
    ReferableObjectList *self)
{
    if (NULL != (self->itemRefs))
    {
        ReferableObjecttList_clear(self);

        free(self->itemRefs);
    }
}

/**************************************************************/
void
ReferableObjecttList_clear(
    ReferableObjectList *self)
{
    ReferableObject *item;

    for (int i = 0; i < (self->count); i++)
    {
        item = self->itemRefs[i];

        ReferableObject_decRef(item);
    }

    self->count = 0;
}

/**************************************************************/
BOOL
ReferableObjectList_append(
    ReferableObjectList *self,
    ReferableObject *itemRef)
{
    int newCount;
    int newMaxAlloc;
    ReferableObject **newItemRefs;

    newCount = (self->count) + 1;

    if (newCount > (self->maxAlloc))
    {
        newMaxAlloc = 2 * (self->maxAlloc);

        newItemRefs = realloc(
            (self->itemRefs),
            (newMaxAlloc * sizeof(void *)));

        if (NULL == newItemRefs)
        {
            ErrorsQueue_push(
                "List_append(): realloc failed...\n");

            return FALSE;
        }

        self->maxAlloc = newMaxAlloc;
        self->itemRefs = newItemRefs;
    }

    self->itemRefs[self->count] = itemRef;
    ReferableObject_incRef(itemRef);

    self->count = newCount;
    return TRUE;
}

/**************************************************************/
