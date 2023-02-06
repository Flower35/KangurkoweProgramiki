/**************************************************************/
#include "GUI.h"
#include "Kao2_DigiTweaks_splash.inc"
#include "ErrorsQueue.h"

/**************************************************************/
BOOL g_GUI_quitTest;
HFONT g_GUI_font;
HWND g_GUI_mainWindow;
HWND g_GUI_listBoxLang1;
HWND g_GUI_listBoxLang2;
HWND g_GUI_listBoxPatches;
void const *(*g_GUI_gameDataCallback)(uint32_t);

/**************************************************************/
void
GUI_globalOpen(
    void const *(*callback)(uint32_t))
{
    g_GUI_quitTest = FALSE;
    g_GUI_font     = NULL;

    g_GUI_mainWindow     = NULL;
    g_GUI_listBoxLang1   = NULL;
    g_GUI_listBoxLang2   = NULL;
    g_GUI_listBoxPatches = NULL;

    g_GUI_gameDataCallback = callback;
}

/**************************************************************/
void
GUI_globalClose(
    void)
{
    if (NULL != g_GUI_font)
    {
        DeleteObject((HGDIOBJ) g_GUI_font);
    }
}

/**************************************************************/
void
GUI_showErrorMessage(
    ReadOnlyUTF8String text)
{
    UTF16String wideTextTest;
    UTF16CodePoint wideTextBuffer[256];

    wideTextTest = convertUTF8to16(
        text,
        (-1),
        wideTextBuffer,
        256);

    if (wideTextBuffer == wideTextTest)
    {
        MessageBoxW(
            g_GUI_mainWindow,
            wideTextBuffer,
            L"[Kao2_DigiTweaks] Error!",
            MB_ICONERROR);
    }
    else
    {
        MessageBoxA(
            g_GUI_mainWindow,
            text,
            "[Kao2_DigiTweaks] Error!",
            MB_ICONERROR);
    }
}

/**************************************************************/
BOOL
GUI_mainLoopIteration(
    void)
{
    BOOL active = TRUE;
    MSG message;

    while (PeekMessage(&(message), NULL, 0, 0, PM_REMOVE))
    {
        if (WM_QUIT == message.message)
        {
            active = FALSE;
        }

        TranslateMessage(&(message));
        DispatchMessage(&(message));
    }

    return ((!g_GUI_quitTest) && active);
}

