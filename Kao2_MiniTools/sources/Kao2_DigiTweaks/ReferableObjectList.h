/**************************************************************/
#ifndef HEADER_KAO2_DIGITWEAKS_REFERABLEOBJECTLIST
#define HEADER_KAO2_DIGITWEAKS_REFERABLEOBJECTLIST

/**************************************************************/
#include "ReferableObject.h"

/**************************************************************/
typedef struct ReferableObjectListTag ReferableObjectList;

struct ReferableObjectListTag
{
    int count;
    int maxAlloc;
    ReferableObject **itemRefs;
};

/**************************************************************/
extern BOOL
ReferableObjectList_init(
    ReferableObjectList *self);

/**************************************************************/
extern void
ReferableObjecttList_destroy(
    ReferableObjectList *self);

/**************************************************************/
extern void
ReferableObjecttList_clear(
    ReferableObjectList *self);

/**************************************************************/
extern BOOL
ReferableObjectList_append(
    ReferableObjectList *self,
    ReferableObject *itemRef);

/**************************************************************/
#endif  /* HEADER_KAO2_DIGITWEAKS_REFERABLEOBJECTLIST */

/**************************************************************/
