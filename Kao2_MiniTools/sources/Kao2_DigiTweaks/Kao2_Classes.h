/**************************************************************/
#ifndef HEADER_KAO2_DIGITWEAKS_KAO2CLASES
#define HEADER_KAO2_DIGITWEAKS_KAO2CLASES

/**************************************************************/
#include "Kao2_DigiTweaks.h"

/**************************************************************/
typedef struct Kao2_ArrayTag Kao2_Array;

#pragma pack(push)
#pragma pack(4)

struct Kao2_ArrayTag
{
    int32_t count;
    int32_t maxAlloc;
    uint32_t arrayPointer;
};

#pragma pack(pop)

/**************************************************************/
typedef struct Kao2_StringHeaderTag Kao2_StringHeader;

#pragma pack(push)
#pragma pack(4)

struct Kao2_StringHeaderTag
{
    int32_t refCount;
    int32_t length;
    int32_t allocatedLength;
};

#pragma pack(pop)

/**************************************************************/
typedef struct Kao2_StringTag Kao2_String;

#pragma pack(push)
#pragma pack(4)

struct Kao2_StringTag
{
    Kao2_StringHeader header;
    char text[1];
};

#pragma pack(pop)

/**************************************************************/
typedef struct Kao2_LangTag Kao2_Lang;

#pragma pack(push)
#pragma pack(4)

struct Kao2_LangTag
{
    uint32_t vptr;
    Kao2_Array langs;
    uint32_t currentVoicesLang;
    Kao2_Array flatTags;
    int32_t currentSubsLang;
};

#pragma pack(pop)

/**************************************************************/
#endif  /* HEADER_KAO2_DIGITWEAKS_KAO2CLASES */

/**************************************************************/
