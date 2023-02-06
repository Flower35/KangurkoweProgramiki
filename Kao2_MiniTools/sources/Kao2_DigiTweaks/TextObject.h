/**************************************************************/
#ifndef HEADER_KAO2_DIGITWEAKS_TEXTOBJECT
#define HEADER_KAO2_DIGITWEAKS_TEXTOBJECT

/**************************************************************/
#include "ReferableObject.h"
#include "TextHelper.h"

/**************************************************************/
#define MAX_TEXT_LENGTH 256

/**************************************************************/
extern ReferableObject_VirtualMethods
g_TextObject_VirtualMethods;

/**************************************************************/
typedef struct TextObject_DataTag TextObject_Data;

struct TextObject_DataTag
{
    ReferableObject_Data _super;

    int length;
    UTF8CodePoint *text;
};

/**************************************************************/
typedef struct TextObjectTag TextObject;

struct TextObjectTag
{
    ReferableObject_VirtualMethods *vptr;
    TextObject_Data data;
};

/**************************************************************/
extern BOOL
TextObject_init(
    TextObject *self,
    ReadOnlyUTF8String text);

/**************************************************************/
extern void
TextObject_destroy(
    TextObject *self);

/**************************************************************/
extern TextObject *
TextObject_createOnHeap(
    ReadOnlyUTF8String text);

/**************************************************************/
extern void
TextObject_incRef(
    TextObject *self);

/**************************************************************/
extern void
TextObject_decRef(
    TextObject *self);

/**************************************************************/
extern BOOL
TextObject_loadNewText(
    TextObject *self,
    ReadOnlyUTF8String text);

/**************************************************************/
extern BOOL
TextObject_compare(
    TextObject *self,
    ReadOnlyUTF8String otherText);

/**************************************************************/
#endif  /* HEADER_KAO2_DIGITWEAKS_TEXTOBJECT */

/**************************************************************/
