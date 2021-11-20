////////////////////////////////////////////////////////////////
// "Kao2_GameInfo.c"
////////////////////////////////////////////////////////////////
// (2021-11-19)
//  * Program działa jak mikro-trainer. Napisany na bazie
//   "Kao2_TAS.c". Podgląd na pozycję i stan bohatera,
//   podgląd na ID aktualnego poziomu, wszystkie zebrane
//   znajdźki i liczbę wygaszonych aktorów w poziomie.
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// Libraries
////////////////////////////////////////////////////////////////

#define NULL  0  // not C++'s `((void *)0)`, just zero :)

#define _USE_MATH_DEFINES
#include <math.h>

#include <stdlib.h>
#include <string.h>

#include <Windows.h>
#include <psapi.h>

#pragma comment (lib,        "USER32.LIB")
#pragma comment (lib,         "GDI32.LIB")
#pragma comment (linker, "/subsystem:windows")

////////////////////////////////////////////////////////////////
// Const Defines
////////////////////////////////////////////////////////////////

#define KAO2_GAMEINFO_WINDOW_CLASSNAME  "KAO2_GAMEINFO_WINDOW_CLASS"
#define KAO2_GAMEINFO_WINDOW_NAME       "KAO2 :: Game Info"

#define MSGBOX_ERROR_CAPTION  "An epic fail has occurred! :)"

#define IDM_BUTTON_ATTACH  101

#define BUF_SIZE         256
#define LARGE_BUF_SIZE   512
#define MEDIUM_BUF_SIZE   64
#define SMALL_BUF_SIZE    32
#define TINY_BUF_SIZE     16
#define LASTMSG_SIZE    (BUF_SIZE / 2)

#define ENUMERATED_PROCESSES  512

#define KAO2_WINDOW_CLASSNAME  "GLUT"
#define KAO2_WINDOW_NAME       "kangurek Kao: 2ga runda"

const char * KAO2_EXECUTABLE_NAMES[2] =
{
    "kao2.exe", "kao2_mod.exe"
};

////////////////////////////////////////////////////////////////
// KAO2 Addresses and Offsets
////////////////////////////////////////////////////////////////

#define KAO2_LEVEL_ID       0x0062B7D4

#define KAO2_LEVEL_ARRAY    0x0062B7C8
#define KAO2_LEVEL_LANGNAME     0x0008
#define KAO2_LEVEL_FILENAME     0x000C

#define KAO2_PERSISTENT_ACTORS_ARRAY  0x0062B7DC

#define KAO2_COLLECTABLES  0x0062B8F8

#define KAO2_GAMELET       0x006267CC
#define KAO2_GAMELET_HERO      0x038C

#define KAO2_XFORM_SRP  0x0048
#define KAO2_SRP_ROT    0x0000
#define KAO2_SRP_POS    0x0010
#define KAO2_SRP_SCL    0x001C

#define KAO2_ACTOR_SCRIPT    0x0114
#define KAO2_STATE_TERMINAL  0x0009
#define KAO2_STATE_NAME      0x000C
#define KAO2_STATE_DEFAULT   0x0020
#define KAO2_STATE_GADGETS   0x0054

#define KAO2_TIMER_ELAPSED  0x0014

////////////////////////////////////////////////////////////////
// Cool macros
////////////////////////////////////////////////////////////////

#define INVALID_OFFSET ((DWORD) 0xFFFFFFFF)

#define EQ(__a, __b)  ((__a) == (__b))
#define NE(__a, __b)  ((__a) != (__b))

#define IS_NULL(__a)      EQ(                NULL, (__a))
#define NOT_NULL(__a)     NE(                NULL, (__a))
#define IS_INVALID(__a)   EQ(INVALID_HANDLE_VALUE, (__a))
#define NOT_INVALID(__a)  NE(INVALID_HANDLE_VALUE, (__a))
#define VALID_OFFSET(__a) NE(      INVALID_OFFSET, (__a))

