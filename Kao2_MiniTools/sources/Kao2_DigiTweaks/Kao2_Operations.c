/**************************************************************/
#include "Kao2_Operations.h"
#include "Kao2_Classes.h"
#include "GameTweaker.h"

/**************************************************************/
BOOL
Kao2_StringWrapper_read(
    UTF8String destination,
    int bufferSize,
    GameProcess *game,
    uint32_t stringWrapperAddr)
{
    BOOL test;
    uint32_t stringBaseAddr;

    test = GameProcess_readUInt32(
        game,
        &(stringBaseAddr),
        stringWrapperAddr);

    if (FALSE == test)
    {
        return FALSE;
    }

    return Kao2_String_read(
        destination,
        bufferSize,
        game,
        stringBaseAddr);
}

/**************************************************************/
BOOL
Kao2_StringWrapper_write(
    ReadOnlyUTF8String source,
    int sourceLength,
    GameProcess *game,
    uint32_t stringWrapperAddr)
{
    BOOL test;
    uint32_t stringBaseAddr;

    test = GameProcess_readUInt32(
        game,
        &(stringBaseAddr),
        stringWrapperAddr);

    if (FALSE == test)
    {
        return FALSE;
    }

    return Kao2_String_write(
        source,
        sourceLength,
        game,
        stringBaseAddr);
}

/**************************************************************/
BOOL
Kao2_String_read(
    UTF8String destination,
    int bufferSize,
    GameProcess *game,
    uint32_t stringBaseAddr)
{
    BOOL test;
    int maxLengthToRead;
    Kao2_StringHeader strHead;

    test = GameProcess_read(
        game,
        &(strHead),
        stringBaseAddr,
        sizeof(strHead));

    if (FALSE == test)
    {
        return FALSE;
    }

    maxLengthToRead = max((bufferSize - 1), (strHead.length));

    test = GameProcess_read(
        game,
        destination,
        (stringBaseAddr + offsetof(Kao2_String, text)),
        maxLengthToRead);

    if (FALSE == test)
    {
        return FALSE;
    }

    destination[maxLengthToRead] = '\0';

    return TRUE;
}

/**************************************************************/
BOOL
Kao2_String_write(
    ReadOnlyUTF8String source,
    int sourceLength,
    GameProcess *game,
    uint32_t stringBaseAddr)
{
    BOOL test;
    int maxLengthToWrite;
    Kao2_StringHeader strHead;

    test = GameProcess_read(
        game,
        &(strHead),
        stringBaseAddr,
        sizeof(strHead));

    if (FALSE == test)
    {
        return FALSE;
    }

    maxLengthToWrite = max((sourceLength), (strHead.allocatedLength - 1));

    test = GameProcess_write(
        game,
        source,
        (stringBaseAddr + offsetof(Kao2_String, text)),
        maxLengthToWrite);

    return test;
}

/**************************************************************/
BOOL
Kao2_StringWrappersArray_read(
    TextObjectList *textList,
    GameProcess *game,
    uint32_t arrayAddr)
{
    BOOL test;
    int32_t count;
    uint32_t arrayBase;
    UTF8CodePoint textBuffer[2 + 1];

    TextObjectList_clear(textList);

    test = GameProcess_readUInt32(
        game,
        (uint32_t *) &(count),
        (arrayAddr + offsetof(Kao2_Array, count)));

    if (FALSE == test)
    {
        return FALSE;
    }

    test = GameProcess_readUInt32(
        game,
        &(arrayBase),
        (arrayAddr + offsetof(Kao2_Array, arrayPointer)));

    if (FALSE == test)
    {
        return FALSE;
    }

    for (int i = 0; i < count; i++)
    {
        test = Kao2_StringWrapper_read(
            textBuffer,
            sizeof(textBuffer),
            game,
            (arrayBase + i * sizeof(uint32_t)));

        if (FALSE == test)
        {
            return FALSE;
        }

        test = TextObjectList_appendRaw(textList, textBuffer);

        if (FALSE == test)
        {
            return FALSE;
        }
    }

    return TRUE;
}

/**************************************************************/
BOOL
Kao2_Lang_readLangList(
    GameProcess *game,
    GameVerSigEx *sig)
{
    BOOL test;
    uint32_t langObjectBase;
    uint32_t addressValue;

    /* Validate "eLang" object type. */

    langObjectBase = ((game->imageBase) + (sig->langAddr));

    test = GameProcess_readUInt32(
        game,
        &(addressValue),
        langObjectBase);

    if (FALSE == test)
    {
        return FALSE;
    }

    if (((game->imageBase) + (sig->langVptr)) != addressValue)
    {
        return FALSE;
    }

    /* Read the list of "short language names". */

    return Kao2_StringWrappersArray_read(
        &(g_languages),
        game,
        (langObjectBase + offsetof(Kao2_Lang, langs)));
}

