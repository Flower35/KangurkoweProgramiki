/**************************************************************/
#include "GameVersions.h"

/**************************************************************/
GameVersList g_gameVersList;

/**************************************************************/
BOOL
GameVersions_globalOpen(
    void)
{
    BOOL test;
    GameVerSigEx extSig;

    test = GameVersList_init(
        &(g_gameVersList));

    if (FALSE == test)
    {
        return FALSE;
    }

    extSig.supported = FALSE;
    extSig.name      = "Kao2 2003 Retail";

    extSig.signature.unixTimestamp = 0x3BF7BCFF;
    extSig.signature.entryPoint    = 0x001A5632;
    extSig.signature.imageSize     = 0x00268000;

    extSig.gameletAddr = 0;
    extSig.gameletVptr = 0;
    extSig.langAddr    = 0;
    extSig.langVptr    = 0;

    test = GameVersList_register(
        &(g_gameVersList),
        &(extSig));

    if (FALSE == test)
    {
        return FALSE;
    }

    extSig.supported = TRUE;
    extSig.name      = "Kao2 2019 v1.2.0 Digital";

    extSig.signature.unixTimestamp = 0x5D911D71;
    extSig.signature.entryPoint    = 0x00244006;
    extSig.signature.imageSize     = 0x00847000;

    extSig.gameletAddr = 0x0073CEF8;
    extSig.gameletVptr = 0x0056A96C;
    extSig.langAddr    = 0x0073D53C;
    extSig.langVptr    = 0x00584BE4;

    test = GameVersList_register(
        &(g_gameVersList),
        &(extSig));

    if (FALSE == test)
    {
        return FALSE;
    }

    extSig.supported = TRUE;
    extSig.name      = "Kao2 2019 v1.2.0 Steam";

    extSig.signature.unixTimestamp = 0x5D911D18;
    extSig.signature.entryPoint    = 0x00848310;
    extSig.signature.imageSize     = 0x0086B000;

    extSig.gameletAddr = 0x0073E0F8;
    extSig.gameletVptr = 0x0056B98C;
    extSig.langAddr    = 0x0073E73C;
    extSig.langVptr    = 0x00585C0C;

    test = GameVersList_register(
        &(g_gameVersList),
        &(extSig));

    if (FALSE == test)
    {
        return FALSE;
    }

    extSig.supported = TRUE;
    extSig.name      = "Kao2 2019 v1.2.0 Steamless";

    extSig.signature.unixTimestamp = 0x5D911D18;
    extSig.signature.entryPoint    = 0x002450D0;
    extSig.signature.imageSize     = 0x00848000;

    extSig.gameletAddr = 0x0073E0F8;
    extSig.gameletVptr = 0x0056B98C;
    extSig.langAddr    = 0x0073E73C;
    extSig.langVptr    = 0x00585C0C;

    test = GameVersList_register(
        &(g_gameVersList),
        &(extSig));

    if (FALSE == test)
    {
        return FALSE;
    }

    return TRUE;
}

/**************************************************************/
void
GameVersions_globalClose(
    void)
{
    GameVersList_destroy(
        &(g_gameVersList));
}

/**************************************************************/
int
GameVersions_findMatchingVersion(
    GameVerSig *dummySignature)
{
    int result;

    result = GameVersList_findMatchingVersion(
        &(g_gameVersList),
        dummySignature);

    return result;
}

/**************************************************************/
BOOL
GameVersions_getSignatureByIdx(
    GameVerSigEx **result,
    int idx)
{
    if ((idx < 0) || (idx >= (g_gameVersList.count)))
    {
        return FALSE;
    }

    result[0] = &(g_gameVersList.signatures[idx]);

    return TRUE;
}

/**************************************************************/