#define IS_EITHER(__a, __b, __c)  (EQ(__a, __b) || EQ(__a, __c))

#define FAIL_IF_NULL(__a)  if IS_NULL(__a) { return FALSE; }

#define CHECK_WINDOW_AND_SET_FONT(__hwnd) \
    FAIL_IF_NULL(__hwnd) \
    SendMessage(__hwnd, WM_SETFONT, (WPARAM)GAMEINFO_font, (LPARAM) 0);

#define CREATE_STATIC_WND(__hwnd, __text) \
    __hwnd = CreateWindow("STATIC", __text, WS_VISIBLE | WS_CHILD, \
        x, y, WIDTH_WO_PADDINGS, LABEL_HEIGHT, \
        GAMEINFO_mainWindow, NULL, hInstance, NULL); \
    CHECK_WINDOW_AND_SET_FONT(__hwnd) \
    y += LABEL_HEIGHT + PADDING;

#define CREATE_STATIC_WND_EDGES(__hwnd, __height) \
    __hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, "STATIC", "", \
        WS_VISIBLE | WS_CHILD, x, y, WIDTH_WO_PADDINGS, __height, \
        GAMEINFO_mainWindow, NULL, hInstance, NULL); \
    CHECK_WINDOW_AND_SET_FONT(__hwnd) \
    y += __height + PADDING;

#define CREATE_LABEL_WNDS_PAIR(__index, __desc, __lines) \
    CREATE_STATIC_WND(test_window, __desc) \
    CREATE_STATIC_WND_EDGES(GAMEINFO_labels[__index], LABEL_HEIGHT * __lines)

#define MACRO_KAO2_READ_BYTE(__x, __at) \
    FAIL_IF_NULL(KAO2_readMem(__at, __x, 0x01))

#define MACRO_KAO2_READ_DWORD(__x, __at) \
    FAIL_IF_NULL(KAO2_readMem(__at, __x, 0x04))

#define MACRO_KAO_READ_STRING(__str, __at) \
    FAIL_IF_NULL(KAO2_readMem(__at, &a, 0x04)) \
    FAIL_IF_NULL(KAO2_readMem(a + 0x08, &b, 0x04)) \
    FAIL_IF_NULL(KAO2_readMem(a + 0x0C, __str, b))

////////////////////////////////////////////////////////////////
// Point structures
////////////////////////////////////////////////////////////////

typedef struct ePoint3 ePoint3_t;
struct ePoint3
{
    float x;
    float y;
    float z;
};

VOID ePoint3_set(ePoint3_t * pt, float x, float y, float z)
{
    (pt->x) = x;
    (pt->y) = y;
    (pt->z) = z;
}

VOID ePoint3_reset(ePoint3_t * pt)
{
    ePoint3_set(pt, 0, 0, 0);
}

BOOL ePoint3_eq(ePoint3_t * a, ePoint3_t * b)
{
    return (
        EQ((a->x), (b->x)) &&
        EQ((a->y), (b->y)) &&
        EQ((a->z), (b->z)) );
}

//@ VOID ePoint3_add(ePoint3_t * a, ePoint3_t * b)
//@ {
//@     (a->x) += (b->x);
//@     (a->y) += (b->y);
//@     (a->z) += (b->z);
//@ }

//@ VOID ePoint3_sub(ePoint3_t * a, ePoint3_t * b)
//@ {
//@     (a->x) -= (b->x);
//@     (a->y) -= (b->y);
//@     (a->z) -= (b->z);
//@ }

VOID ePoint3_set_sub(ePoint3_t * a, ePoint3_t * b,  ePoint3_t * c)
{
    (a->x) = (b->x) - (c->x);
    (a->y) = (b->y) - (c->y);
    (a->z) = (b->z) - (c->z);
}

FLOAT ePoint3_length(ePoint3_t * pt)
{
    return sqrt (
        (pt->x) * (pt->x) +
        (pt->y) * (pt->y) +
        (pt->z) * (pt->z) );
}

