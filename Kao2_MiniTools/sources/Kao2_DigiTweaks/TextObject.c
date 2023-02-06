/**************************************************************/
#include "TextObject.h"

/**************************************************************/
ReferableObject_VirtualMethods
g_TextObject_VirtualMethods =
{
    (void (*)(void *)) TextObject_destroy
};

/**************************************************************/
BOOL
TextObject_init(
    TextObject *self,
    ReadOnlyUTF8String text)
{
    BOOL test;

    /* Parent initialization */

     ReferableObject_init((void *) self);

     /* Self initialization */

    self->vptr = &(g_TextObject_VirtualMethods);

    self->data.length = 0;

    self->data.text = NULL;

    test = TextObject_loadNewText(self, text);

    return test;
}

/**************************************************************/
void
TextObject_destroy(
    TextObject *self)
{
    if (NULL != (self->data.text))
    {
        free(self->data.text);
    }
}

/**************************************************************/
TextObject *
TextObject_createOnHeap(
    ReadOnlyUTF8String text)
{
    BOOL test;
    TextObject *self = malloc(sizeof(TextObject));

    if (NULL == self)
    {
        return NULL;
    }

    test = TextObject_init(self, text);

    if (FALSE == test)
    {
        TextObject_destroy(self);
        free(self);

        return NULL;
    }

    TextObject_incRef(self);

    return self;
}

/**************************************************************/
void
TextObject_incRef(
    TextObject *self)
{
    ReferableObject_incRef((void *) self);
}

/**************************************************************/
void
TextObject_decRef(
    TextObject *self)
{
    ReferableObject_decRef((void *) self);
}

/**************************************************************/
BOOL
TextObject_loadNewText(
    TextObject *self,
    ReadOnlyUTF8String text)
{
    if (NULL != (self->data.text))
    {
        free(self->data.text);
        self->data.text = NULL;
    }

    if (NULL == text)
    {
        self->data.length = 0;

        return TRUE;
    }

    self->data.length = strlen(text) / sizeof(UTF8CodePoint);

    if (0 == (self->data.length))
    {
        return TRUE;
    }

    self->data.text = malloc(
        (self->data.length) * sizeof(UTF8CodePoint));

    if (NULL == (self->data.text))
    {
        return FALSE;
    }

    memcpy((self->data.text), text, (self->data.length));
    self->data.text[self->data.length] = '\0';

    return TRUE;
}

/**************************************************************/
BOOL
TextObject_compare(
    TextObject *self,
    ReadOnlyUTF8String otherText)
{
    int otherTextLength = strlen(otherText);

    if ((self->data.length) != otherTextLength)
    {
        return FALSE;
    }

    for (int i = 0; i < otherTextLength; i++)
    {
        if ((self->data.text[i]) != (otherText[i]))
        {
            return FALSE;
        }
    }

    return TRUE;
}

/**************************************************************/
