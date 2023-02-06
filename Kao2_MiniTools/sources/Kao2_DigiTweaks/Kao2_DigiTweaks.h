/**************************************************************/
#ifndef HEADER_KAO2_DIGITWEAKS
#define HEADER_KAO2_DIGITWEAKS

/**************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <Windows.h>
#include <psapi.h>

/**************************************************************/
#define DIGITWEAKS_REQUEST_GET    0
#define DIGITWEAKS_REQUEST_SET    1
#define DIGITWEAKS_REQUEST_UNSET  2

/**************************************************************/
#define DIGITWEAKS_CATEGORY_LANG_VOICES  0
#define DIGITWEAKS_CATEGORY_LANG_SUBS    1
#define DIGITWEAKS_CATEGORY_PATCHES      2

/**************************************************************/
extern uint32_t
DigiTweaks_packCommand(
    uint8_t request,
    uint8_t category,
    int16_t index);

/**************************************************************/
extern void
DigiTweaks_unpackCommand(
    uint32_t command,
    uint8_t *request,
    uint8_t *category,
    int16_t *index);

/**************************************************************/
#endif  /* HEADER_KAO2_DIGITWEAKS */

/**************************************************************/
