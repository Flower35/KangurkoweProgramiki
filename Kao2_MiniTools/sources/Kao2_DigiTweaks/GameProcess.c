/**************************************************************/
#include "GameProcess.h"
#include "ErrorsQueue.h"

/**************************************************************/
void
GameProcess_init(
    GameProcess *self)
{
    self->gameVersion = (-1);
    self->processHandle = NULL;
    self->imageBase = 0;
}

/**************************************************************/
void
GameProcess_detach(
    GameProcess *self)
{
    if (NULL != (self->processHandle))
    {
        CloseHandle(self->processHandle);
        (self->processHandle) = NULL;
    }

    (self->gameVersion) = (-1);
}

/**************************************************************/
BOOL
GameProcess_isActive(
    GameProcess *self)
{
    DWORD exitCode;

    if (NULL == (self->processHandle))
    {
        return FALSE;
    }

    GetExitCodeProcess(
        (self->processHandle),
        &(exitCode));

    return (STILL_ACTIVE == exitCode);
}

/**************************************************************/
BOOL
GameProcess_getModuleInfo(
    GameProcess *self,
    MODULEINFO *resultInfo)
{
    BOOL test;
    DWORD sizeInBytes;
    HMODULE moduleHandle;
    MODULEINFO moduleInfo;

    /* Read the first module (basically the `.ImageBase` value). */

    test = EnumProcessModules(
        (self->processHandle),
        &(moduleHandle),
        sizeof(moduleHandle),
        &(sizeInBytes));

    if ((FALSE == test) || (sizeInBytes < sizeof(HMODULE)))
    {
        ErrorsQueue_push(
            "EnumProcessModules() failed...\n");

        return FALSE;
    }

    /* `moduleInfo.lpBaseOfDll` will be exactly the same as `moduleHandle`. */

    test = GetModuleInformation(
        (self->processHandle),
        moduleHandle,
        &(moduleInfo),
        sizeof(MODULEINFO));

    if (FALSE == test)
    {
        ErrorsQueue_push(
            "GetModuleInformation() failed...\n");

        return FALSE;
    }

    resultInfo[0] = moduleInfo;

    return TRUE;
}

/**************************************************************/
BOOL
GameProcess_read(
    GameProcess *self,
    void *destination,
    uint32_t gameAddress,
    int32_t size)
{
    BOOL test;
    SIZE_T bytesRead;

    test = ReadProcessMemory(
        (self->processHandle),
        (void const *) ((ULONG_PTR) gameAddress),
        destination,
        size,
        &(bytesRead));

    if ((FALSE == test) || (size != bytesRead))
    {
        ErrorsQueue_push(
            "ReadProcessMemory() failed...\n");

        return FALSE;
    }

    return TRUE;
}

/**************************************************************/
BOOL
GameProcess_write(
    GameProcess *self,
    void const *source,
    uint32_t gameAddress,
    int32_t size)
{
    BOOL test;
    SIZE_T bytesWritten;

    test = WriteProcessMemory(
        (self->processHandle),
        (void *) ((ULONG_PTR) gameAddress),
        source,
        size,
        &(bytesWritten));

    if ((FALSE == test) || (size != bytesWritten))
    {
        ErrorsQueue_push(
            "WriteProcessMemory() failed...\n");

        return FALSE;
    }

    return TRUE;
}

/**************************************************************/
BOOL
GameProcess_readUInt32(
    GameProcess *self,
    uint32_t *destination,
    uint32_t gameAddress)
{
    BOOL test;

    test = GameProcess_read(
        self,
        destination,
        gameAddress,
        sizeof(uint32_t));

    return test;
}

/**************************************************************/
BOOL
GameProcess_writeUInt32(
    GameProcess *self,
    uint32_t const *source,
    uint32_t gameAddress)
{
    BOOL test;

    test = GameProcess_write(
        self,
        source,
        gameAddress,
        sizeof(uint32_t));

    return test;
}

/**************************************************************/
BOOL
GameProcess_readUnixTimestamp(
    GameProcess *self,
    uint32_t *resultTimestamp)
{
    BOOL test;
    uint32_t value;

    /* Read "e_lfanew" from the DOS stub. */

    test = GameProcess_readUInt32(
        self,
        &(value),
        ((self->imageBase) + 0x3C));

    if (FALSE == test)
    {
        return FALSE;
    }

    /* Read "Unix TimeStamp" from PE header. */

    test = GameProcess_readUInt32(
        self,
        resultTimestamp,
        ((self->imageBase) + value + 0x08));

    return test;
}

/**************************************************************/
