/**************************************************************/
#include "GameProcessWatcher.h"
#include "ErrorsQueue.h"
#include "GameVersions.h"
#include "TextHelper.h"

/**************************************************************/
GameProcess g_gameProcess;

/**************************************************************/
void
GameProcessWatcher_globalOpen(
    void)
{
    GameProcess_init(&(g_gameProcess));
}

/**************************************************************/
void
GameProcessWatcher_globalClose(
    void)
{
    GameProcess_detach(&(g_gameProcess));
}

/**************************************************************/
BOOL
GameProcessWatcher_attachFirstMatching(
    void)
{
    BOOL test;
    int pIdsCount;
    DWORD arrayByteSize = 1024 * sizeof(DWORD);
    DWORD *pIds = malloc(arrayByteSize);

    if (NULL == pIds)
    {
        ErrorsQueue_push(
            "GameProcessWatcher_attachFirstMatching(): malloc failed...\n");

        return FALSE;
    }

    test = EnumProcesses(
        pIds,
        arrayByteSize,
        &(arrayByteSize));

    if (FALSE == test)
    {
        ErrorsQueue_push(
            "EnumProcesses() failed...\n");

        free(pIds);

        return FALSE;
    }

    pIdsCount = arrayByteSize / sizeof(DWORD);

    for (int i = 0; i < pIdsCount; i++)
    {
        test = GameProcessWatcher_testSingleProcess(
            pIds[i]);

        if (FALSE != test)
        {
            free(pIds);

            return TRUE;
        }
    }

    ErrorsQueue_push(
        "No process named \"kao2*\" found...\n" \
        "Please launch \"Kao the Kangaroo: Round 2\" first!\n");

    free(pIds);

    return FALSE;
}

/**************************************************************/
BOOL
GameProcessWatcher_testSingleProcess(
    DWORD processId)
{
    BOOL test;
    MODULEINFO moduleInfo;
    GameProcess dummyGameProcess;
    GameVerSig dummySignature;

    (dummyGameProcess.processHandle) = OpenProcess(
        (PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION),
        FALSE,
        processId);

    if (NULL == (dummyGameProcess.processHandle))
    {
        /*
        fprintf(
            stderr,
            "OpenProcess(PID=0x%08X) failed...\n",
            processId);
        */

        return FALSE;
    }

    test = GameProcessWatcher_validateImageFileName(
        (dummyGameProcess.processHandle));

    if (FALSE == test)
    {
        CloseHandle(dummyGameProcess.processHandle);

        return FALSE;
    }

    test = GameProcess_getModuleInfo(
        &(dummyGameProcess),
        &(moduleInfo));

    if (FALSE == test)
    {
        CloseHandle(dummyGameProcess.processHandle);

        return FALSE;
    }

    dummyGameProcess.imageBase = (uint32_t) moduleInfo.lpBaseOfDll;

    test = GameProcess_readUnixTimestamp(
        &(dummyGameProcess),
        &(dummySignature.unixTimestamp));

    if (FALSE == test)
    {
        CloseHandle(dummyGameProcess.processHandle);

        return FALSE;
    }

    /* Prepare process signature. Change `entryPoint` to `RVA` value. */

    dummySignature.entryPoint = (uint32_t) moduleInfo.EntryPoint;
    dummySignature.entryPoint -= dummyGameProcess.imageBase;

    dummySignature.imageSize  = (uint32_t) moduleInfo.SizeOfImage;

    /*
    fprintf(stdout, "--------------------------------\n");
    fprintf(stdout, "* ImageBase:     0x%08X\n", dummyGameProcess.imageBase);
    fprintf(stdout, "* UnixTimeStamp: 0x%08X\n", dummySignature.unixTimestamp);
    fprintf(stdout, "* EntryPoint:    0x%08X\n", dummySignature.entryPoint);
    fprintf(stdout, "* ImageSize:     0x%08X\n", dummySignature.imageSize);
    fprintf(stdout, "\n");
    */

    (dummyGameProcess.gameVersion) = GameVersions_findMatchingVersion(
        &(dummySignature));

    /*
    fprintf(
        stderr,
        "Detected Game Version: (%d)\n",
        (dummyGameProcess.gameVersion));
    */

    if ((dummyGameProcess.gameVersion) < 0)
    {
        CloseHandle(dummyGameProcess.processHandle);

        return FALSE;
    }

    GameProcess_detach(&(g_gameProcess));
    g_gameProcess = dummyGameProcess;

    return TRUE;
}

/**************************************************************/
BOOL
GameProcessWatcher_validateImageFileName(
    HANDLE dummyProcess)
{
    BOOL test;
    DWORD bytesWritten;
    UTF8String fileNameStart;
    UTF8String dummyText;
    UTF16CodePoint wideModuleName[MAX_PATH];
    UTF8CodePoint narrowModuleName[MAX_PATH];

    bytesWritten = GetProcessImageFileNameW(
        dummyProcess,
        wideModuleName,
        MAX_PATH);

    if (0 == bytesWritten)
    {
        return FALSE;
    }

    wideModuleName[MAX_PATH - 1] = L'\0';

    dummyText = convertUTF16to8(
        wideModuleName,
        (-1),
        narrowModuleName,
        MAX_PATH);

    if (dummyText != narrowModuleName)
    {
        return FALSE;
    }

    fileNameStart = NULL;

    for (dummyText = (UTF8String) &(endOfUTF8String(narrowModuleName)[-1]);
        (NULL == fileNameStart) && (dummyText > narrowModuleName);
        dummyText--)
    {
        if ('\\' == dummyText[0])
        {
            fileNameStart = &(dummyText[1]);
        }
    }

    if (NULL == fileNameStart)
    {
        return FALSE;
    }

    test = stringStartsWith(
        fileNameStart,
        "kao2");

    return test;
}

/**************************************************************/
BOOL
GameProcessWatcher_getProcWSig(
    GameProcess **processResult,
    GameVerSigEx **signatureResult)
{
    /* Get (Game) Process with (Game Version) Signature */

    BOOL test = GameVersions_getSignatureByIdx(
        signatureResult,
        (g_gameProcess.gameVersion));

    if (FALSE == test)
    {
        return FALSE;
    }

    processResult[0] = &(g_gameProcess);

    return TRUE;
}

/**************************************************************/
