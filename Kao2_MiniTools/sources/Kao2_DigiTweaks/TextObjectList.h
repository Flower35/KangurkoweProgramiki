/**************************************************************/
#ifndef HEADER_KAO2_DIGITWEAKS_TEXTOBJECTLIST
#define HEADER_KAO2_DIGITWEAKS_TEXTOBJECTLIST

/**************************************************************/
#include "ReferableObjectList.h"
#include "TextObject.h"

/**************************************************************/
typedef struct TextObjectListTag TextObjectList;

struct TextObjectListTag
{
    ReferableObjectList list;
};

/**************************************************************/
extern BOOL
TextObjectList_init(
    TextObjectList *self);

/**************************************************************/
extern void
TextObjectList_destroy(
    TextObjectList *self);

/**************************************************************/
extern void
TextObjectList_clear(
    TextObjectList *self);

/**************************************************************/
extern BOOL
TextObjectList_append(
    TextObjectList *self,
    TextObject *textRef);

/**************************************************************/
extern BOOL
TextObjectList_appendRaw(
    TextObjectList *self,
    ReadOnlyUTF8String text);

/**************************************************************/
extern int
TextObjectList_indexOfTextRaw(
    TextObjectList *self,
    ReadOnlyUTF8String text);

/**************************************************************/
extern int
TextObjectList_indexOfText(
    TextObjectList *self,
    TextObject *textRef);

/**************************************************************/
#endif  /* HEADER_KAO2_DIGITWEAKS_TEXTOBJECTLIST */

/**************************************************************/
