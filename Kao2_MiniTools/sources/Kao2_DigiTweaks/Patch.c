/**************************************************************/
#include "Patch.h"
#include "ErrorsQueue.h"

/**************************************************************/
ReferableObject_VirtualMethods
g_Patch_VirtualMethods =
{
    (void (*)(void *)) Patch_destroy
};

/**************************************************************/
BOOL
Patch_init(
    Patch *self,
    Patch_Data *params)
{
    /* Parent initialization */

     ReferableObject_init((void *) self);

     /* Self initialization */

    self->vptr = &(g_Patch_VirtualMethods);

    if ((params->patchSize) > MAX_PATCH_SIZE)
    {
        ErrorsQueue_push(
            "Patch_init() failed: (params->patchSize) > MAX_PATCH_SIZE...");

        (self->data.oldBytes) = NULL;
        (self->data.newBytes) = NULL;

        return FALSE;
    }

    (self->data.relAddr) = (params->relAddr);
    (self->data.patchSize) = (params->patchSize);

    (self->data.oldBytes) = malloc(self->data.patchSize);
    (self->data.newBytes) = malloc(self->data.patchSize);

    if ((NULL == (self->data.oldBytes)) || (NULL == (self->data.newBytes)))
    {
        return FALSE;
    }

    memcpy((self->data.oldBytes), (params->oldBytes), (self->data.patchSize));
    memcpy((self->data.newBytes), (params->newBytes), (self->data.patchSize));

    return TRUE;
}

/**************************************************************/
void
Patch_destroy(
    Patch *self)
{
    if (NULL != (self->data.oldBytes))
    {
        free(self->data.oldBytes);
    }

    if (NULL != (self->data.newBytes))
    {
        free(self->data.newBytes);
    }
}

/**************************************************************/
Patch *
Patch_createOnHeap(
    Patch_Data *params)
{
    BOOL test;
    Patch *self = malloc(sizeof(Patch));

    if (NULL == self)
    {
        return NULL;
    }

    test = Patch_init(self, params);

    if (FALSE == test)
    {
        Patch_destroy(self);
        free(self);

        return NULL;
    }

    Patch_incRef(self);

    return self;
}

/**************************************************************/
void
Patch_incRef(
    Patch *self)
{
    ReferableObject_incRef((void *) self);
}

/**************************************************************/
void
Patch_decRef(
    Patch *self)
{
    ReferableObject_decRef((void *) self);
}

/**************************************************************/
BOOL
Patch_tryOut(
    Patch *self,
    BOOL applyElseRestore,
    GameProcess *game)
{
    BOOL test;
    uint32_t address;
    uint8_t const* fromBytes;
    uint8_t const* toBytes;
    uint8_t buffer[MAX_PATCH_SIZE];

    address = (game->imageBase) + (self->data.relAddr);

    if (applyElseRestore)
    {
        /* Apply new code */

        fromBytes = (self->data.oldBytes);
        toBytes   = (self->data.newBytes);
    }
    else
    {
        /* Restore old code */

        fromBytes = (self->data.newBytes);
        toBytes   = (self->data.oldBytes);
    }

    /* Read from game memory */

    test = GameProcess_read(
        game,
        buffer,
        address,
        (self->data.patchSize));

    if (FALSE == test)
    {
        return FALSE;
    }

    /* Is the code already patched (or already restored)? */

    if (0 == memcmp(buffer, toBytes, (self->data.patchSize)))
    {
        return TRUE;
    }

    /* Otherwise, is the code valid? */

    if (0 != memcmp(buffer, fromBytes, (self->data.patchSize)))
    {
        return FALSE;
    }

    /* Write to game memory */

    test = GameProcess_write(
        game,
        toBytes,
        address,
        (self->data.patchSize));

    return test;
}

/**************************************************************/
