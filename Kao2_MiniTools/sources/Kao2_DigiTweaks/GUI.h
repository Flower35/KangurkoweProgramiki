/**************************************************************/
#ifndef HEADER_KAO2_DIGITWEAKS_GUI
#define HEADER_KAO2_DIGITWEAKS_GUI

/**************************************************************/
#include "Kao2_DigiTweaks.h"
#include "TextHelper.h"

/**************************************************************/
#define GUI_WND_CLASSNAME_A  "MainWindowClass"
#define GUI_WND_CLASSNAME_B  "SplashWindowClass"

#define GUI_WND_WIDTH    640
#define GUI_WND_HEIGHT   480
#define GUI_PADDING       16
#define GUI_FONT_HEIGHT   24
#define GUI_LABEL_HEIGHT  28

/**************************************************************/
extern BOOL g_GUI_quitTest;
extern HFONT g_GUI_font;
extern HWND g_GUI_mainWindow;
extern HWND g_GUI_listBoxLang1;
extern HWND g_GUI_listBoxLang2;
extern HWND g_GUI_listBoxPatches;
extern void const *(*g_GUI_gameDataCallback)(uint32_t);

/**************************************************************/
extern void
GUI_globalOpen(
    void const *(*callback)(uint32_t));

/**************************************************************/
extern void
GUI_globalClose(
    void);

/**************************************************************/
extern void
GUI_showErrorMessage(
    ReadOnlyUTF8String text);

/**************************************************************/
extern BOOL
GUI_mainLoopIteration(
    void);

/**************************************************************/
extern BOOL
GUI_prepareGraphicalInterface(
    HINSTANCE hInstance);

/**************************************************************/
extern LRESULT CALLBACK
GUI_mainWindowProcedure(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam);

/**************************************************************/
extern LRESULT CALLBACK
GUI_splashWindowProcedure(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam);

/**************************************************************/
extern void
GUI_populateListBoxes(
    void);

/**************************************************************/
extern void
GUI_listBoxesNotifications(
    HWND const listBox,
    WORD const notification);

/**************************************************************/
#endif  /* HEADER_KAO2_DIGITWEAKS_GUI */

/**************************************************************/
