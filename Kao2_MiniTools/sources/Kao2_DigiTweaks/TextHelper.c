/**************************************************************/
#include "TextHelper.h"

/**************************************************************/
BOOL
stringStartsWith(
    ReadOnlyUTF8String strA,
    ReadOnlyUTF8String strB)
{
    int i;

    if ((NULL == strA) || (NULL == strB))
    {
        return FALSE;
    }

    i = 0;

    if ('\0' == strB[i])
    {
        return FALSE;
    }

    while ('\0' != strA[i])
    {
        if (strA[i] != strB[i])
        {
            return ('\0' == strB[i]);
        }

        i++;
    }

    return FALSE;
}

/**************************************************************/
UTF16String
convertUTF8to16(
    ReadOnlyUTF8String inputText,
    int inputLengthInBytes,
    UTF16String outputBuffer,
    int outputLengthInWideChars)
{
    int test = MultiByteToWideChar(
        CP_UTF8,
        0,
        inputText,
        inputLengthInBytes,
        outputBuffer,
        outputLengthInWideChars);

    if (test <= 0)
    {
        /* Niepowodzenie mapowania tekstu! */

        return NULL;
    }

    /* Czy bufor po konwersji jest NULL-terminated? */

    if ((-1) == inputLengthInBytes)
    {
        if (test > outputLengthInWideChars)
        {
            /* Wiadomość została ucięta! */

            return NULL;
        }
    }
    else
    {
        /* Należy dopisać znak L'\0' ręcznie. */

        if (test > (outputLengthInWideChars - 1))
        {
            /* Wiadomość została ucięta! */

            return NULL;
        }

        outputBuffer[test] = L'\0';
    }

    return outputBuffer;
}

/**************************************************************/
UTF8String
convertUTF16to8(
    ReadOnlyUTF16String inputText,
    int inputLengthInWideChars,
    UTF8String outputBuffer,
    int outputLengthInBytes)
{
    int test = WideCharToMultiByte(
        CP_UTF8,
        0,
        inputText,
        inputLengthInWideChars,
        outputBuffer,
        outputLengthInBytes,
        NULL,
        NULL);

    if (test <= 0)
    {
        /* Niepowodzenie mapowania tekstu! */

        return NULL;
    }

    /* Czy bufor po konwersji jest NULL-terminated? */

    if ((-1) == inputLengthInWideChars)
    {
        if (test > outputLengthInBytes)
        {
            /* Wiadomość została ucięta! */

            return NULL;
        }
    }
    else
    {
        /* Należy dopisać znak '\0' ręcznie. */

        if (test > (outputLengthInBytes - 1))
        {
            /* Wiadomość została ucięta! */

            return NULL;
        }

        outputBuffer[test] = '\0';
    }

    return outputBuffer;
}

/**************************************************************/
ReadOnlyUTF8String
endOfUTF8String(
    ReadOnlyUTF8String text)
{
    if (NULL == text)
    {
        return NULL;
    }

    while ('\0' != text[0])
    {
        text++;
    }

    return text;
}

/**************************************************************/
