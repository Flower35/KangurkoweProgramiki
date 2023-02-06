/**************************************************************/
#ifndef HEADER_KAO2_DIGITWEAKS_REFERABLEOBJECT
#define HEADER_KAO2_DIGITWEAKS_REFERABLEOBJECT

/**************************************************************/
#include "Kao2_DigiTweaks.h"

/**************************************************************/
typedef struct ReferableObject_VirtualMethodsTag
    ReferableObject_VirtualMethods;

struct ReferableObject_VirtualMethodsTag
{
    void (*destroyingDtor)(void *self);
};

/**************************************************************/
extern ReferableObject_VirtualMethods
g_ReferableObject_VirtualMethods;

/**************************************************************/
typedef struct ReferableObject_DataTag ReferableObject_Data;

struct ReferableObject_DataTag
{
    int refCount;
};

/**************************************************************/
typedef struct ReferableObjectTag ReferableObject;

struct ReferableObjectTag
{
    ReferableObject_VirtualMethods *vptr;
    ReferableObject_Data data;
};

/**************************************************************/
extern void
ReferableObject_init(
    ReferableObject *self);

/**************************************************************/
extern void
ReferableObject_destroy(
    ReferableObject *self);

/**************************************************************/
extern void
ReferableObject_incRef(
    ReferableObject *self);

/**************************************************************/
extern void
ReferableObject_decRef(
    ReferableObject *self);

/**************************************************************/
#endif  /* HEADER_KAO2_DIGITWEAKS_REFERABLEOBJECT */

/**************************************************************/