////////////////////////////////////////////////////////////////
// Globals
////////////////////////////////////////////////////////////////

BOOL g_quit;

char g_lastMessage[LASTMSG_SIZE];

HWND GAMEINFO_mainWindow;
HWND GAMEINFO_labels[3];
HFONT GAMEINFO_font;

HWND KAO2_gameWindow;
HANDLE KAO2_gameHandle;

ePoint3_t g_heroLastPos;
ePoint3_t g_heroLastVel;
DWORD g_heroLastState;

DWORD g_lastLevelID;
DWORD g_lastCollectables[3];
DWORD g_lastPersistentActors[2];

////////////////////////////////////////////////////////////////
// TOOL-ASSISTED MANIPULATION: Write bytes to Kao2 game process
////////////////////////////////////////////////////////////////

BOOL KAO2_writeMem(DWORD address, LPCVOID from, size_t length)
{
    if (!WriteProcessMemory(KAO2_gameHandle, (LPVOID)address, from, length, NULL))
    {
        sprintf_s(g_lastMessage, LASTMSG_SIZE, "Could not write %d bytes to address 0x%08X!", length, address);

        return FALSE;
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////
// TOOL-ASSISTED MANIPULATION: Read bytes from Kao2 game process
////////////////////////////////////////////////////////////////

BOOL KAO2_readMem(DWORD address, LPVOID into, size_t length)
{
    if (!ReadProcessMemory(KAO2_gameHandle, (LPVOID)address, into, length, NULL))
    {
        sprintf_s(g_lastMessage, LASTMSG_SIZE, "Could not read %d bytes from address 0x%08X!", length, address);

        return FALSE;
    }

    return TRUE;
}


////////////////////////////////////////////////////////////////
// TOOL-ASSISTED MANIPULATION: Read data from list of offsets
////////////////////////////////////////////////////////////////

BOOL KAO2_readSomeData(LPVOID into, size_t length, ...)
{
    va_list args;
    DWORD offset, address = 0;

    va_start(args, length);

    while VALID_OFFSET(offset = va_arg(args, DWORD))
    {
        if IS_NULL(KAO2_readMem(address + offset, &address, 0x04))
        {
            va_end(args);
            return FALSE;
        }
    }

    va_end(args);

    if (0 == length)
    {
        (* ((LPDWORD)into)) = address;
    }
    else
    {
        FAIL_IF_NULL(KAO2_readMem(address, into, length))
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////
// KAO2 PROCESS HOOKING: Match possible EXE filename
////////////////////////////////////////////////////////////////

BOOL KAO2_matchGameExecutableName(const char * filename)
{
    const int NAMES = sizeof(KAO2_EXECUTABLE_NAMES) / sizeof(const char *);

    for (int i = 0; i < NAMES; i++)
    {
        if IS_NULL(strcmp(filename, KAO2_EXECUTABLE_NAMES[i]))
        {
            return TRUE;
        }
    }

    return FALSE;
}

////////////////////////////////////////////////////////////////
// KAO2 PROCESS HOOKING: Match the window with process ID
////////////////////////////////////////////////////////////////

BOOL KAO2_matchGameWindowWithProccess(const HWND found_window, const DWORD found_pid)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(found_window, &pid);

    return EQ(found_pid, pid);
}

////////////////////////////////////////////////////////////////
// KAO2 PROCESS HOOKING: Find the right process
//  and return special handle on success
////////////////////////////////////////////////////////////////

VOID KAO2_findGameProcess()
{
    HANDLE proc_handle;
    HWND glut_window;
    DWORD pids[ENUMERATED_PROCESSES], i, count;
    char buf[BUF_SIZE], * file_name, * p;

    /* Find game window */
    if IS_NULL(glut_window = FindWindow(KAO2_WINDOW_CLASSNAME, KAO2_WINDOW_NAME))
    {
        sprintf_s(g_lastMessage, LASTMSG_SIZE, "Could not locate \"%s\" window!", KAO2_WINDOW_NAME);
    }
    else
    {
        /* Enumerate processes */
        if (!EnumProcesses(pids, (sizeof(DWORD) * ENUMERATED_PROCESSES), &count))
        {
            sprintf_s(g_lastMessage, LASTMSG_SIZE, "EnumProcesses() failed!");
        }
        else
        {
            for (i = 0; i < count; i++)
            {
                proc_handle = OpenProcess((PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION ), FALSE, pids[i]);

                if (NOT_NULL(proc_handle) && NOT_INVALID(proc_handle))
                {
                    if (KAO2_matchGameWindowWithProccess(glut_window, pids[i]))
                    {
                        buf[0] = '\0';
                        GetModuleFileNameEx(proc_handle, NULL, buf, BUF_SIZE);
                        buf[BUF_SIZE - 1] = '\0';
                        file_name = NULL;

                        for (p = buf; (*p); p++)
                        {
                            if IS_EITHER((*p), '/', '\\')
                            {
                                file_name = p + 1;
                            }
                        }

                        if (file_name && KAO2_matchGameExecutableName(file_name))
                        {
                            KAO2_gameWindow = glut_window;
                            KAO2_gameHandle = proc_handle;
                            return;
                        }
                    }

                    CloseHandle(proc_handle);
                }
            }

            sprintf_s(g_lastMessage, LASTMSG_SIZE, "No process module filename matches the expected executable name!");
        }
    }

    KAO2_gameWindow = NULL;
    KAO2_gameHandle = INVALID_HANDLE_VALUE;
}

////////////////////////////////////////////////////////////////
// HELPER: printing constant-length floating-point numbers
////////////////////////////////////////////////////////////////

char * HELPER_fpText(char * buf, float value)
{
    char digit_before_dot;
    int int_part = (int) value;
    int decimal_index;

    /* Start with the sign and one space */
    buf[0] = (value > 0) ? '+' : ( (value < 0) ? '-' : ' ' );
    buf[1] = ' ';

    /* Left part (digits + NULL) */
    const int left_size = 6 + 1;

    sprintf_s
    (
        buf + 2, left_size,
        "%6d", (int_part >= 0) ? int_part : (-int_part)
    );

    decimal_index = strlen(buf) - 1;
    digit_before_dot = buf[decimal_index];

    /* Right part ("0." + digits + NULL) */
    const int right_size = 2 + 5 + 1;

    /* Disabling FPU SIGN bit before FMOD */
    (*(LPDWORD)(&value)) &= (~ 0x80000000);

    sprintf_s
    (
        buf + decimal_index, right_size,
        "%.5f", (0 != value) ? fmod((double)value, 1.0) : 0.0
    );

    buf[decimal_index] = digit_before_dot;

    return buf;
}

////////////////////////////////////////////////////////////////
// WINAPI GUI: Show quick text status
////////////////////////////////////////////////////////////////

VOID GAMEINFO_showStatus(DWORD label_id, const char * msg)
{
    SetWindowText(GAMEINFO_labels[label_id], msg);
    /* UpdateWindow(GAMEINFO_mainWindow); */
}

////////////////////////////////////////////////////////////////
// TOOL-ASSISTED INFO: Read xforms's position
////////////////////////////////////////////////////////////////

BOOL GAMEINFO_readXformPos(ePoint3_t * pos, DWORD xform)
{
    DWORD address = xform + KAO2_XFORM_SRP + KAO2_SRP_POS;

    MACRO_KAO2_READ_DWORD(&(pos->x), address + 0x00)
    MACRO_KAO2_READ_DWORD(&(pos->y), address + 0x04)
    MACRO_KAO2_READ_DWORD(&(pos->z), address + 0x08)

    return TRUE;
}

////////////////////////////////////////////////////////////////
// TOOL-ASSISTED INFO: Read actor's full state name
////////////////////////////////////////////////////////////////

BOOL GAMEINFO_readActorStateName(char * str_final, DWORD * state)
{
    DWORD a, b, prev_state = NULL;
    char str_combine[BUF_SIZE];
    char state_name[MEDIUM_BUF_SIZE];

    str_final[0] = '\0';

    while NOT_NULL(*state)
    {
        MACRO_KAO_READ_STRING(state_name, ((*state) + KAO2_STATE_NAME))
        sprintf_s(str_combine, BUF_SIZE, "%s::%s", str_final, state_name);
        strcpy_s(str_final, BUF_SIZE, str_combine);

        prev_state = (*state);
        MACRO_KAO2_READ_DWORD(state, ((*state) + KAO2_STATE_DEFAULT))
    }

    (*state) = prev_state;
    return TRUE;
}

////////////////////////////////////////////////////////////////
// TOOL-ASSISTED INFO: Is actor terminated?
////////////////////////////////////////////////////////////////

BOOL GAMEINFO_isActorTerminated(DWORD array, DWORD id)
{
    DWORD actor, state;

    MACRO_KAO2_READ_DWORD(&actor, (array + 0x04 * id))

    if NOT_NULL(actor)
    {
        MACRO_KAO2_READ_DWORD(&state, (actor + KAO2_ACTOR_SCRIPT))

        while NOT_NULL(state)
        {
            MACRO_KAO2_READ_BYTE(&actor, (state + KAO2_STATE_TERMINAL))

            if (0x00FF & actor)
            {
                return TRUE;
            }

            MACRO_KAO2_READ_DWORD(&state, (state + KAO2_STATE_DEFAULT))
        }
    }

    return FALSE;
}

////////////////////////////////////////////////////////////////
// TOOL-ASSISTED INFO: Update gameplay
////////////////////////////////////////////////////////////////

BOOL GAMEINFO_updateGameplay()
{
    char buf[LARGE_BUF_SIZE], level_name[MEDIUM_BUF_SIZE];
    DWORD a, b, level_id, levels_count, level_struct,
        collectables[3], persistent_actors[3];
    BOOL changed = FALSE;
    BYTE terminated;

    /* Level info */

    MACRO_KAO2_READ_DWORD(&level_id, KAO2_LEVEL_ID);
    MACRO_KAO2_READ_DWORD(&levels_count, KAO2_LEVEL_ARRAY);

    if ((level_id >= 0) && (level_id < levels_count))
    {
        KAO2_readSomeData
        (
            &level_struct, NULL,
            (KAO2_LEVEL_ARRAY + 0x08), (0x04 * level_id),
            INVALID_OFFSET
        );

        MACRO_KAO_READ_STRING(level_name, (level_struct + KAO2_LEVEL_LANGNAME))

        if NE(g_lastLevelID, level_id)
        {
            changed = TRUE;
            g_lastLevelID = level_id;
        }
    }
    else
    {
        level_name[0] = '\0';
    }

    /* Persistent actors */

    MACRO_KAO2_READ_DWORD(&(persistent_actors[1]), KAO2_PERSISTENT_ACTORS_ARRAY);
    MACRO_KAO2_READ_DWORD(&(persistent_actors[2]), (KAO2_PERSISTENT_ACTORS_ARRAY + 0x08));

    persistent_actors[0] = 0;
    if NOT_NULL(persistent_actors[2])
    {
        for (a = 0; a < persistent_actors[1]; a++)
        {
            if (GAMEINFO_isActorTerminated(persistent_actors[2], a))
            {
                persistent_actors[0] ++;
            }
        }
    }
    else
    {
        persistent_actors[1] = 0;
    }

    for (a = 0; a < 2; a++)
    {
        if NE(persistent_actors[a], g_lastPersistentActors[a])
        {
            changed = TRUE;
            g_lastPersistentActors[a] = persistent_actors[a];
        }
    }

    /* Collectables */

    for (a = 0; a < 3; a++)
    {
        MACRO_KAO2_READ_DWORD(&(collectables[a]), (KAO2_COLLECTABLES + 0x04 * a))

        if NE(collectables[a], g_lastCollectables[a])
        {
            changed = TRUE;
            g_lastCollectables[a] = collectables[a];
        }
    }

    /* If anything changed */

    if (changed)
    {
        sprintf_s
        (
            buf, LARGE_BUF_SIZE,
                "level_%02d: \"%s\"\n\n"
                "total collectables: ( %4d ) ( %3d ) ( %3d )\n"
                "terminated persistent actors: %3d of %3d",
            level_id, level_name,
            collectables[0], collectables[1], collectables[2],
            persistent_actors[0], persistent_actors[1]
        );

        GAMEINFO_showStatus(1, buf);
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////
// TOOL-ASSISTED INFO: Update hero
////////////////////////////////////////////////////////////////

BOOL GAMEINFO_updateHero()
{
    char buf[LARGE_BUF_SIZE];
    char str_state[BUF_SIZE];
    char fp[7][TINY_BUF_SIZE];
    BOOL changed = FALSE;
    ePoint3_t pos, vel;
    DWORD hero_actor, hero_state;

    /* Hero instance */

    KAO2_readSomeData
    (
        &hero_actor, NULL,
        KAO2_GAMELET, KAO2_GAMELET_HERO,
        INVALID_OFFSET
    );

    /* Hero state & Hero position */

    MACRO_KAO2_READ_DWORD(&hero_state, hero_actor + KAO2_ACTOR_SCRIPT)

    FAIL_IF_NULL(GAMEINFO_readXformPos(&pos, hero_actor))
    FAIL_IF_NULL(GAMEINFO_readActorStateName(str_state, &hero_state))

    if NE(hero_state, g_heroLastState)
    {
        changed = TRUE;
        g_heroLastState = hero_state;
    }

    if (ePoint3_eq(&pos, &g_heroLastPos))
    {
        vel = g_heroLastVel;
    }
    else
    {
        ePoint3_set_sub(&vel, &pos, &g_heroLastPos);

        changed = TRUE;
        g_heroLastPos = pos;
        g_heroLastVel = vel;
    }

    /* If anything changed */

    if (changed)
    {
        sprintf_s
        (
            buf, LARGE_BUF_SIZE,
                "position: ( %s ) ( %s ) ( %s )\n"
                "velocity: ( %s ) ( %s ) ( %s ) ( %s )\n\n"
                "state: \"%s\"\n",
            HELPER_fpText(fp[0], pos.x), HELPER_fpText(fp[1], pos.y), HELPER_fpText(fp[2], pos.z),
            HELPER_fpText(fp[3], vel.x), HELPER_fpText(fp[4], vel.y), HELPER_fpText(fp[5], vel.z),
            HELPER_fpText(fp[6], ePoint3_length(&vel)),
            str_state
        );

        GAMEINFO_showStatus(2, buf);
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////
// TOOL-ASSISTED INFO: Update all
////////////////////////////////////////////////////////////////

VOID GAMEINFO_update()
{
    GAMEINFO_updateGameplay();
    GAMEINFO_updateHero();
}

////////////////////////////////////////////////////////////////
// WINAPI GUI: Main window loop
////////////////////////////////////////////////////////////////

BOOL GAMEINFO_windowLoop()
{
    BOOL still_active = TRUE;
    MSG message = {0};

    while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
    {
        if EQ(WM_QUIT, message.message)
        {
            still_active = FALSE;
        }

        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return ((!g_quit) && still_active);
}

////////////////////////////////////////////////////////////////
// WINAPI GUI: Show error message
////////////////////////////////////////////////////////////////

VOID GAMEINFO_iAmError(const char * caption)
{
    char buf[BUF_SIZE];

    GAMEINFO_showStatus(0, caption);

    if NOT_NULL(g_lastMessage[0])
    {
        sprintf_s(buf, BUF_SIZE, "%s\n\n%s", caption, g_lastMessage);

        MessageBox(GAMEINFO_mainWindow, buf, MSGBOX_ERROR_CAPTION, MB_ICONERROR);
    }
}

////////////////////////////////////////////////////////////////
// WINAPI GUI: Callback procedure for the main window of this tool
////////////////////////////////////////////////////////////////

LRESULT CALLBACK GAMEINFO_windowProcedure(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
        /* Main window destroyed */
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            g_quit = TRUE;

            break;
        }

        /* Main window closed */
        case WM_CLOSE:
        {
            DestroyWindow(hWnd);

            break;
        }

        /* Command from some child control */
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                /* Attach KAO2 game process */
                case IDM_BUTTON_ATTACH:
                {
                    if EQ(BN_CLICKED, HIWORD(wParam))
                    {
                        KAO2_findGameProcess();

                        if IS_INVALID(KAO2_gameHandle)
                        {
                            GAMEINFO_iAmError("Could not attach the \"kao2.exe\" game process!");
                        }
                        else
                        {
                            GAMEINFO_showStatus(0, "KAO2 attached. ^^");
                        }

                        return 0;
                    }

                    break;
                }
            }

            break;
        }
    }

    return DefWindowProc(hWnd, Msg, wParam, lParam);
}

////////////////////////////////////////////////////////////////
// WINAPI GUI: Creating all windows and controls
////////////////////////////////////////////////////////////////

BOOL GAMEINFO_createWindows(HINSTANCE hInstance)
{
    HWND test_window;
    WNDCLASSEX window_class;
    RECT real_window_rect;

    const int PADDING = 8;
    const int FONT_HEIGHT = 14;
    const int LABEL_HEIGHT = FONT_HEIGHT + 4;
    const int BUTTON_HEIGHT = 24;

    const int WINDOW_WIDTH  = 640;
    const int WINDOW_HEIGHT = 480;
    const int WIDTH_WO_PADDINGS = WINDOW_WIDTH - 2 * PADDING;

    LONG i, j, x = PADDING, y = PADDING, x2, y2;

    /* Register Window */

    ZeroMemory(&(window_class), sizeof(WNDCLASSEX));
    window_class.cbSize = sizeof(WNDCLASSEX);
    window_class.hInstance = hInstance;
    window_class.style = (CS_OWNDC | CS_VREDRAW | CS_HREDRAW);
    window_class.hbrBackground = (HBRUSH)(1 + COLOR_BTNFACE);
    window_class.lpfnWndProc = GAMEINFO_windowProcedure;
    window_class.lpszClassName = KAO2_GAMEINFO_WINDOW_CLASSNAME;

    FAIL_IF_NULL(RegisterClassEx(&window_class))

    /* Create font */

    GAMEINFO_font = CreateFont
    (
        FONT_HEIGHT, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, (DEFAULT_PITCH | FF_DONTCARE),
        "Consolas"
    );

    FAIL_IF_NULL(GAMEINFO_font)

    /* Create Main Window */

    real_window_rect.left   = 0;
    real_window_rect.right  = WINDOW_WIDTH;
    real_window_rect.top    = 0;
    real_window_rect.bottom = WINDOW_HEIGHT;

    i = (WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);
    AdjustWindowRect(&real_window_rect, i, FALSE);

    real_window_rect.right  -= real_window_rect.left;
    real_window_rect.bottom -= real_window_rect.top;

    GAMEINFO_mainWindow = CreateWindow
    (
        KAO2_GAMEINFO_WINDOW_CLASSNAME, KAO2_GAMEINFO_WINDOW_NAME, i,
        (GetSystemMetrics(SM_CXSCREEN) - WINDOW_WIDTH) / 2,
        (GetSystemMetrics(SM_CYSCREEN) - WINDOW_HEIGHT) / 2,
        real_window_rect.right, real_window_rect.bottom,
        NULL, 0, hInstance, NULL
    );

    CHECK_WINDOW_AND_SET_FONT(GAMEINFO_mainWindow);

    /* Static title */

    test_window = CreateWindow
    (
        "STATIC", "[Kao the Kangroo: Round 2] :: [Game Info]",
        WS_VISIBLE | WS_CHILD,
        x, y, WIDTH_WO_PADDINGS, LABEL_HEIGHT,
        GAMEINFO_mainWindow, NULL, hInstance, NULL
    );

    CHECK_WINDOW_AND_SET_FONT(test_window);
    y += (LABEL_HEIGHT + PADDING);

    /* Game attachment button */

    test_window = CreateWindow
    (
        "BUTTON", "Attach <KAO2> game process",
        WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        x, y, WIDTH_WO_PADDINGS, BUTTON_HEIGHT,
        GAMEINFO_mainWindow, (HMENU)IDM_BUTTON_ATTACH, hInstance, NULL
    );

    CHECK_WINDOW_AND_SET_FONT(test_window);
    y += (BUTTON_HEIGHT + PADDING);

    /* Label for general status */

    CREATE_STATIC_WND_EDGES(GAMEINFO_labels[0], 2 * LABEL_HEIGHT)

    /* Gameplay info */

    CREATE_LABEL_WNDS_PAIR(1, "Gameplay", 4)

    /* Hero info */

    CREATE_LABEL_WNDS_PAIR(2, "Hero (Kao)", 4)

    /* Default text */

    GAMEINFO_showStatus(0, "Start by launching \"Kao the Kangaroo: Round 2\".");

    ShowWindow(GAMEINFO_mainWindow, SW_SHOW);
    return TRUE;
}

////////////////////////////////////////////////////////////////
// WINAPI: Entry point of the application
////////////////////////////////////////////////////////////////

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    DWORD i;

    /* Reseting global stuff */

    g_quit = FALSE;

    GAMEINFO_mainWindow = NULL;
    GAMEINFO_labels[0]  = NULL;
    GAMEINFO_labels[1]  = NULL;
    GAMEINFO_labels[2]  = NULL;

    KAO2_gameWindow = NULL;
    KAO2_gameHandle = INVALID_HANDLE_VALUE;

    ePoint3_reset(&g_heroLastPos);
    ePoint3_reset(&g_heroLastVel);
    g_heroLastState = NULL;

    g_lastLevelID = (-1);
    g_lastCollectables[0] = (-1);
    g_lastCollectables[1] = (-1);
    g_lastCollectables[2] = (-1);
    g_lastPersistentActors[0] = (-1);
    g_lastPersistentActors[1] = (-1);

    /* Starting the program */

    if (!GAMEINFO_createWindows(hInstance))
    {
        MessageBox(NULL, "Could not create the application window!", MSGBOX_ERROR_CAPTION, MB_ICONERROR);
    }
    else
    {
        while (GAMEINFO_windowLoop())
        {
            if (NOT_INVALID(KAO2_gameHandle) && GetExitCodeProcess(KAO2_gameHandle, &i))
            {
                if NE(STILL_ACTIVE, i)
                {
                    KAO2_gameWindow = NULL;
                    KAO2_gameHandle = INVALID_HANDLE_VALUE;

                    GAMEINFO_showStatus(0, "Game has been closed! Please re-launch \"Kao the Kangaroo: Round 2\".");
                }
                else
                {
                    GAMEINFO_update();
                }
            }
        }
    }

    /* Ending the program */

    if NOT_NULL(GAMEINFO_font)
    {
        DeleteObject((HGDIOBJ)GAMEINFO_font);
    }

    if NOT_INVALID(KAO2_gameHandle)
    {
        CloseHandle(KAO2_gameHandle);
    }

    return 0;
}

////////////////////////////////////////////////////////////////
