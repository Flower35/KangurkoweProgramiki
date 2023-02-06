/**************************************************************/
#include "ReferableObject.h"

/**************************************************************/
ReferableObject_VirtualMethods
g_ReferableObject_VirtualMethods =
{
    (void (*)(void *)) ReferableObject_destroy
};

/**************************************************************/
void
ReferableObject_init(
    ReferableObject *self)
{
    (self->vptr) = &(g_ReferableObject_VirtualMethods);

    (self->data.refCount) = 0;
}

/**************************************************************/
void
ReferableObject_destroy(
    ReferableObject *self)
{}

/**************************************************************/
void
ReferableObject_incRef(
    ReferableObject *self)
{
    (self->data.refCount) += 1;
}

/**************************************************************/
void
ReferableObject_decRef(
    ReferableObject *self)
{
    (self->data.refCount) -= 1;

    if (0 == (self->data.refCount))
    {
        (self->vptr)->destroyingDtor(self);

        free(self);
    }
}

/**************************************************************/