/**************************************************************/
BOOL
GUI_prepareGraphicalInterface(
    HINSTANCE hInstance)
{
    BOOL test;
    HWND hWnd;
    DWORD wndStyle;
    WNDCLASSEX wndClass;
    RECT wndRect;

    /* Register custom Window Classes */

    ZeroMemory(&(wndClass), sizeof(WNDCLASSEX));
    wndClass.cbSize        = sizeof(WNDCLASSEX);
    wndClass.hInstance     = hInstance;
    wndClass.style         = (CS_OWNDC | CS_VREDRAW | CS_HREDRAW);
    wndClass.hbrBackground = (HBRUSH)(1 + COLOR_BTNFACE);

    wndClass.lpfnWndProc   = GUI_mainWindowProcedure;
    wndClass.lpszClassName = TEXT(GUI_WND_CLASSNAME_A);
    wndClass.hIcon = LoadIcon(hInstance, TEXT("MAINICON"));

    test = RegisterClassEx(&(wndClass));

    if (FALSE == test)
    {
        return FALSE;
    }

    wndClass.lpfnWndProc   = GUI_splashWindowProcedure;
    wndClass.lpszClassName = TEXT(GUI_WND_CLASSNAME_B);
    wndClass.hIcon         = NULL;

    test = RegisterClassEx(&(wndClass));

    if (FALSE == test)
    {
        return FALSE;
    }

  /* Create font */

    g_GUI_font = CreateFont(
        GUI_FONT_HEIGHT,
        0,
        0,
        0,
        FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        (DEFAULT_PITCH | FF_DONTCARE),
        TEXT("Verdana"));

    if (NULL == g_GUI_font)
    {
        return FALSE;
    }

    /* Creating Main Window */

    wndRect.left   = 0;
    wndRect.right  = GUI_WND_WIDTH;
    wndRect.top    = 0;
    wndRect.bottom = GUI_WND_HEIGHT;

    wndStyle = (WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);

    AdjustWindowRect(&(wndRect), wndStyle, FALSE);

    g_GUI_mainWindow = CreateWindow(
        TEXT(GUI_WND_CLASSNAME_A),
        TEXT("Kao2 DigiTweaks"),
        wndStyle,
        (GetSystemMetrics(SM_CXSCREEN) - GUI_WND_WIDTH) / 2,
        (GetSystemMetrics(SM_CYSCREEN) - GUI_WND_HEIGHT) / 2,
        (wndRect.right - wndRect.left),
        (wndRect.bottom - wndRect.top),
        NULL,
        0,
        hInstance,
        NULL);

    if (NULL == g_GUI_mainWindow)
    {
        return FALSE;
    }

    SendMessage(
        g_GUI_mainWindow,
        WM_SETFONT,
        (WPARAM) g_GUI_font,
        (LPARAM) TRUE);

    /* Creating Splash Window */

    hWnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        TEXT(GUI_WND_CLASSNAME_B),
        TEXT(""),
        (WS_VISIBLE | WS_CHILD),
        0,
        0,
        (GUI_WND_WIDTH / 2),
        (GUI_WND_HEIGHT * 2 / 5),
        g_GUI_mainWindow,
        NULL,
        hInstance,
        NULL);

    if (NULL == hWnd)
    {
        return FALSE;
    }

    /* Creating Language Category 1 Label */

    hWnd = CreateWindow(
        TEXT("STATIC"),
        TEXT("Voiceover language:"),
        (WS_VISIBLE | WS_CHILD),
        (GUI_PADDING),
        ((GUI_WND_HEIGHT * 2 / 5) + GUI_PADDING),
        ((GUI_WND_WIDTH / 4) - (2 * GUI_PADDING)),
        (2 * GUI_LABEL_HEIGHT),
        g_GUI_mainWindow,
        NULL,
        hInstance,
        NULL);

    if (NULL == hWnd)
    {
        return FALSE;
    }

    SendMessage(
        hWnd,
        WM_SETFONT,
        (WPARAM) g_GUI_font,
        (LPARAM) TRUE);

    /* Creating Language Category 2 Label */

    hWnd = CreateWindow(
        TEXT("STATIC"),
        TEXT("Subtitles language:"),
        (WS_VISIBLE | WS_CHILD),
        (GUI_PADDING + (GUI_WND_WIDTH / 4)),
        ((GUI_WND_HEIGHT * 2 / 5) + GUI_PADDING),
        ((GUI_WND_WIDTH / 4) - (2 * GUI_PADDING)),
        (2 * GUI_LABEL_HEIGHT),
        g_GUI_mainWindow,
        NULL,
        hInstance,
        NULL);

    if (NULL == hWnd)
    {
        return FALSE;
    }

    SendMessage(
        hWnd,
        WM_SETFONT,
        (WPARAM) g_GUI_font,
        (LPARAM) TRUE);

    /* Creating Language Category 1 ListBox */

    g_GUI_listBoxLang1 = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        TEXT("LISTBOX"),
        TEXT(""),
        (WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_AUTOVSCROLL |
            LBS_HASSTRINGS | LBS_NOTIFY),
        (GUI_PADDING),
        ((GUI_WND_HEIGHT * 2 / 5) + GUI_PADDING + (2 * GUI_LABEL_HEIGHT)),
        ((GUI_WND_WIDTH / 4) - (2 * GUI_PADDING)),
        ((GUI_WND_HEIGHT * 3 / 5) - (2 * GUI_PADDING) - (2 * GUI_LABEL_HEIGHT)),
        g_GUI_mainWindow,
        NULL,
        hInstance,
        NULL);

    if (NULL == g_GUI_listBoxLang1)
    {
        return FALSE;
    }

    SendMessage(
        g_GUI_listBoxLang1,
        WM_SETFONT,
        (WPARAM) g_GUI_font,
        (LPARAM) TRUE);

    /* Creating Language Category 2 ListBox */

    g_GUI_listBoxLang2 = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        TEXT("LISTBOX"),
        TEXT(""),
        (WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_AUTOVSCROLL |
            LBS_HASSTRINGS | LBS_NOTIFY),
        (GUI_PADDING + (GUI_WND_WIDTH / 4)),
        ((GUI_WND_HEIGHT * 2 / 5) + GUI_PADDING + (2 * GUI_LABEL_HEIGHT)),
        ((GUI_WND_WIDTH / 4) - (2 * GUI_PADDING)),
        ((GUI_WND_HEIGHT * 3 / 5) - (2 * GUI_PADDING) - (2 * GUI_LABEL_HEIGHT)),
        g_GUI_mainWindow,
        NULL,
        hInstance,
        NULL);

    if (NULL == g_GUI_listBoxLang2)
    {
        return FALSE;
    }

    SendMessage(
        g_GUI_listBoxLang2,
        WM_SETFONT,
        (WPARAM) g_GUI_font,
        (LPARAM) TRUE);

    /* Creating Patches Label */

    hWnd = CreateWindow(
        TEXT("STATIC"),
        TEXT("Available patches:"),
        (WS_VISIBLE | WS_CHILD),
        (GUI_PADDING + (GUI_WND_WIDTH / 2)),
        (GUI_PADDING),
        ((GUI_WND_WIDTH / 2) - (2 * GUI_PADDING)),
        (GUI_LABEL_HEIGHT),
        g_GUI_mainWindow,
        NULL,
        hInstance,
        NULL);

    if (NULL == hWnd)
    {
        return FALSE;
    }

    SendMessage(
        hWnd,
        WM_SETFONT,
        (WPARAM) g_GUI_font,
        (LPARAM) TRUE);

    /* Creating Patches ListBox */

    g_GUI_listBoxPatches = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        TEXT("LISTBOX"),
        TEXT(""),
        (WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_AUTOVSCROLL |
            LBS_HASSTRINGS | LBS_MULTIPLESEL | LBS_NOTIFY),
        (GUI_PADDING + (GUI_WND_WIDTH / 2)),
        (GUI_PADDING + GUI_LABEL_HEIGHT),
        ((GUI_WND_WIDTH / 2) - (2 * GUI_PADDING)),
        (GUI_WND_HEIGHT - (2 * GUI_PADDING) - GUI_LABEL_HEIGHT),
        g_GUI_mainWindow,
        NULL,
        hInstance,
        NULL);

    if (NULL == g_GUI_listBoxPatches)
    {
        return FALSE;
    }

    SendMessage(
        g_GUI_listBoxPatches,
        WM_SETFONT,
        (WPARAM) g_GUI_font,
        (LPARAM) TRUE);

    /* Done! */

    ShowWindow(g_GUI_mainWindow, SW_SHOW);

    return TRUE;
}

