/**************************************************************/
#ifndef HEADER_KAO2_DIGITWEAKS_TEXTHELPER
#define HEADER_KAO2_DIGITWEAKS_TEXTHELPER

/**************************************************************/
#include "Kao2_DigiTweaks.h"

/**************************************************************/
typedef char     UTF8CodePoint;
typedef uint16_t UTF16CodePoint;
typedef UTF8CodePoint*  UTF8String;
typedef UTF16CodePoint* UTF16String;
typedef UTF8CodePoint  const* ReadOnlyUTF8String;
typedef UTF16CodePoint const* ReadOnlyUTF16String;

/**************************************************************/
extern BOOL
stringStartsWith(
    ReadOnlyUTF8String strA,
    ReadOnlyUTF8String strB);

/**************************************************************/
/**
 * @brief Zwraca NULL-terminated tekst, skonwertowany
 *  z zapisu UTF-8 (multibajtowy) na UTF-16 (dwubajtowy).
 *
 * @param inputText Tekst wejściowy (UTF-8).
 * @param inputLengthInBytes Rozmiar tekstu wejściowego w bajtach.
 *  (-1) jeżeli tekst ma być traktowany jako NULL-terminated.
 * @param outputBuffer Zaalokowany wcześniej bufor wyjściowy (UTF-16).
 * @param outputLengthInWideChars Maksymalna długość bufora wyjściowego
 *  (wliczając NULL-termination char), wyrażona w dwubajtowych słowach.
 *
 * @return Adres `outputBuffer` dla poprawnej konwersji;
 *  NULL jeśli nie udało się skonwertować tekstu z UTF-8 na UTF-16,
 *  lub jeśli bufor docelowy jest za mały
 *  (znak NULL również musi się zmieścić!).
 */
extern UTF16String
convertUTF8to16(
    ReadOnlyUTF8String inputText,
    int inputLengthInBytes,
    UTF16String outputBuffer,
    int outputLengthInWideChars);

/**************************************************************/
/**
 * @brief Zwraca NULL-terminated tekst, skonwertowany
 *  z zapisu UTF-16 (dwubajtowy) na UTF-8 (multibajtowy).
 *
 * @param inputText Tekst wejściowy (UTF-16).
 * @param inputLengthInWideChars Rozmiar tekstu wejściowego, wyrażony
 * w dwubajtowych słowach. (-1) jeżeli tekst ma być traktowany
 * jako NULL-terminated.
 * @param outputBuffer Zaalokowany wcześniej bufor wyjściowy (UTF-8).
 * @param outputLengthInBytes Maksymalna długość bufora wyjściowego
 *  (wliczając NULL-termination char), wyrażona w bajtach.
 *
 * @return Adres `outputBuffer` dla poprawnej konwersji;
 *  NULL jeśli nie udało się skonwertować tekstu z UTF-16 na UTF-8,
 *  lub jeśli bufor docelowy jest za mały
 *  (znak NULL również musi się zmieścić!).
 */
extern UTF8String
convertUTF16to8(
    ReadOnlyUTF16String inputText,
    int inputLengthInWideChars,
    UTF8String outputBuffer,
    int outputLengthInBytes);

/**************************************************************/
extern ReadOnlyUTF8String
endOfUTF8String(
    ReadOnlyUTF8String text);


/**************************************************************/
#endif  /* HEADER_KAO2_DIGITWEAKS_TEXTHELPER */

/**************************************************************/
