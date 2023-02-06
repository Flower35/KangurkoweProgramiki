/**************************************************************/
#ifndef HEADER_KAO2_DIGITWEAKS_GAMEVERSLIST
#define HEADER_KAO2_DIGITWEAKS_GAMEVERSLIST

/**************************************************************/
#include "Kao2_DigiTweaks.h"
#include "GameVerSigEx.h"

/**************************************************************/
typedef struct GameVersListTag GameVersList;

struct GameVersListTag
{
    int count;
    int allocated;
    GameVerSigEx *signatures;
};

/**************************************************************/
extern BOOL
GameVersList_init(
    GameVersList *self);

/**************************************************************/
extern void
GameVersList_destroy(
    GameVersList *self);

/**************************************************************/
extern BOOL
GameVersList_register(
    GameVersList *self,
    GameVerSigEx *extendedSignature);

/**************************************************************/
extern int
GameVersList_findMatchingVersion(
    GameVersList *self,
    GameVerSig *dummySignature);

/**************************************************************/
#endif  /* HEADER_KAO2_DIGITWEAKS_GAMEVERSLIST */

/**************************************************************/
