/**************************************************************/
#ifndef HEADER_KAO2_DIGITWEAKS_GAMEPROCESS
#define HEADER_KAO2_DIGITWEAKS_GAMEPROCESS

/**************************************************************/
#include "Kao2_DigiTweaks.h"

/**************************************************************/
typedef struct GameProcessTag GameProcess;
struct GameProcessTag
{
    int32_t gameVersion;
    HANDLE processHandle;
    uint32_t imageBase;
};

/**************************************************************/
extern void
GameProcess_init(
    GameProcess *self);

/**************************************************************/
extern void
GameProcess_detach(
    GameProcess *self);

/**************************************************************/
extern BOOL
GameProcess_isActive(
    GameProcess *self);

/**************************************************************/
extern BOOL
GameProcess_getModuleInfo(
    GameProcess *self,
    MODULEINFO *resultInfo);

/**************************************************************/
extern BOOL
GameProcess_read(
    GameProcess *self,
    void *destination,
    uint32_t gameAddress,
    int32_t size);

/**************************************************************/
extern BOOL
GameProcess_write(
    GameProcess *self,
    void const *source,
    uint32_t gameAddress,
    int32_t size);

/**************************************************************/
extern BOOL
GameProcess_readUInt32(
    GameProcess *self,
    uint32_t *destination,
    uint32_t gameAddress);

/**************************************************************/
extern BOOL
GameProcess_writeUInt32(
    GameProcess *self,
    uint32_t const *source,
    uint32_t gameAddress);

/**************************************************************/
extern BOOL
GameProcess_readUnixTimestamp(
    GameProcess *self,
    uint32_t *resultTimestamp);

/**************************************************************/
#endif  /* HEADER_KAO2_DIGITWEAKS_GAMEPROCESS */

/**************************************************************/
