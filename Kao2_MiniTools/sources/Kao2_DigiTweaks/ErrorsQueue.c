/**************************************************************/
#include "ErrorsQueue.h"
#include "GUI.h"

/**************************************************************/
BOOL g_errorsQAvail;
TextObjectList g_errorsQ;

/**************************************************************/
BOOL
ErrorsQueue_globalOpen(
    void)
{
    g_errorsQAvail = FALSE;

    return TextObjectList_init(&(g_errorsQ));
}

/**************************************************************/
void
ErrorsQueue_globalClose(
    void)
{
    TextObjectList_destroy(&(g_errorsQ));
}

/**************************************************************/
void
ErrorsQueue_push(
    ReadOnlyUTF8String text)
{
    if (g_errorsQAvail)
    {
        g_errorsQAvail = FALSE;

        TextObjectList_appendRaw(
            &(g_errorsQ),
            text);

        g_errorsQAvail = TRUE;
    }
}

/**************************************************************/
void
ErrorsQueue_dropBy(
    void)
{
    BOOL lastErrorsAvailState;
    int textPos = 0;
    UTF8CodePoint textBuffer[MAX_TEXT_LENGTH];
    TextObject *textRef;

    for (int i = (g_errorsQ.list.count - 1);
        (i >= 0) && (textPos >= 0);
        i--)
    {
        textRef = (TextObject *) (g_errorsQ.list.itemRefs[i]);

        if (((textRef->data.length) + textPos) <= (MAX_TEXT_LENGTH - 1))
        {
            memcpy(
                &(textBuffer[textPos]),
                &(textRef->data.text[0]),
                (textRef->data.length));

            textPos += (textRef->data.length);

            if (((textRef->data.length) + 1) <= (MAX_TEXT_LENGTH - 1))
            {
                textBuffer[textPos] = '\n';
                textPos += 1;
            }
            else
            {
                textBuffer[textPos] = '\0';
                textPos = (-1);
            }
        }
        else
        {
            textBuffer[textPos] = '\0';
            textPos = (-1);
        }
    }

    if (textPos >= 0)
    {
        textBuffer[textPos] = '\0';
    }

    lastErrorsAvailState = g_errorsQAvail;

    g_errorsQAvail = FALSE;

    GUI_showErrorMessage(textBuffer);

    g_errorsQAvail = lastErrorsAvailState;

    TextObjectList_clear(&(g_errorsQ));
}

/**************************************************************/
