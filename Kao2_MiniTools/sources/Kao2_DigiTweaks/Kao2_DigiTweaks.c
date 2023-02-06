/**************************************************************/
#include "Kao2_DigiTweaks.h"
#include "ErrorsQueue.h"
#include "GUI.h"
#include "GameProcessWatcher.h"
#include "GameVersions.h"
#include "GameTweaker.h"

/**************************************************************/
uint32_t
DigiTweaks_packCommand(
    uint8_t request,
    uint8_t category,
    int16_t index)
{
    return (
        ((uint16_t) index) |
        (category << (2 * 8)) |
        (request << (3 * 8)));
}

/**************************************************************/
void
DigiTweaks_unpackCommand(
    uint32_t command,
    uint8_t *request,
    uint8_t *category,
    int16_t *index)
{
    request[0]  = 0x00FF & (command >> (3 * 8));
    category[0] = 0x00FF & (command >> (2 * 8));
    index[0]    = 0x0000FFFF & command;
}

/**************************************************************/
BOOL
DigiTweaksMain(
    BOOL initialized,
    HINSTANCE hInstance)
{
    BOOL test;
    GameProcess *game;
    GameVerSigEx *sig;

    g_errorsQAvail = TRUE;

    if (FALSE == initialized)
    {
        ErrorsQueue_push(
            "Failed to initialize \"Kao2_DigiTweaks\"!\n");

        return FALSE;
    }

    test = GameProcessWatcher_attachFirstMatching();

    if (FALSE == test)
    {
        /* No process named "kao2*" found! */

        return FALSE;
    }

    /* We are now attached to some recognized version of "kao2". */

    test = GameProcessWatcher_getProcWSig(&(game), &(sig));

    if (FALSE == test)
    {
        ErrorsQueue_push(
            "An unexpected error has occurred...\n");

        return FALSE;
    }

    /* Validate that user launched the latest Steam/Digital version. */

    if (FALSE == (sig->supported))
    {
        ErrorsQueue_push(
            "You are running an unsupported version of \"kao2\"!\n" \
            "Please try again with the 2019 Digital release of \"Kao the Kangaroo: Round 2\".\n");

        return FALSE;
    }

    /* Read all available game languages. */

    test = GameTweaker_loadLanguages();

    if (FALSE == test)
    {
        ErrorsQueue_push(
            "Failed to read game's language list!\n");

        return FALSE;
    }

    /* Now we can start the Graphical Interface */

    test = GUI_prepareGraphicalInterface(hInstance);

    if (FALSE == (sig->supported))
    {
        ErrorsQueue_push(
            "Failed to prepare the Graphical User Interface...\n");

        return FALSE;
    }

    GUI_populateListBoxes();

    /* Application's main loop. */

    while (GUI_mainLoopIteration())
    {
        if (FALSE == GameProcess_isActive(game))
        {
            return TRUE;
        }

        if (g_errorsQ.list.count > 0)
        {
            ErrorsQueue_dropBy();
        }
    }

    return TRUE;
}

/**************************************************************/
int WINAPI
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    int exitCode;
    BOOL test = TRUE;

    BOOL errorsQueueReady         = FALSE;
    BOOL guiReady                 = FALSE;
    BOOL gameProcessWatcherReady  = FALSE;
    BOOL gameVersionsReady        = FALSE;
    BOOL gameTweakerReady         = FALSE;

    if (test && (!errorsQueueReady))
    {
        test = ErrorsQueue_globalOpen();
        errorsQueueReady = TRUE;
    }

    if (test && (!guiReady))
    {
        GUI_globalOpen(GameTweaker_guiCallback);
        guiReady = TRUE;
    }

    if (test && (!gameProcessWatcherReady))
    {
        GameProcessWatcher_globalOpen();
        gameProcessWatcherReady = TRUE;
    }

    if (test && (!gameVersionsReady))
    {
        test = GameVersions_globalOpen();
        gameVersionsReady = TRUE;
    }

    if (test && (!gameTweakerReady))
    {
        test = GameTweaker_globalOpen();
        gameTweakerReady = TRUE;
    }

    /****************/

    test = DigiTweaksMain(test, hInstance);

    exitCode = test ? EXIT_SUCCESS : EXIT_FAILURE;

    if (g_errorsQ.list.count > 0)
    {
        ErrorsQueue_dropBy();
    }

    /****************/

    if (gameTweakerReady)
    {
        GameTweaker_globalClose();
    }

    if (gameVersionsReady)
    {
        GameVersions_globalClose();
    }

    if (gameProcessWatcherReady)
    {
        GameProcessWatcher_globalClose();
    }

    if (errorsQueueReady)
    {
        ErrorsQueue_globalClose();
    }

    return exitCode;
}

/**************************************************************/
