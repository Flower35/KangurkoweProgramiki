/**************************************************************/
#ifndef HEADER_KAO2_DIGITWEAKS_ERRORSQUEUE
#define HEADER_KAO2_DIGITWEAKS_ERRORSQUEUE

/**************************************************************/
#include "Kao2_DigiTweaks.h"
#include "TextObjectList.h"

/**************************************************************/
extern BOOL g_errorsQAvail;
extern TextObjectList g_errorsQ;

/**************************************************************/
extern BOOL
ErrorsQueue_globalOpen(
    void);

/**************************************************************/
extern void
ErrorsQueue_globalClose(
    void);

/**************************************************************/
extern void
ErrorsQueue_push(
    ReadOnlyUTF8String text);

/**************************************************************/
extern void
ErrorsQueue_dropBy(
    void);

/**************************************************************/
#endif  /* HEADER_KAO2_DIGITWEAKS_ERRORSQUEUE */

/**************************************************************/