/**************************************************************/
LRESULT CALLBACK
GUI_mainWindowProcedure(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam)
{
    HWND controlHandle;

    switch (Msg)
    {
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            break;
        }

        case WM_CLOSE:
        {
            DestroyWindow(hWnd);
            break;
        }

        case WM_COMMAND:
        {
            controlHandle = (HWND) lParam;

            if ((g_GUI_listBoxLang1 == controlHandle) ||
                (g_GUI_listBoxLang2 == controlHandle) ||
                (g_GUI_listBoxPatches == controlHandle))
            {
                GUI_listBoxesNotifications(controlHandle, HIWORD(wParam));
            }

            break;
        }
    }

    return DefWindowProc(hWnd, Msg, wParam, lParam);
}

/**************************************************************/
LRESULT CALLBACK
GUI_splashWindowProcedure(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam)
{
    BITMAPINFOHEADER bih;
    PAINTSTRUCT ps;
    HDC hdcPaint;

    if (WM_PAINT == Msg)
    {
        ZeroMemory(&(bih), sizeof(BITMAPINFOHEADER));
        bih.biSize        = sizeof(BITMAPINFOHEADER);
        bih.biBitCount    = 24;
        bih.biPlanes      = 1;
        bih.biCompression = BI_RGB;
        bih.biWidth       = SPLASH_IMAGE_WIDTH;
        bih.biHeight      = SPLASH_IMAGE_HEIGHT;

        hdcPaint = BeginPaint(hWnd, &(ps));

        StretchDIBits(
            hdcPaint,
            0,
            0,
            (GUI_WND_WIDTH / 2),
            (GUI_WND_HEIGHT * 2 / 5),
            0,
            0,
            SPLASH_IMAGE_WIDTH,
            SPLASH_IMAGE_HEIGHT,
            SPLASH_PIXELS,
            (BITMAPINFO *) &(bih),
            DIB_RGB_COLORS,
            SRCCOPY);

        EndPaint(hWnd, &(ps));
    }

    return DefWindowProc(hWnd, Msg, wParam, lParam);
}

