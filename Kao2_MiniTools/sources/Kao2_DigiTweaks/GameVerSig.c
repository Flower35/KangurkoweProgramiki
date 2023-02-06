/**************************************************************/
#include "GameVerSig.h"

/**************************************************************/
BOOL
GameVerSig_compare(
    GameVerSig *self,
    GameVerSig *other)
{
    if ((self->unixTimestamp) != (other->unixTimestamp))
    {
        return FALSE;
    }

    if ((self->entryPoint) != (other->entryPoint))
    {
        return FALSE;
    }

    if ((self->imageSize) != (other->imageSize))
    {
        return FALSE;
    }

    return TRUE;
}

/**************************************************************/
