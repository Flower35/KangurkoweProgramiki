/**************************************************************/
#include "TextObjectList.h"

/**************************************************************/
BOOL
TextObjectList_init(
    TextObjectList *self)
{
    BOOL test = ReferableObjectList_init(
        &(self->list));

    return test;
}

/**************************************************************/
void
TextObjectList_destroy(
    TextObjectList *self)
{
    ReferableObjecttList_destroy(
        &(self->list));
}

/**************************************************************/
void
TextObjectList_clear(
    TextObjectList *self)
{
    ReferableObjecttList_clear(
        &(self->list));
}

/**************************************************************/
BOOL
TextObjectList_append(
    TextObjectList *self,
    TextObject *textRef)
{
    BOOL test = ReferableObjectList_append(
        &(self->list),
        (ReferableObject *) textRef);

    return test;
}

/**************************************************************/
BOOL
TextObjectList_appendRaw(
    TextObjectList *self,
    ReadOnlyUTF8String text)
{
    BOOL test;
    TextObject *textRef = TextObject_createOnHeap(text);

    if (NULL == textRef)
    {
        return FALSE;
    }

    test = TextObjectList_append(self, textRef);

    TextObject_decRef(textRef);

    return test;
}

/**************************************************************/
extern int
TextObjectList_indexOfTextRaw(
    TextObjectList *self,
    ReadOnlyUTF8String text)
{
    BOOL test;

    for (int i = 0; i < (self->list.count); i++)
    {
        test = TextObject_compare(
            (TextObject *) (self->list.itemRefs[i]),
            text);

        if (FALSE != test)
        {
            return i;
        }
    }

    return (-1);
}

/**************************************************************/
extern int
TextObjectList_indexOfText(
    TextObjectList *self,
    TextObject *textRef)
{
    return TextObjectList_indexOfTextRaw(
        self,
        (textRef->data.text));
}

/**************************************************************/