/**************************************************************/
int
Kao2_Lang_getSubsLang(
    GameProcess *game,
    GameVerSigEx *sig)
{
    BOOL test;
    uint32_t value;

    value = ((game->imageBase) + (sig->langAddr) +
        offsetof(Kao2_Lang, currentSubsLang));

    test = GameProcess_readUInt32(
        game,
        &(value),
        value);

    if (FALSE == test)
    {
        return (-1);
    }

    if ((value >= 0) && (value < (g_languages.list.count)))
    {
        return value;
    }

    return (-1);
}

/**************************************************************/
int
Kao2_Lang_getVoicesLang(
    GameProcess *game,
    GameVerSigEx *sig)
{
    BOOL test;
    uint32_t address;
    UTF8CodePoint textBuffer[2 + 1];

    address = ((game->imageBase) + (sig->langAddr) +
        offsetof(Kao2_Lang, currentVoicesLang));

    test = Kao2_StringWrapper_read(
        textBuffer,
        sizeof(textBuffer),
        game,
        address);

    if (FALSE == test)
    {
        return (-1);
    }

    return TextObjectList_indexOfTextRaw(
        &(g_languages),
        textBuffer);
}

/**************************************************************/
BOOL
Kao2_Lang_setSubsLang(
    int idx,
    GameProcess *game,
    GameVerSigEx *sig)
{
    uint32_t address;

    if ((idx < 0) || (idx >= (g_languages.list.count)))
    {
        return FALSE;
    }

    address = ((game->imageBase) + (sig->langAddr) +
        offsetof(Kao2_Lang, currentSubsLang));

    return GameProcess_writeUInt32(
        game,
        (uint32_t *) &(idx),
        address);
}

/**************************************************************/
BOOL
Kao2_Lang_setVoicesLang(
    int idx,
    GameProcess *game,
    GameVerSigEx *sig)
{
    BOOL test;
    TextObject *textRef;
    uint32_t currentVoicesLangAddr;
    uint32_t currentVoicesLangStrBase;
    uint32_t currentVoicesLangStrRefCount;
    uint32_t newVoicesLangAddr;
    uint32_t newVoicesLangStrBase;
    uint32_t newVoicesLangStrRefCount;

    if ((idx < 0) || (idx >= (g_languages.list.count)))
    {
        return FALSE;
    }

    currentVoicesLangAddr = ((game->imageBase) + (sig->langAddr) +
        offsetof(Kao2_Lang, currentVoicesLang));

    test = GameProcess_readUInt32(
        game,
        &(currentVoicesLangStrBase),
        currentVoicesLangAddr);

    if (FALSE == test)
    {
        return FALSE;
    }

    test = GameProcess_readUInt32(
        game,
        &(currentVoicesLangStrRefCount),
        currentVoicesLangStrBase);

    if (FALSE == test)
    {
        return FALSE;
    }

    if (1 == currentVoicesLangStrRefCount)
    {
        /* String has `refCount==`, so we can directly replace the text! */

        textRef = (TextObject *) (g_languages.list.itemRefs[idx]);

        test = Kao2_String_write(
            textRef->data.text,
            textRef->data.length,
            game,
            currentVoicesLangStrBase);
    }
    else
    {
        /* Decrease refCount of `currentVoicesLang` */

        currentVoicesLangStrRefCount -= 1;

        test = GameProcess_writeUInt32(
            game,
            &(currentVoicesLangStrRefCount),
            currentVoicesLangStrBase);

        if (FALSE == test)
        {
            return FALSE;
        }

        /* Get string reference for `newVoicesLang` */

        newVoicesLangAddr = ((game->imageBase) + (sig->langAddr) +
            offsetof(Kao2_Lang, langs.arrayPointer));

        test = GameProcess_readUInt32(
            game,
            &(newVoicesLangAddr),
            newVoicesLangAddr);

        if (FALSE == test)
        {
            return FALSE;
        }

        newVoicesLangAddr += idx * sizeof(uint32_t);

        test = GameProcess_readUInt32(
            game,
            &(newVoicesLangStrBase),
            newVoicesLangAddr);

        if (FALSE == test)
        {
            return FALSE;
        }

        /* Increase refCount of `newVoicesLang` */

        test = GameProcess_readUInt32(
            game,
            &(newVoicesLangStrRefCount),
            newVoicesLangStrBase);

        if (FALSE == test)
        {
            return FALSE;
        }

        newVoicesLangStrRefCount += 1;

        test = GameProcess_writeUInt32(
            game,
            &(newVoicesLangStrRefCount),
            newVoicesLangStrBase);

        if (FALSE == test)
        {
            return FALSE;
        }

        /* Replace `currentVoicesLang` with `newVoicesLang` */

        test = GameProcess_writeUInt32(
            game,
            &(newVoicesLangStrBase),
            currentVoicesLangAddr);
    }

    return test;
}

/**************************************************************/
