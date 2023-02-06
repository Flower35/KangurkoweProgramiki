/**************************************************************/
#include "GameVersList.h"
#include "ErrorsQueue.h"

/**************************************************************/
BOOL
GameVersList_init(
    GameVersList *self)
{
    self->count = 0;

    self->allocated = 16;

    self->signatures = malloc(
        (self->allocated) * sizeof(GameVerSigEx));

    if (NULL == (self->signatures))
    {
        ErrorsQueue_push(
            "GameVersList_init(): malloc failed...\n");

        return FALSE;
    }

    return TRUE;
}

/**************************************************************/
void
GameVersList_destroy(
    GameVersList *self)
{
    if (NULL != (self->signatures))
    {
        free(self->signatures);
    }
}

/**************************************************************/
BOOL
GameVersList_register(
    GameVersList *self,
    GameVerSigEx *extendedSignature)
{
    int newItemsCount;
    int newItemsAllocated;
    GameVerSigEx *newItemsArray;

    newItemsCount = (self->count) + 1;

    if (newItemsCount > (self->allocated))
    {
        newItemsAllocated = 2 * (self->allocated);

        newItemsArray = realloc(
            self->signatures,
            newItemsAllocated * sizeof(GameVerSigEx));

        if (NULL == newItemsArray)
        {
            ErrorsQueue_push(
                "GameVersList_register(): realloc failed...\n");

            return FALSE;
        }

        self->allocated = newItemsAllocated;
        self->signatures = newItemsArray;
    }

    self->signatures[self->count] = extendedSignature[0];
    self->count = newItemsCount;

    return TRUE;
}

/**************************************************************/
int
GameVersList_findMatchingVersion(
    GameVersList *self,
    GameVerSig *dummySignature)
{
    BOOL test;

    for (int i = 0; i < (self->count); i++)
    {
        test = GameVerSig_compare(
            &(self->signatures[i].signature),
            dummySignature);

        if (FALSE != test)
        {
            return i;
        }
    }

    return (-1);
}

/**************************************************************/
