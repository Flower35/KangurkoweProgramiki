/**************************************************************/
#include "GameTweaker.h"
#include "GameProcessWatcher.h"
#include "Kao2_Operations.h"

/**************************************************************/
PatchExList g_patches;
TextObjectList g_languages;

/**************************************************************/
BOOL
GameTweaker_globalOpen(
    void)
{
    BOOL testA = PatchExList_init(&(g_patches));
    BOOL testB = TextObjectList_init(&(g_languages));

    if ((FALSE == testA) || (FALSE == testB))
    {
        return FALSE;
    }

    return GameTweaker_loadPatches();
}

/**************************************************************/
void
GameTweaker_globalClose(
    void)
{
    TextObjectList_destroy(&(g_languages));
    PatchExList_destroy(&(g_patches));
}

/**************************************************************/
BOOL
GameTweaker_loadPatches(
    void)
{
    BOOL test;
    PatchEx *dummyPatch;
    Patch_Data patchParams;

    /* "Skip Intro Logos" patch entry. */

    dummyPatch = PatchEx_createOnHeap(
        "Skip Intro Logos");

    if (NULL == dummyPatch)
    {
        return FALSE;
    }

    test = PatchExList_append(&(g_patches), dummyPatch);

    PatchEx_decRef(dummyPatch);

    if (FALSE == test)
    {
        return FALSE;
    }

    /* "Skip Intro Logos" parameters. */

    patchParams.relAddr   = 0x00015E98;
    patchParams.patchSize = 0x06;
    patchParams.oldBytes  = (uint8_t *) "\x8B\x8D\x4C\xFF\xFF\xFF";
    patchParams.newBytes  = (uint8_t *) "\xEB\x09\x90\x90\x90\x90";

    test = PatchEx_registerSubpatch(dummyPatch, &(patchParams));

    if (FALSE == test)
    {
        return FALSE;
    }

    patchParams.relAddr   = 0x000165D7;
    patchParams.patchSize = 0x06;
    patchParams.oldBytes  = (uint8_t *) "\x8B\x4D\xE4\x3B\x4D\xEC";
    patchParams.newBytes  = (uint8_t *) "\xE9\xF7\x00\x00\x00\x90";

    test = PatchEx_registerSubpatch(dummyPatch, &(patchParams));

    if (FALSE == test)
    {
        return FALSE;
    }

    /* "VirtualSound SetPan Fix" patch entry. */

    dummyPatch = PatchEx_createOnHeap(
        "VirtualSound SetPan Fix");

    if (NULL == dummyPatch)
    {
        return FALSE;
    }

    test = PatchExList_append(&(g_patches), dummyPatch);

    PatchEx_decRef(dummyPatch);

    if (FALSE == test)
    {
        return FALSE;
    }

    /* "VirtualSound SetPan Fix" parameters. */

    patchParams.relAddr   = 0x00074082;
    patchParams.patchSize = 0x02;
    patchParams.oldBytes  = (uint8_t *) "\x7D\x7E";
    patchParams.newBytes  = (uint8_t *) "\xEB\x7E";

    test = PatchEx_registerSubpatch(dummyPatch, &(patchParams));

    if (FALSE == test)
    {
        return FALSE;
    }

    /* Loading complete! */

    return TRUE;
}

/**************************************************************/
BOOL
GameTweaker_loadLanguages(
    void)
{
    GameProcess *game;
    GameVerSigEx *sig;

    BOOL test = GameProcessWatcher_getProcWSig(&(game), &(sig));

    if (FALSE == test)
    {
        return FALSE;
    }

    return Kao2_Lang_readLangList(game, sig);
}

/**************************************************************/
void const *
GameTweaker_guiCallback(
    uint32_t command)
{
    BOOL test;
    GameProcess *game;
    GameVerSigEx *sig;

    uint8_t request;
    uint8_t category;
    int16_t index;

    TextObject *textRef;
    PatchEx *patchRef;

    DigiTweaks_unpackCommand(
        command,
        &(request),
        &(category),
        &(index));

    test = GameProcessWatcher_getProcWSig(&(game), &(sig));

    if (FALSE == test)
    {
        return NULL;
    }

    if (DIGITWEAKS_REQUEST_GET == request)
    {
        if (index < 0)
        {
            /* Get current selection (as 1-based idx) */

            if (DIGITWEAKS_CATEGORY_LANG_VOICES == category)
            {
                return (void const *) (1 + Kao2_Lang_getVoicesLang(game, sig));
            }
            else if (DIGITWEAKS_CATEGORY_LANG_SUBS == category)
            {
                return (void const *) (1 + Kao2_Lang_getSubsLang(game, sig));
            }
        }
        else
        {
            /* Get n-th item (as text) */

            if ((DIGITWEAKS_CATEGORY_LANG_VOICES == category) ||
                (DIGITWEAKS_CATEGORY_LANG_SUBS == category))
            {
                if ((index >= 0) && (index < (g_languages.list.count)))
                {
                    textRef = (TextObject *) (g_languages.list.itemRefs[index]);

                    return (textRef->data.text);
                }
            }
            else if (DIGITWEAKS_CATEGORY_PATCHES == category)
            {
                if ((index >= 0) && (index < (g_patches.list.count)))
                {
                    patchRef = (PatchEx *) (g_patches.list.itemRefs[index]);

                    return (patchRef->data.name);
                }
            }
        }
    }
    else if (DIGITWEAKS_REQUEST_SET == request)
    {
        if (DIGITWEAKS_CATEGORY_LANG_VOICES == category)
        {
            return (void const *) Kao2_Lang_setVoicesLang(index, game, sig);
        }
        else if (DIGITWEAKS_CATEGORY_LANG_SUBS == category)
        {
            return (void const *) Kao2_Lang_setSubsLang(index, game, sig);
        }
        else if (DIGITWEAKS_CATEGORY_PATCHES == category)
        {
            if ((index >= 0) && (index < (g_patches.list.count)))
            {
                patchRef = (PatchEx *) (g_patches.list.itemRefs[index]);

                return (void const *) PatchEx_tryOut(patchRef, TRUE, game);
            }
        }
    }
    else if (DIGITWEAKS_REQUEST_UNSET == request)
    {
        if (DIGITWEAKS_CATEGORY_PATCHES == category)
        {
            if ((index >= 0) && (index < (g_patches.list.count)))
            {
                patchRef = (PatchEx *) (g_patches.list.itemRefs[index]);

                return (void const *) PatchEx_tryOut(patchRef, FALSE, game);
            }
        }
    }

    return NULL;
}

/**************************************************************/