/**************************************************************/
void
GUI_populateListBoxes(
    void)
{
    int16_t index;
    uint32_t command;
    void const *response;

    /* Get all Language Names */

    index = 0;

    do
    {
        command = DigiTweaks_packCommand(
            DIGITWEAKS_REQUEST_GET,
            DIGITWEAKS_CATEGORY_LANG_SUBS,
            index);

        response = g_GUI_gameDataCallback(command);

        if (NULL != response)
        {
            SendMessage(
                g_GUI_listBoxLang1,
                LB_ADDSTRING,
                0,
                (LPARAM) response);

            SendMessage(
                g_GUI_listBoxLang2,
                LB_ADDSTRING,
                0,
                (LPARAM) response);

            index++;
        }
    }
    while (NULL != response);

    /* Get all Patch Names */

    index = 0;

    do
    {
        command = DigiTweaks_packCommand(
            DIGITWEAKS_REQUEST_GET,
            DIGITWEAKS_CATEGORY_PATCHES,
            index);

        response = g_GUI_gameDataCallback(command);

        if (NULL != response)
        {
            SendMessage(
                g_GUI_listBoxPatches,
                LB_ADDSTRING,
                0,
                (LPARAM) response);

            index++;
        }
    }
    while (NULL != response);

    /* Get Current Languages */

    command = DigiTweaks_packCommand(
        DIGITWEAKS_REQUEST_GET,
        DIGITWEAKS_CATEGORY_LANG_VOICES,
        (-1));

    response = g_GUI_gameDataCallback(command);

    if (NULL != response)
    {
        index = ((intptr_t) response) - 1;

        SendMessage(
            g_GUI_listBoxLang1,
            LB_SETCURSEL,
            index,
            0);
    }

    command = DigiTweaks_packCommand(
        DIGITWEAKS_REQUEST_GET,
        DIGITWEAKS_CATEGORY_LANG_SUBS,
        (-1));

    response = g_GUI_gameDataCallback(command);

    if (NULL != response)
    {
        index = ((intptr_t) response) - 1;

        SendMessage(
            g_GUI_listBoxLang2,
            LB_SETCURSEL,
            index,
            0);
    }
}

/**************************************************************/
void
GUI_listBoxesNotifications(
    HWND const listBox,
    WORD const notification)
{
    long selectionId;
    long selectionState;

    uint8_t request;
    uint8_t category;
    uint32_t command;
    void const *response;

    if (LBN_SELCHANGE == notification)
    {
        selectionId = SendMessage(
            listBox,
            LB_GETCURSEL,
            0,
            0);

        selectionState = SendMessage(
            listBox,
            LB_GETSEL,
            selectionId,
            0);

        if (selectionState > 0)
        {
            request = DIGITWEAKS_REQUEST_SET;
        }
        else
        {
            request = DIGITWEAKS_REQUEST_UNSET;
        }

        if (g_GUI_listBoxLang1 == listBox)
        {
            category = DIGITWEAKS_CATEGORY_LANG_VOICES;
        }
        else if (g_GUI_listBoxLang2 == listBox)
        {
            category = DIGITWEAKS_CATEGORY_LANG_SUBS;
        }
        else if (g_GUI_listBoxPatches == listBox)
        {
            category = DIGITWEAKS_CATEGORY_PATCHES;
        }
        else
        {
            return;
        }

        command = DigiTweaks_packCommand(
            request,
            category,
            (int16_t) selectionId);

        response = g_GUI_gameDataCallback(command);

        if (NULL == response)
        {
            ErrorsQueue_push("ListBox Notification request failed!");
        }
    }
}

/**************************************************************/
