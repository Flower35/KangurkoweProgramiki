////////////////////////////////////////////////////////////////
// "Kao2_TAS.c"
////////////////////////////////////////////////////////////////
// (2021-11-08)
//  * Wstępny kod - attach process, lista jednostronna,
//   algorytm Replaying.
// (2021-11-09)
//  * Udoskonalne GUI, okienka Pad Sticków, interaktywny
//   ListBox, interaktywne checkboxy, algorytm Recording,
//   zapis do pliku i odczyt z pliku, lista dwustonna klatek
//   (problem z usuwaniem pierwszej klatki).
// (2021-11-11)
//  * Injekcja kodu na stabilne 30 FPS, injekcja kodu
//   na komunikaty w prawym-górnym rogu okna gry.
// (2021-11-12)
//  * Injekcja kodu na poprawienie warunku wysłania notyfikacji
//   przez klasę "eAnimNotyfier" (przez kod na stałe kroki
//   czasowe, notyfikacja była wysyłana podwójnie)
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// Libraries
////////////////////////////////////////////////////////////////

#define _USE_MATH_DEFINES
#include <math.h>

#include <stdlib.h>
#include <string.h>

#include <Windows.h>
#include <psapi.h>

#pragma comment (lib, "USER32.LIB")
#pragma comment (lib, "GDI32.LIB")
#pragma comment (lib, "COMDLG32.LIB")
#pragma comment(linker, "/subsystem:windows")

////////////////////////////////////////////////////////////////
// Const Defines for GUI
////////////////////////////////////////////////////////////////

#define KAO2TAS_WINDOW_CLASSNAME  "KAO2_TAS_WINDOW_CLASS"
#define KAO2TAS_WINDOW_NAME  "Kao2 Tool-Assisted Speedruns - Input Tester"

enum DIRECTION
{
    LEFT,
    RIGHT
};

enum IDM
{
    BUTTON_ATTACH = 101,
    BUTTON_RUN,
    BUTTON_PLAYMODE,
    BUTTON_RECMODE,
    BUTTON_OPEN_FILE,
    BUTTON_SAVE_FILE,
    BUTTON_CLEAR_ALL,
    BUTTON_NOTHING,
    BUTTON_PREV_PAGE,
    BUTTON_NEXT_PAGE,
    BUTTON_INSERT_FRAME,
    BUTTON_REMOVE_FRAME,
    BUTTON_FRAME_PARAM
};

#define UPPER_BUTTONS_COUNT  8

const char * BUTTON_NAMES[UPPER_BUTTONS_COUNT + 4 + 2] =
{
    "Attach <KAO2>", "RUN!  |>", "Mode:REPLAY inputs", "Mode:RECORD inputs",
    "Open data file", "Save data file", "Clear all inputs", "",
    "<", ">", "+", "-", "Left Stick (x, y)", "Right Stick (x, y)"
};

const char * FRAME_PARAM_NAMES[1 << 4] =
{
    "(x) JUMP", "(o) PUNCH", "(q) ROLL", "(t) THROW",
    "(RB) STRAFE", "(LB) FPP", "(L1) UNUSED", "(SELECT) HUD",
    "(ESC) MENU", "(START) MENU", "D-pad UP", "D-pad LEFT",
    "D-pad RIGHT", "D-pad DOWN", "RESET CAM", "LOADING"
};

////////////////////////////////////////////////////////////////
// Other defines (buffers, i-frames, binary headers)
////////////////////////////////////////////////////////////////

#define BUF_SIZE       256
#define TINY_BUF_SIZE   32

enum TAS_mode
{
    TM_REPLAY,
    TM_RECORD
};

struct FrameNode;
typedef struct FrameNode FrameNode_t;

struct FrameNode
{
    float sticks[2][2];
    DWORD buttons;
    FrameNode_t * prev;
    FrameNode_t * next;
};

#define KAO2_FPS  30
#define SECONDS_IN_MINUTE  60

#define KAO2_WINDOW_CLASSNAME  "GLUT"
#define KAO2_WINDOW_NAME  "kangurek Kao: 2ga runda"

#define FRAMES_PER_PAGE  ((SECONDS_IN_MINUTE / 2) * KAO2_FPS)

#define TAS_BINFILE_HEADER  "KAO2TAS!"

////////////////////////////////////////////////////////////////
// Globals (common, GUI, i-frames, proc)
////////////////////////////////////////////////////////////////

char g_lastMessage[BUF_SIZE];

enum TAS_MODE g_TAS_mode;

BOOL g_allowButtons;

HWND KAO2_mainWindow;
HWND KAO2_statusLabel;
HWND KAO2_listBoxFrames;
HWND KAO2_listBoxStatus;
HWND KAO2_staticBoxSticks[2];
HWND KAO2_editBoxSticks[2][2];
HWND KAO2_checkBoxParams[1 << 4];
HWND KAO2_editBoxLevel;

WNDPROC KAO2_staticSticksProcedures[2];
HFONT KAO2_font01;
HFONT KAO2_font02;

FrameNode_t * g_frames;
FrameNode_t * g_currentFrame;
FrameNode_t * g_paintedFrame;
int g_currentPage;
int g_currentFrameId;
int g_totalFrames;
HANDLE KAO2_gameHandle;

////////////////////////////////////////////////////////////////
// Cool macros
////////////////////////////////////////////////////////////////

#define MACRO_CLAMPF(__x, __min, __max) \
    (__x > __max) ? __max : \
        ( (__x < __min) ? __min : __x  )

#define TAS_MACRO_SHOW_INPUTS \
    g_paintedFrame = frame; \
    KAO2_clearAndUpdateFrameGui(frame); \
    UpdateWindow(KAO2_mainWindow); \
    if (!KAO2_windowLoop()) { return FALSE; } \
    g_paintedFrame = NULL;

#define CHECK_WINDOW_AND_SET_FONT(__hwnd, __font) \
    if (NULL == __hwnd) { return FALSE; } \
    SendMessage(__hwnd, WM_SETFONT, \
    (WPARAM)__font, (LPARAM) 0);

#define TAS_MACRO_STORE_FLAG(__value, __at) \
    flag = __value; \
    if (!KAO2_writeMem(__at, &flag, 0x01)) { return FALSE; } \

#define TAS_MACRO_WAIT_FLAG_EQ(__value, __at) \
    do { if (!KAO2_readMem(__at, &flag, 0x01)) { return FALSE; } \
    } while (__value == flag);

#define TAS_MACRO_WAIT_FLAG_NE(__value, __at) \
    do { if (!KAO2_readMem(__at, &flag, 0x01)) { return FALSE; } \
    } while (__value != flag);

////////////////////////////////////////////////////////////////
// KAO2 Addresses and Code Replacements
////////////////////////////////////////////////////////////////

#define KAO2_INPUTS_SEMAPHORE_ADDRESS  0x00626EA5
#define KAO2_CODE_INPUTS_ADDRESS       0x004919E0

const BYTE KAO2_INJECTION_PLAYMODE[51] =
{
    0x57, 0x31, 0xC0, 0x31, 0xC9, 0xB1, 0x18, 0xBF, 0xBC, 0x69, 0x62, 0x00, 0xF3, 0xAB, 0x5F, 0x80, 0x3D, 0xA5, 0x6E, 0x62, 0x00, 0x01, 0x76, 0x11, 0xC6, 0x05, 0xA5, 0x6E, 0x62, 0x00, 0x03, 0x80, 0x3D, 0xA5, 0x6E, 0x62, 0x00, 0x03, 0x74, 0xF7, 0xC3, 0xE8, 0xC2, 0xF4, 0xFF, 0xFF, 0xE9, 0xED, 0xFB, 0xFF, 0xFF
};

const BYTE KAO2_INJECTION_RECMODE[51] =
{
    0x57, 0x31, 0xC0, 0x31, 0xC9, 0xB1, 0x18, 0xBF, 0xBC, 0x69, 0x62, 0x00, 0xF3, 0xAB, 0x5F, 0xE8, 0xDC, 0xF4, 0xFF, 0xFF, 0xE8, 0x07, 0xFC, 0xFF, 0xFF, 0x80, 0x3D, 0xA5, 0x6E, 0x62, 0x00, 0x01, 0x76, 0x10, 0xC6, 0x05, 0xA5, 0x6E, 0x62, 0x00, 0x03, 0x80, 0x3D, 0xA5, 0x6E, 0x62, 0x00, 0x03, 0x74, 0xF7, 0xC3
};

#define KAO2_LOADING_FLAG_ADDRESS  0x0062451C
#define KAO2_PAD_STICKS_ADDRESS    0x006269BC
#define KAO2_BUTTONS_LIST_ADDRESS  0x006269DC

const char * KAO2_TASMSG_STANDBY = "[TAS] standby ;)";
const char * KAO2_TASMSG_PLAY    = "[TAS] PLAY: %5i / %5i";
const char * KAO2_TASMSG_REC     = "[TAS] REC: %5i";
const char * KAO2_AVGFPS         = "Average FPS: %.4f";

#define KAO2_TASMSG_ADDRESS     0x006096F0
#define KAO2_AVGFPS_ADDRESS     0x0060970C
#define KAO2_CODE_MSGS_ADDRESS  0x0048B706

const BYTE KAO2_INJECTION_MSGS[80] =
{
    0xD9, 0x47, 0x2C, 0x83, 0xEC, 0x08, 0xDD, 0x1C, 0xE4, 0x68, 0x0C, 0x97, 0x60, 0x00, 0xA1, 0xA8, 0x67, 0x62, 0x00, 0x83, 0xE8, 0x14, 0x50, 0xA1, 0xAC, 0x67, 0x62, 0x00, 0x2D, 0xB0, 0x00, 0x00, 0x00, 0x50, 0xE8, 0x33, 0x9A, 0x02, 0x00, 0x83, 0xC4, 0x14, 0x68, 0xF0, 0x96, 0x60, 0x00, 0xA1, 0xA8, 0x67, 0x62, 0x00, 0x83, 0xE8, 0x28, 0x50, 0xA1, 0xAC, 0x67, 0x62, 0x00, 0x2D, 0xD0, 0x00, 0x00, 0x00, 0x50, 0xE8, 0x12, 0x9A, 0x02, 0x00, 0x83, 0xC4, 0x0C, 0xE9, 0x30, 0x03, 0x00, 0x00
};

#define KAO2_CODE_TICK_ADDRESS  0x0048C198

const BYTE KAO2_INJECTION_TICK[161] =
{
    0x8A, 0x86, 0x84, 0x03, 0x00, 0x00, 0x84, 0xC0, 0x0F, 0x85, 0x81, 0x00, 0x00, 0x00, 0xD9, 0x46, 0x10, 0xD8, 0x4C, 0xE4, 0x10, 0xD8, 0x0D, 0xB0, 0xDF, 0x5C, 0x00, 0xD8, 0x86, 0x64, 0x03, 0x00, 0x00, 0xD9, 0x54, 0xE4, 0x10, 0x83, 0xEC, 0x08, 0xD8, 0xC0, 0xDD, 0x1C, 0xE4, 0xFF, 0x15, 0x6C, 0xD1, 0x5C, 0x00, 0xDB, 0x1C, 0xE4, 0x8B, 0x3C, 0xE4, 0x83, 0xC4, 0x08, 0xD9, 0x44, 0xE4, 0x10, 0xD9, 0x05, 0x4C, 0xDA, 0x5C, 0x00, 0xE8, 0xED, 0x97, 0x11, 0x00, 0xD9, 0x9E, 0x64, 0x03, 0x00, 0x00, 0x85, 0xFF, 0x74, 0x3F, 0xD9, 0x05, 0x4C, 0xDA, 0x5C, 0x00, 0xD8, 0x0D, 0x20, 0xE1, 0x5C, 0x00, 0xD9, 0x5C, 0xE4, 0x08, 0xE8, 0xDE, 0x57, 0x00, 0x00, 0xD9, 0x44, 0xE4, 0x08, 0xD8, 0x86, 0x60, 0x03, 0x00, 0x00, 0xD9, 0x96, 0x60, 0x03, 0x00, 0x00, 0x51, 0xD9, 0x1C, 0xE4, 0x8B, 0x44, 0xE4, 0x0C, 0x50, 0x89, 0xF1, 0xE8, 0xAE, 0xF9, 0xFF, 0xFF, 0x4F, 0x75, 0xD8, 0xEB, 0x05, 0xE8, 0xB4, 0x57, 0x00, 0x00, 0x89, 0xF1, 0xE8, 0x8D, 0xB9, 0xF7, 0xFF, 0x5F, 0x5E, 0x59, 0xC2, 0x04, 0x00
};

#define KAO2_CODE_ANIMNOTYFIER_FIXA_ADDRESS  0x00444C66
#define KAO2_CODE_ANIMNOTYFIER_FIXB_ADDRESS  0x00444D16

const BYTE KAO2_INJECTION_ANIMNOTYFIER_FIXA[11] =
{
    0xF6, 0xC4, 0x41, 0x0F, 0x85, 0x9D, 0x00, 0x00, 0x00, 0x90, 0x90
};

const BYTE KAO2_INJECTION_ANIMNOTYFIER_FIXB[11] =
{
    0xF6, 0xC4, 0x41, 0x0F, 0x85, 0xAF, 0x00, 0x00, 0x00, 0x90, 0x90
};

enum KAO2_KEY_DEFINES
{
    KF_JUMP = 0,
    KF_PUNCH,
    KF_ROLL,
    KF_THROW,
    KF_STRAFE,
    KF_FPP,
    KF_L1,
    KF_R1,
    KF_MENU,
    KF_START,
    KF_DIG_UP,
    KF_DIG_LEFT,
    KF_DIG_RIGHT,
    KF_DIG_DOWN,
    KF_RESET_CAMERA,
    KF_ACTION_LOADING
};

////////////////////////////////////////////////////////////////
// FRAME NODE: Create new object
////////////////////////////////////////////////////////////////

FrameNode_t * FrameNode_create(float l_x, float l_y, float r_x, float r_y, DWORD buttons, FrameNode_t * parent)
{
    FrameNode_t * node = (FrameNode_t *) malloc(sizeof(FrameNode_t));

    if (NULL == node)
    {
        return NULL;
    }

    node->sticks[0][0] = l_x;
    node->sticks[0][1] = l_y;
    node->sticks[1][0] = r_x;
    node->sticks[1][1] = r_y;

    node->buttons = buttons;

    if (NULL != parent)
    {
        (node->next) = (parent->next);
        (parent->next) = node;
    }
    else
    {
        (node->next) = NULL;
    }

    (node->prev) = parent;

    return node;
}

////////////////////////////////////////////////////////////////
// FRAME NODE: remove chain of input-frames
////////////////////////////////////////////////////////////////

VOID FrameNode_remove(FrameNode_t ** head_ref)
{
    FrameNode_t * current = * head_ref;
    FrameNode_t * next = NULL;

    * head_ref = NULL;

    while (NULL != current)
    {
        next = current->next;
        free(current);
        current = next;
    }
}

////////////////////////////////////////////////////////////////
// FRAME NODE: Get i-th frame (if exists)
////////////////////////////////////////////////////////////////

FrameNode_t * FrameNode_getIth(FrameNode_t * head, int id)
{
    if (id < 0)
    {
        return NULL;
    }

    while ((NULL != head) && (id > 0))
    {
        head = head->next;
        id--;
    }

    return head;
}

////////////////////////////////////////////////////////////////
// Quickly check if file has "*.dat" extension
////////////////////////////////////////////////////////////////

BOOL filenameDatExt(const char * name)
{
    int i = strlen(name);

    if (i < 4)
    {
        return FALSE;
    }

    if ((i + 4) >= BUF_SIZE)
    {
        return TRUE;
    }

    if (('.' == name[i - 4]) &&
        (('d' == name[i - 3]) || ('D' == name[i - 3])) &&
        (('a' == name[i - 2]) || ('A' == name[i - 2])) &&
        (('t' == name[i - 1]) || ('T' == name[i - 1])))
    {
        return TRUE;
    }

    return FALSE;
}

////////////////////////////////////////////////////////////////
// BINARY FILE WITH INPUT FRAMES: Saving current list
////////////////////////////////////////////////////////////////

BOOL KAO2_binfileSave()
{
    int i;
    char buf[BUF_SIZE];
    FrameNode_t * frame;

    OPENFILENAME ofn;
    HANDLE file;

    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = KAO2_mainWindow;
    ofn.lpstrFilter = "Binary data files (*.dat)\0*.dat\0";
    ofn.lpstrFile = buf;
    ofn.nMaxFile = BUF_SIZE;
    ofn.lpstrTitle = "Saving KAO2 TAS input frames...";
    ofn.Flags = (OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT);

    buf[0] = '\0';

    if (FALSE != GetSaveFileName(&ofn))
    {
        if (!filenameDatExt(buf))
        {
            i = strlen(buf);
            buf[i] = '.';
            buf[i + 1] = 'd';
            buf[i + 2] = 'a';
            buf[i + 3] = 't';
            buf[i + 4] = '\0';
        }

        file = CreateFile(buf, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);

        if (INVALID_HANDLE_VALUE == file)
        {
            return FALSE;
        }

        WriteFile(file, (LPCVOID) TAS_BINFILE_HEADER, 0x08, NULL, NULL);

        WriteFile(file, (LPCVOID) &g_totalFrames, 0x04, NULL, NULL);

        frame = g_frames;

        for (i = 0; i < g_totalFrames; i++)
        {
            if (NULL == frame)
            {
                CloseHandle(file);
                return FALSE;
            }

            WriteFile(file, (LPCVOID) &(frame->sticks[0][0]), 0x04, NULL, NULL);
            WriteFile(file, (LPCVOID) &(frame->sticks[0][1]), 0x04, NULL, NULL);
            WriteFile(file, (LPCVOID) &(frame->sticks[1][0]), 0x04, NULL, NULL);
            WriteFile(file, (LPCVOID) &(frame->sticks[1][1]), 0x04, NULL, NULL);
            WriteFile(file, (LPCVOID) &(frame->buttons), 0x04, NULL, NULL);

            frame = (frame->next);
        }

        CloseHandle(file);
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////
// BINARY FILE WITH INPUT FRAMES: Loading previously stored list
////////////////////////////////////////////////////////////////

BOOL KAO2_binfileLoad()
{
    DWORD a, b, btn;
    float l_x, l_y, r_x, r_y;
    char buf[BUF_SIZE];
    FrameNode_t * frame, * prev, * new_head;

    OPENFILENAME ofn;
    HANDLE file;

    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = KAO2_mainWindow;
    ofn.lpstrFilter = "Binary data files (*.dat)\0*.dat\0";
    ofn.lpstrFile = buf;
    ofn.nMaxFile = BUF_SIZE;
    ofn.lpstrTitle = "Opening KAO2 TAS input frames...";
    ofn.Flags = (OFN_FILEMUSTEXIST);

    buf[0] = '\0';

    if (FALSE != GetOpenFileName(&ofn))
    {
        file = CreateFile(buf, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

        if (INVALID_HANDLE_VALUE == file)
        {
            return FALSE;
        }

        ReadFile(file, (LPVOID) buf, 0x08, NULL, NULL);

        if (0 != memcmp(buf, TAS_BINFILE_HEADER, 0x08))
        {
            CloseHandle(file);
            return FALSE;
        }

        ReadFile(file, (LPVOID) &b, 0x04, NULL, NULL);

        prev = NULL;
        new_head = NULL;

        for (a = 0; a < b; a++)
        {
            ReadFile(file, (LPVOID) &l_x, 0x04, NULL, NULL);
            ReadFile(file, (LPVOID) &l_y, 0x04, NULL, NULL);
            ReadFile(file, (LPVOID) &r_x, 0x04, NULL, NULL);
            ReadFile(file, (LPVOID) &r_y, 0x04, NULL, NULL);
            ReadFile(file, (LPVOID) &btn, 0x04, NULL, NULL);

            frame = FrameNode_create(l_x, l_y, r_x, r_y, btn, prev);

            prev = frame;

            if (NULL == new_head)
            {
                new_head = prev;
            }
        }

        CloseHandle(file);

        g_currentFrame = NULL;
        g_currentFrameId = (-1);
        g_currentPage = 0;

        FrameNode_remove(&g_frames);

        g_totalFrames = b;
        g_frames = new_head;
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////
// KAO2 PROCESS HOOKING: Match possible EXE filename
////////////////////////////////////////////////////////////////

BOOL KAO2_matchGameExecutableName(const char * filename)
{
    if (0 == strcmp(filename, "kao2.exe"))
    {
        return TRUE;
    }

    if (0 == strcmp(filename, "kao2_mod.exe"))
    {
        return TRUE;
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

    return (found_pid == pid);
}

////////////////////////////////////////////////////////////////
// KAO2 PROCESS HOOKING: Find the right process
//  and return special handle on success
////////////////////////////////////////////////////////////////

HANDLE KAO2_findGameProcess()
{
    HANDLE proc_handle;
    HWND game_window;
    DWORD pids[(2 * BUF_SIZE)], i, count;
    char buf[(2 * BUF_SIZE)], * file_name, * p;

    if (NULL != (game_window = FindWindow(KAO2_WINDOW_CLASSNAME, KAO2_WINDOW_NAME)))
    {
        if (FALSE != EnumProcesses(pids, (sizeof(DWORD) * (2 * BUF_SIZE)), &count))
        {
            for (i = 0; i < count; i++)
            {
                proc_handle = OpenProcess((PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION ), FALSE, pids[i]);

                if ((NULL != proc_handle) && (INVALID_HANDLE_VALUE != proc_handle))
                {
                    buf[0] = '\0';
                    GetModuleFileNameEx(proc_handle, NULL, buf, (2 * BUF_SIZE));
                    buf[(2 * BUF_SIZE - 1)] = '\0';
                    file_name = NULL;

                    for (p = buf; (*p); p++)
                    {
                        if (('/' == (*p)) || ('\\' == (*p)))
                        {
                            file_name = p + 1;
                        }
                    }

                    if (file_name)
                    {
                        if (KAO2_matchGameExecutableName(file_name))
                        {
                            if (KAO2_matchGameWindowWithProccess(game_window, pids[i]))
                            {
                                return proc_handle;
                            }
                        }
                    }

                    CloseHandle(proc_handle);
                    proc_handle = INVALID_HANDLE_VALUE;
                }
            }
        }
    }

    return INVALID_HANDLE_VALUE;
}

////////////////////////////////////////////////////////////////
// WINAPI GUI: Main window loop
////////////////////////////////////////////////////////////////

BOOL KAO2_windowLoop()
{
    BOOL still_active = TRUE;
    MSG message = {0};

    while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
    {
        if (WM_QUIT == message.message)
        {
            still_active = FALSE;
        }

        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return still_active;
}

////////////////////////////////////////////////////////////////
// WINAPI GUI: Updating EditBox control of Gamepad Stick co-ord
////////////////////////////////////////////////////////////////

VOID KAO2_updateStickEdit(const FrameNode_t * frame, int id, int coord)
{
    char buf[TINY_BUF_SIZE];

    if (NULL != frame)
    {
        sprintf_s(buf, TINY_BUF_SIZE, "%.7f", (frame->sticks[id][coord]));
    }
    else
    {
        buf[0] = '\0';
    }

    SetWindowText(KAO2_editBoxSticks[id][coord], buf);
}

////////////////////////////////////////////////////////////////
// WINAPI GUI: Redraw Static control with Gamepad Stick
// and optionally update associated EditBox [X][Y] controls
////////////////////////////////////////////////////////////////

VOID KAO2_updateStickStaticAndEdits(const FrameNode_t * frame, int id, BOOL settext)
{
    RedrawWindow(KAO2_staticBoxSticks[id], NULL, NULL, RDW_INVALIDATE);

    if (settext)
    {
        KAO2_updateStickEdit(frame, id, 0);
        KAO2_updateStickEdit(frame, id, 1);
    }
}

////////////////////////////////////////////////////////////////
// WINAPI GUI: Handling notification sent by EditBox control
// for co-ords [LX][LY][RX][RY] of one of Gamepad Sticks
////////////////////////////////////////////////////////////////

BOOL KAO2_stickEditNotify(HWND editbox, WORD msg, int id, int coord)
{
    int a, b;
    char buf[16];
    float dummy;

    if ((EN_CHANGE == msg) && (NULL != g_currentFrame))
    {
        GetWindowText(editbox, buf, 16);

        dummy = strtof(buf, NULL);

        if ((HUGE_VAL == errno) || (ERANGE == errno))
        {
            dummy = 0;
        }
        else
        {
            dummy = MACRO_CLAMPF(dummy, (-1.5f), (+1.5f));
        }

        g_currentFrame->sticks[id][coord] = dummy;

        KAO2_updateStickStaticAndEdits(NULL, id, FALSE);

        return TRUE;
    }

    return FALSE;
}

////////////////////////////////////////////////////////////////
// WINAPI GUI: Clear controls that display the state of current
// input-frame, and set according controls to given i-frame object
////////////////////////////////////////////////////////////////

VOID KAO2_clearAndUpdateFrameGui(const FrameNode_t * frame)
{
    int i;
    DWORD buttons;

    for (i = 0; i < 2; i++)
    {
        KAO2_updateStickStaticAndEdits(frame, i, TRUE);
    }

    for (i = 0; i < (1 << 4); i++)
    {
        SendMessage(KAO2_checkBoxParams[i], BM_SETCHECK, (WPARAM) BST_UNCHECKED, (LPARAM) NULL);
    }

    if (NULL != frame)
    {
        buttons = (frame->buttons);

        for (i = 0; i < (1 << 4); i++)
        {
            SendMessage
            (
                KAO2_checkBoxParams[i],
                BM_SETCHECK,
                (WPARAM) (((0x01 << i) & buttons) ? BST_CHECKED : BST_UNCHECKED),
                (LPARAM) NULL
            );
        }
    }
}

////////////////////////////////////////////////////////////////
// WINAPI GUI: Show quick text status after some operation
////////////////////////////////////////////////////////////////

VOID KAO2_showStatus(const char * msg)
{
    char buf[BUF_SIZE];
    sprintf_s(buf, BUF_SIZE, "(re%s) %s", ((TM_REPLAY == g_TAS_mode) ? "play" : "cord"), msg);

    SetWindowText(KAO2_statusLabel, buf);
    UpdateWindow(KAO2_mainWindow);
}

////////////////////////////////////////////////////////////////
// TOOL-ASSISTED MANIPULATION: Write bytes to Kao2 game process
////////////////////////////////////////////////////////////////

BOOL KAO2_writeMem(DWORD address, LPCVOID what, size_t length)
{
    if (!WriteProcessMemory(KAO2_gameHandle, (LPVOID)address, what, length, NULL))
    {
        sprintf_s(g_lastMessage, BUF_SIZE, "Could not write %d bytes to address 0x%08X!", length, address);

        return FALSE;
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////
// TOOL-ASSISTED MANIPULATION: Read bytes from Kao2 game process
////////////////////////////////////////////////////////////////

BOOL KAO2_readMem(DWORD address, LPVOID what, size_t length)
{
    if (!ReadProcessMemory(KAO2_gameHandle, (LPVOID)address, what, length, NULL))
    {
        sprintf_s(g_lastMessage, BUF_SIZE, "Could not read %d bytes from address 0x%08X!", length, address);

        return FALSE;
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////
// TOOL-ASSISTED MANIPULATION: Prepare the retail version
// of "Kao the Kangaroo: Round 2" for usage with this cool tool
////////////////////////////////////////////////////////////////

BOOL KAO2_firstInjection()
{
    BYTE flag = 0x00;
    const BYTE * code;
    size_t length;

    /* InputManager - Semaphore unlock */

    if (!KAO2_writeMem(KAO2_INPUTS_SEMAPHORE_ADDRESS, &flag, 0x01))
    {
        return FALSE;
    }

    /* InputManager - Choosing algorithm */

    if (TM_REPLAY == g_TAS_mode)
    {
        code = KAO2_INJECTION_PLAYMODE;
        length = sizeof(KAO2_INJECTION_PLAYMODE);
    }
    else
    {
        code = KAO2_INJECTION_RECMODE;
        length = sizeof(KAO2_INJECTION_RECMODE);
    }

    if (!KAO2_writeMem(KAO2_CODE_INPUTS_ADDRESS, (LPCVOID)code, length))
    {
        return FALSE;
    }

    /* eKao2Gamelet::onTick() - stable framerate */

    if (!KAO2_writeMem(KAO2_CODE_TICK_ADDRESS, (LPCVOID)KAO2_INJECTION_TICK, sizeof(KAO2_INJECTION_TICK)))
    {
        return FALSE;
    }

    /* eAnimNotyfier::onUpdate() - fix double notifies */

    if (!KAO2_writeMem(KAO2_CODE_ANIMNOTYFIER_FIXA_ADDRESS, (LPCVOID)KAO2_INJECTION_ANIMNOTYFIER_FIXA, sizeof(KAO2_INJECTION_ANIMNOTYFIER_FIXA)))
    {
        return FALSE;
    }

    if (!KAO2_writeMem(KAO2_CODE_ANIMNOTYFIER_FIXB_ADDRESS, (LPCVOID)KAO2_INJECTION_ANIMNOTYFIER_FIXB, sizeof(KAO2_INJECTION_ANIMNOTYFIER_FIXB)))
    {
        return FALSE;
    }

    /* Micro messages (FPS and TAS status, visible in-game) */

    if (!KAO2_writeMem(KAO2_TASMSG_ADDRESS, (LPCVOID)KAO2_TASMSG_STANDBY, 0x01 + strlen(KAO2_TASMSG_STANDBY)))
    {
        return FALSE;
    }

    if (!KAO2_writeMem(KAO2_AVGFPS_ADDRESS, (LPCVOID)KAO2_AVGFPS, 0x01 + strlen(KAO2_AVGFPS)))
    {
        return FALSE;
    }

    if (!KAO2_writeMem(KAO2_CODE_MSGS_ADDRESS, (LPCVOID)KAO2_INJECTION_MSGS, sizeof(KAO2_INJECTION_MSGS)))
    {
        return FALSE;
    }

    /* Cool and good */

    return TRUE;
}

////////////////////////////////////////////////////////////////
// TOOL-ASSISTED MANIPULATION: Recording input frames
////////////////////////////////////////////////////////////////

BOOL KAO2_runRecordingAlgorithm()
{
    DWORD a, b, rec = TRUE, address, buttons;
    char buf[TINY_BUF_SIZE];
    BYTE flag;
    float sticks[2][2];
    FrameNode_t * frame, * prev_frame = NULL;

    /* Remove current set of i-frames... */

    FrameNode_remove(&g_frames);
    g_totalFrames = 0;

    //@ /* DEBUG with counters */
    //@ a = 0;
    //@ char debug_buf[BUF_SIZE];
    //@ if (!KAO2_writeMem(0x00626A24, &a, 0x04)) { return FALSE; }
    //@ if (!KAO2_writeMem(0x00626A28, &a, 0x04)) { return FALSE; }

    while (rec)
    {
        /* Micro message */

        sprintf_s(buf, TINY_BUF_SIZE, KAO2_TASMSG_REC, g_totalFrames);
        if (!KAO2_writeMem(KAO2_TASMSG_ADDRESS, buf, 0x01 + strlen(buf)))
        {
            return FALSE;
        }

        /* Is this loading mode? */

        if (!KAO2_readMem(KAO2_LOADING_FLAG_ADDRESS, &flag, 0x01))
        {
            return FALSE;
        }

        if (0x01 == flag)
        {
            /* Zanotuj to i czekaj na załadowanie */

            buttons = (0x01 << KF_ACTION_LOADING);

            TAS_MACRO_STORE_FLAG(0x00, KAO2_INPUTS_SEMAPHORE_ADDRESS);

            TAS_MACRO_WAIT_FLAG_EQ(0x01, KAO2_LOADING_FLAG_ADDRESS);

            TAS_MACRO_STORE_FLAG(0x02, KAO2_INPUTS_SEMAPHORE_ADDRESS);

            sticks[0][0] = 0;
            sticks[0][1] = 0;
            sticks[1][0] = 0;
            sticks[1][1] = 0;
        }
        else
        {
            /* Notify Kao that I want to read user's input frame... */

            TAS_MACRO_STORE_FLAG(0x02, KAO2_INPUTS_SEMAPHORE_ADDRESS);

            /* Kao accepts the request */
            /* (if unlocks now, processes input from last frame) */

            TAS_MACRO_WAIT_FLAG_NE(0x03, KAO2_INPUTS_SEMAPHORE_ADDRESS);

            /* Read sticks and buttons */

            address = KAO2_PAD_STICKS_ADDRESS;

            for (a = 0; a < 2; a++)
            {
                for (b = 0; b < 2; b++)
                {
                    if (!KAO2_readMem(address, &(sticks[a][b]), 0x04))
                    {
                        return FALSE;
                    }

                    address += 0x04;
                }
            }

            address = KAO2_BUTTONS_LIST_ADDRESS;
            buttons = 0;
            b = 0x01;

            for (a = 0; a < ((1 << 4) - 1); a++)
            {
                if (!KAO2_readMem(address, &flag, 0x01))
                {
                    return FALSE;
                }

                if (flag)
                {
                    buttons |= b;
                }

                b = (b << 1);
                address += 0x04;
            }
        }

        /* Create input frame */

        frame = FrameNode_create
        (
            sticks[0][0], sticks[0][1],
            sticks[1][0], sticks[1][1],
            buttons, prev_frame
        );

        g_totalFrames++;

        if (NULL == prev_frame)
        {
            g_frames = frame;
        }

        prev_frame = frame;

        /* Present input frame in GUI */

        TAS_MACRO_SHOW_INPUTS;

        /* Warunek zakończenia nagrywania */

        if ((0x01 << KF_R1) & buttons)
        {
            rec = FALSE;
        }
    }

    /* Micro message */

    if (!KAO2_writeMem(KAO2_TASMSG_ADDRESS, (LPCVOID)KAO2_TASMSG_STANDBY, 0x01 + strlen(KAO2_TASMSG_STANDBY)))
    {
        return FALSE;
    }

    /* Gra nie musi czekać następnym razem */

    TAS_MACRO_STORE_FLAG(0x00, KAO2_INPUTS_SEMAPHORE_ADDRESS);

    //@ /* DEBUG with counters */
    //@ if (!KAO2_readMem(0x00626A24, &a, 0x04)) { return FALSE; }
    //@ if (!KAO2_readMem(0x00626A28, &b, 0x04)) { return FALSE; }
    //@ sprintf_s
    //@ (
    //@     debug_buf, BUF_SIZE,
    //@         "g_totalFrames  = %5d\r\n" \
    //@         "skipped frames = %5d\r\n" \
    //@         "TAS frames     = %5d\r\n",
    //@     g_totalFrames, a, b
    //@ );
    //@ MessageBox(KAO2_mainWindow, debug_buf, "Emulator", MB_ICONINFORMATION);

    frame = NULL;
    TAS_MACRO_SHOW_INPUTS;

    return TRUE;
}

////////////////////////////////////////////////////////////////
// TOOL-ASSISTED MANIPULATION: Replaying input frames
////////////////////////////////////////////////////////////////

BOOL KAO2_runReplayingAlgorithm()
{
    DWORD a, b, counter = 1, address;
    char buf[TINY_BUF_SIZE];
    BYTE flag;
    FrameNode_t * frame = g_frames;

    //@ /* DEBUG with counters */
    //@ a = 0;
    //@ char debug_buf[BUF_SIZE];
    //@ if (!KAO2_writeMem(0x00626A24, &a, 0x04)) { return FALSE; }
    //@ if (!KAO2_writeMem(0x00626A28, &a, 0x04)) { return FALSE; }

    while (NULL != frame)
    {
        /* Micro message */

        sprintf_s(buf, TINY_BUF_SIZE, KAO2_TASMSG_PLAY, counter, g_totalFrames);
        if (!KAO2_writeMem(KAO2_TASMSG_ADDRESS, buf, 0x01 + strlen(buf)))
        {
            return FALSE;
        }

        /* Present input frame in GUI */

        TAS_MACRO_SHOW_INPUTS;

        /* Is this loading mode? */

        if ((0x01 << KF_ACTION_LOADING) & (frame->buttons))
        {
            /* Odblokuj grę - musi przeprocesować inputy */
            /* z poprzedniej klatki... */

            TAS_MACRO_STORE_FLAG(0x00, KAO2_INPUTS_SEMAPHORE_ADDRESS);

            /* Czekaj na wejście do ładowania*/

            TAS_MACRO_WAIT_FLAG_EQ(0x00, KAO2_LOADING_FLAG_ADDRESS);

            /* Zablokuj input */

            TAS_MACRO_STORE_FLAG(0x02, KAO2_INPUTS_SEMAPHORE_ADDRESS);

            /* Czekaj na wyjście z ładowania*/

            TAS_MACRO_WAIT_FLAG_EQ(0x01, KAO2_LOADING_FLAG_ADDRESS);
        }
        else
        {
            /* Notify Kao that I want to insert a custom input frame... */

            TAS_MACRO_STORE_FLAG(0x02, KAO2_INPUTS_SEMAPHORE_ADDRESS);

            /* Wait for Kao to process game logic (from prev frame) */
            /* and clear input data (for this frame) */

            TAS_MACRO_WAIT_FLAG_NE(0x03, KAO2_INPUTS_SEMAPHORE_ADDRESS);

            /* Send data for this input-frame */

            address = KAO2_PAD_STICKS_ADDRESS;

            for (a = 0; a < 2; a++)
            {
                for (b = 0; b < 2; b++)
                {
                    if (!KAO2_writeMem(address, &(frame->sticks[a][b]), 0x04))
                    {
                        return FALSE;
                    }

                    address += 0x04;
                }
            }

            address = KAO2_BUTTONS_LIST_ADDRESS;
            flag = 0x01;
            b = 0x01;

            for (a = 0; a < ((1 << 4) - 1); a++)
            {
                if (b & (frame->buttons))
                {
                    if (!KAO2_writeMem(address, &flag, 0x01))
                    {
                        return FALSE;
                    }
                }

                b = (b << 1);
                address += 0x04;
            }
        }

        /* Gra ruszy z kopyta na początku kolejnej iteracji */
        /* lub po wyjściu z pętelki -- na razie czeka aż */
        /* flaga zmieni się z "0x03" na coś innego */

        frame = (frame->next);
        counter++;
    }

    /* Micro message */

    if (!KAO2_writeMem(KAO2_TASMSG_ADDRESS, (LPCVOID)KAO2_TASMSG_STANDBY, 0x01 + strlen(KAO2_TASMSG_STANDBY)))
    {
        return FALSE;
    }

    /* Odblokuj grę od kolejnej i-klatki */

    TAS_MACRO_STORE_FLAG(0x00, KAO2_INPUTS_SEMAPHORE_ADDRESS);

    //@ /* DEBUG with counters */
    //@ if (!KAO2_readMem(0x00626A24, &a, 0x04)) { return FALSE; }
    //@ if (!KAO2_readMem(0x00626A28, &b, 0x04)) { return FALSE; }
    //@ sprintf_s
    //@ (
    //@     debug_buf, BUF_SIZE,
    //@         "g_totalFrames  = %5d\r\n" \
    //@         "skipped frames = %5d\r\n" \
    //@         "TAS frames     = %5d\r\n",
    //@     g_totalFrames, a, b
    //@ );
    //@ MessageBox(KAO2_mainWindow, debug_buf, "Emulator", MB_ICONINFORMATION);

    frame = NULL;
    TAS_MACRO_SHOW_INPUTS;

    return TRUE;
}

////////////////////////////////////////////////////////////////
// WINAPI GUI: Direction sense for given Gamepad Stick co-ords
////////////////////////////////////////////////////////////////

const char * KAO2_getDirectionText(float x, float y)
{
    double length = sqrt(x * x + y * y);
    double angle = (atan2(y, x) / M_PI * 180.0) + 180.0;

    const double ONE_EIGHT = 360.0 / 8;
    const DOUBLE ONE_SIXTEENTH = ONE_EIGHT * 0.5;

    if (length < 0.5)
    {
        return "  ";
    }
    else if (angle < ((ONE_EIGHT * 1) - ONE_SIXTEENTH))
    {
        return " W";
    }
    else if (angle < ((ONE_EIGHT * 2) - ONE_SIXTEENTH))
    {
        return "SW";
    }
    else if (angle < ((ONE_EIGHT * 3) - ONE_SIXTEENTH))
    {
        return "S ";
    }
    else if (angle < ((ONE_EIGHT * 4) - ONE_SIXTEENTH))
    {
        return "SE";
    }
    else if (angle < ((ONE_EIGHT * 5) - ONE_SIXTEENTH))
    {
        return " E";
    }
    else if (angle < ((ONE_EIGHT * 6) - ONE_SIXTEENTH))
    {
        return "NE";
    }
    else if (angle < ((ONE_EIGHT * 7) - ONE_SIXTEENTH))
    {
        return "N ";
    }
    else if (angle < ((ONE_EIGHT * 8) - ONE_SIXTEENTH))
    {
        return "NW";
    }

    return " W";
}

////////////////////////////////////////////////////////////////
// WINAPI GUI: Genrating short-hand text for ListBox control
////////////////////////////////////////////////////////////////

VOID KAO2_genInputFrameText(char * result, FrameNode_t * frame, int id)
{
    sprintf_s
    (
        result, BUF_SIZE, "(%5d/%5d) [%s] [%s] %c%c%c%c %s %s %s %s %s %s %s [%s]",
            id,
            g_totalFrames,
            KAO2_getDirectionText(frame->sticks[0][0], frame->sticks[0][1]),
            KAO2_getDirectionText(frame->sticks[1][0], - frame->sticks[1][1]),
            ((0x01 << KF_JUMP) & frame->buttons) ? 'x' : ' ',
            ((0x01 << KF_PUNCH) & frame->buttons) ? 'o' : ' ',
            ((0x01 << KF_ROLL) & frame->buttons) ? 'q' : ' ',
            ((0x01 << KF_THROW) & frame->buttons) ? 't' : ' ',
            ((0x01 << KF_STRAFE) & frame->buttons) ? "L2" : "  ",
            ((0x01 << KF_FPP) & frame->buttons) ? "R2" : "  ",
            ((0x01 << KF_L1) & frame->buttons) ? "L1" : "  ",
            ((0x01 << KF_R1) & frame->buttons) ? "L2" : "  ",
            ((0x01 << KF_MENU) & frame->buttons) ? "ESC" : "   ",
            ((0x01 << KF_START) & frame->buttons) ? "START" : "     ",
            ((0x01 << KF_RESET_CAMERA) & frame->buttons) ? "CAM" : "   ",
            ((0x01 << KF_ACTION_LOADING) & frame->buttons) ? "LOADING" : "       "
    );
}

////////////////////////////////////////////////////////////////
// WINAPI GUI: Changing and/or refreshing a page of i-frames
////////////////////////////////////////////////////////////////

VOID KAO2_LB_refreshFrameListBox(int direction)
{
    const int pages = (g_totalFrames + FRAMES_PER_PAGE - 1) / FRAMES_PER_PAGE;

    int counter = FRAMES_PER_PAGE;
    int id;

    char buf[BUF_SIZE];
    FrameNode_t * frame;

    if (LEFT == direction)
    {
        g_currentPage -= 1;

        if (g_currentPage < 0)
        {
            g_currentPage = 0;
        }
    }
    else if (RIGHT == direction)
    {
        g_currentPage += 1;

        if (g_currentPage >= pages)
        {
            g_currentPage = pages - 1;
        }
    }

    id = g_currentPage * FRAMES_PER_PAGE;
    frame = FrameNode_getIth(g_frames, id);

    SendMessage(KAO2_listBoxFrames, LB_RESETCONTENT, (WPARAM)NULL, (LPARAM)NULL);

    while ((NULL != frame) && (counter > 0))
    {
        KAO2_genInputFrameText(buf, frame, id);
        SendMessage(KAO2_listBoxFrames, LB_ADDSTRING, (WPARAM)NULL, (LPARAM)buf);

        frame = frame->next;
        id++;
        counter--;
    }

    sprintf_s(buf, BUF_SIZE, "(page %3d of %3d)", (1 + g_currentPage), pages);
    SetWindowText(KAO2_listBoxStatus, buf);
}

////////////////////////////////////////////////////////////////
// WINAPI GUI: After selecting or changing an item in the ListBox
////////////////////////////////////////////////////////////////

VOID KAO2_LB_selectInputFrameOrUpdateGui(int lb_id, BOOL update)
{
    int i;
    char buf[BUF_SIZE];

    if ((NULL == g_currentFrame) && (lb_id >= 0))
    {
        g_currentFrameId = lb_id + g_currentPage * FRAMES_PER_PAGE;
        g_currentFrame = FrameNode_getIth(g_frames, g_currentFrameId);
    }
    else if (lb_id < 0)
    {
        /* Will return (-1) if there are no frames left. */
        lb_id = KAO2_LB_getIndexAndAdjustPage();
    }

    if (lb_id >= 0)
    {
        if (update)
        {
            SendMessage(KAO2_listBoxFrames, LB_DELETESTRING, (WPARAM) lb_id, (LPARAM) NULL);

            KAO2_genInputFrameText(buf, g_currentFrame, g_currentFrameId);

            SendMessage(KAO2_listBoxFrames, LB_INSERTSTRING, (WPARAM) lb_id, (LPARAM) buf);

            SendMessage(KAO2_listBoxFrames, LB_SETCURSEL, (WPARAM) lb_id, (LPARAM) NULL);

            sprintf_s(buf, BUF_SIZE, "(Updated frame #%05d)", g_currentFrameId);
        }
        else
        {
            SendMessage(KAO2_listBoxFrames, LB_SETCURSEL, (WPARAM) lb_id, (LPARAM) NULL);

            sprintf_s(buf, BUF_SIZE, "(Selected frame #%05d)", g_currentFrameId);
        }

        SetWindowText(KAO2_listBoxStatus, buf);
    }

    KAO2_clearAndUpdateFrameGui(g_currentFrame);
}

////////////////////////////////////////////////////////////////
// WINAPI GUI: Find an index of current i-frame
// relative to currently shown page in ListBox control
////////////////////////////////////////////////////////////////

int KAO2_LB_getIndexAndAdjustPage()
{
    int i = (-1);

    if (g_currentFrameId >= 0)
    {
        i = g_currentFrameId - FRAMES_PER_PAGE * g_currentPage;

        if (i < 0)
        {
            while (i < 0)
            {
                i += FRAMES_PER_PAGE;
                g_currentPage--;
            }
        }
        else if (i >= FRAMES_PER_PAGE)
        {
            while (i >= FRAMES_PER_PAGE)
            {
                i -= FRAMES_PER_PAGE;
                g_currentPage++;
            }
        }
    }

    return i;
}

////////////////////////////////////////////////////////////////
// WINAPI GUI: Insert new input frame
// (cloning data from preceding i-frame)
////////////////////////////////////////////////////////////////

VOID KAO2_LB_insertNewInputFrame()
{
    char buf[TINY_BUF_SIZE];
    FrameNode_t * prev_frame;

    if (NULL == g_frames)
    {
        g_frames = FrameNode_create(0, 0, 0, 0, 0, NULL);
        g_currentFrame = g_frames;
        g_currentFrameId = 0;
        g_totalFrames = 1;
        g_currentPage = 0;
    }
    else
    {
        if (NULL == g_currentFrame)
        {
            g_currentFrame = g_frames;
            prev_frame = NULL;

            g_currentFrameId = 0;
            while (NULL != g_currentFrame)
            {
                prev_frame = g_currentFrame;
                g_currentFrame = g_currentFrame->next;
                g_currentFrameId++;
            }
        }
        else
        {
            prev_frame = g_currentFrame;
            g_currentFrameId++;
        }

        g_currentFrame = FrameNode_create
        (
            prev_frame->sticks[0][0],
            prev_frame->sticks[0][1],
            prev_frame->sticks[1][0],
            prev_frame->sticks[1][1],
            prev_frame->buttons,
            prev_frame
        );

        g_totalFrames++;
    }

    sprintf_s(buf, TINY_BUF_SIZE, "Inserted frame #%03d.", g_currentFrameId);
    SetWindowText(KAO2_listBoxStatus, buf);

    KAO2_LB_refreshFrameListBox(-1);
    KAO2_LB_selectInputFrameOrUpdateGui((-1), FALSE);
}

////////////////////////////////////////////////////////////////
// WINAPI GUI: Removing current input frame
// and rearranging the main list
////////////////////////////////////////////////////////////////

BOOL KAO2_LB_removeCurrentInputFrame()
{
    int i = g_currentFrameId;
    char buf[BUF_SIZE];

    FrameNode_t * prev_frame;
    FrameNode_t * next_frame;

    if (NULL != g_currentFrame)
    {
        prev_frame = (g_currentFrame->prev);
        next_frame = (g_currentFrame->next);

        if ((NULL != prev_frame) && (g_currentFrameId > 0))
        {
            /* There is at least one frame before */

            (prev_frame->next) = next_frame;
        }
        else if ((NULL == prev_frame) && (0 == g_currentFrameId))
        {
            /* Must be the first frame */

            if (g_currentFrame != g_frames)
            {
                return FALSE;
            }

            g_frames = next_frame;
        }
        else
        {
            /* Incorrect setting */

            return FALSE;
        }

        /* Remove this frame and decrese total count */

        free(g_currentFrame);
        g_totalFrames--;

        if ((NULL == next_frame) && (g_currentFrameId == g_totalFrames))
        {
            /* Was the last frame (or the only frame) */

            g_currentFrame = prev_frame;
            g_currentFrameId--;
        }
        else if ((NULL != next_frame) && (g_currentFrameId < g_totalFrames))
        {
            /* Neither last nor first */

            g_currentFrame = next_frame;
            (next_frame->prev) = prev_frame;
        }
        else
        {
            /* Incorrect setting */

            g_currentFrame = NULL;
            g_currentFrameId = (-1);
            g_currentPage = 0;

            return FALSE;
        }

        KAO2_LB_refreshFrameListBox(-1);
        KAO2_LB_selectInputFrameOrUpdateGui((-1), FALSE);

        sprintf_s(buf, BUF_SIZE, "(Deleted frame #%05d)", i);
        SetWindowText(KAO2_listBoxStatus, buf);
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////
// WINAPI GUI: Callback procedure for Static controls
// that give the sense of direction for Gamepad Sticks
////////////////////////////////////////////////////////////////

LRESULT CALLBACK KAO2_winProcAnyStick(int id, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HRGN bgRgn;
    RECT rect;

    POINT pt, halves, line_start, line_end;
    LONG i;
    struct { FLOAT x; FLOAT y; } fp;

    char buf[BUF_SIZE];
    FrameNode_t * frame;

    switch (Msg)
    {
        case WM_PAINT:
        {
            BeginPaint(hWnd, &ps);
            GetClientRect(hWnd, &rect);
            halves.x = rect.right / 2;
            halves.y = rect.bottom / 2;

            SelectObject(ps.hdc, GetStockObject(DC_PEN));
            SelectObject(ps.hdc, GetStockObject(BLACK_BRUSH));

            SetDCPenColor(ps.hdc, RGB(0, 0, 0));
            Rectangle(ps.hdc, 0, 0, rect.right, rect.bottom);

            SetDCPenColor(ps.hdc, RGB(255, 255, 255));
            Ellipse(ps.hdc, 1, 1, rect.right - 1, rect.bottom - 1);
            Rectangle(ps.hdc, halves.x - 2, halves.y - 2, halves.x + 3, halves.y + 3);

            frame = (NULL != g_paintedFrame) ? g_paintedFrame : g_currentFrame;

            if (NULL != frame)
            {
                SetDCPenColor(ps.hdc, RGB(255, 0, 0));

                /* Clamped direction (-1.f to +1.f) extended */
                /* to the size of the half of square control */
                fp.x = MACRO_CLAMPF((frame->sticks[id][0]), (-1.f), (+1.f)) * halves.x;
                fp.y = MACRO_CLAMPF((frame->sticks[id][1]), (-1.f), (+1.f)) * halves.y;

                /* Line end point on the square control. */
                /* Notice how Left Stick is inverted on Y-axis, */
                /* while the Right Stick is not! */
                line_end.x = halves.x + ((int)fp.x);
                line_end.y = halves.y + ((0 == id) ? (-1) : 1) * ((int) fp.y);

                for (i = 0; i < (2 * 2); i++)
                {
                    /* Offsetting for a thicker line. */
                    pt.x = (i % 2);
                    pt.y = (i / 2);

                    MoveToEx(ps.hdc, halves.x + pt.x, halves.y + pt.y, NULL);
                    LineTo(ps.hdc, line_end.x + pt.x, line_end.y + pt.y);
                }
            }

            EndPaint(hWnd, &ps);

            break;
        }

        case WM_LBUTTONDOWN:
        {
            if (NULL != g_currentFrame)
            {
                GetClientRect(hWnd, &rect);
                MapWindowPoints(hWnd, GetParent(hWnd), (LPPOINT)&rect, 2);

                pt.x = (LONG) LOWORD(lParam);
                pt.y = (LONG) HIWORD(lParam);

                if ((pt.x >= rect.left) && (pt.x < rect.right) && (pt.y >= rect.top) && (pt.y < rect.bottom))
                {
                    /* Half of the square control (stick unit length */
                    /* and also a center point for direction) */
                    fp.x = (rect.right - rect.left) / 2;
                    fp.y = (rect.bottom - rect.top) / 2;

                    /* Direction vector in Stick unit distance, */
                    /* with additional error margin */
                    fp.x = ((pt.x - rect.left) - fp.x) / (fp.x - 4);
                    fp.y = ((pt.y - rect.top) - fp.y) / (fp.y - 4);

                    g_currentFrame->sticks[id][0] = fp.x;
                    g_currentFrame->sticks[id][1] = ((0 == id) ? (-1) : 1) * fp.y;

                    KAO2_LB_selectInputFrameOrUpdateGui((-1), TRUE);
                }
            }

            break;
        }
    }

    return CallWindowProc(KAO2_staticSticksProcedures[id], hWnd, Msg, wParam, lParam);
}

////////////////////////////////////////////////////////////////
// WINAPI GUI: Indirect callback procedure
// for Static control of the Left Gamepad Stick
////////////////////////////////////////////////////////////////

LRESULT CALLBACK KAO2_winProcLeftStick(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return KAO2_winProcAnyStick(LEFT, hWnd, Msg, wParam, lParam);
}

////////////////////////////////////////////////////////////////
// WINAPI GUI: Indirect callback procedure
// for Static control of the Right Gamepad Stick
////////////////////////////////////////////////////////////////

LRESULT CALLBACK KAO2_winProcRightStick(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return KAO2_winProcAnyStick(RIGHT, hWnd, Msg, wParam, lParam);
}

////////////////////////////////////////////////////////////////
// WINAPI GUI: Callback procedure for the main window of this tool
////////////////////////////////////////////////////////////////

LRESULT CALLBACK KAO2_windowProcedure(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    DWORD a, b;

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

        case WM_LBUTTONDOWN:
        {
            if (g_allowButtons)
            {
                g_allowButtons = FALSE;

                for (a = 0; a < 2; a++)
                {
                    SendMessage(KAO2_staticBoxSticks[a], WM_LBUTTONDOWN, wParam, lParam);
                }

                g_allowButtons = TRUE;
            }

            break;
        }

        case WM_COMMAND:
        {
            if (g_allowButtons)
            {
                g_allowButtons = FALSE;

                switch (LOWORD(wParam))
                {
                    case BUTTON_ATTACH:
                    {
                        if (BN_CLICKED == HIWORD(wParam))
                        {
                            KAO2_gameHandle = KAO2_findGameProcess();

                            if (INVALID_HANDLE_VALUE == KAO2_gameHandle)
                            {
                                KAO2_showStatus(".");

                                MessageBox(KAO2_mainWindow, "Could not attach the \"kao2.exe\" game process!", "Epic fail!", MB_ICONERROR);
                            }
                            else
                            {
                                KAO2_showStatus("KAO2 attached...");

                                if (KAO2_firstInjection())
                                {
                                    KAO2_showStatus("KAO2 injected and ready!");
                                }
                                else
                                {
                                    KAO2_showStatus(".");

                                    MessageBox(KAO2_mainWindow, g_lastMessage, "Failed to inject TAS-communication code!", MB_ICONERROR);
                                }
                            }

                            g_allowButtons = TRUE;
                            return 0;
                        }

                        break;
                    }

                    case BUTTON_RUN:
                    {
                        if (BN_CLICKED == HIWORD(wParam))
                        {
                            if (INVALID_HANDLE_VALUE == KAO2_gameHandle)
                            {
                                KAO2_showStatus("Please attach \"kao2.exe\" first!");
                            }
                            else if (TM_REPLAY == g_TAS_mode)
                            {
                                KAO2_showStatus("Please wait, replaying inputs...");

                                if (KAO2_runReplayingAlgorithm())
                                {
                                    KAO2_showStatus("Done and ready.");
                                }
                                else
                                {
                                    KAO2_showStatus(".");

                                    MessageBox(KAO2_mainWindow, g_lastMessage, "Jeez", MB_ICONERROR);
                                }

                                g_paintedFrame = NULL;
                            }
                            else
                            {
                                KAO2_showStatus("Please wait, recording inputs... (cancel in-game with [SELECT])");

                                if (KAO2_runRecordingAlgorithm())
                                {
                                    KAO2_showStatus("Done and ready.");
                                }
                                else
                                {
                                    KAO2_showStatus(".");

                                    MessageBox(KAO2_mainWindow, g_lastMessage, "Jeez", MB_ICONERROR);
                                }

                                g_currentFrame = NULL;
                                g_paintedFrame = NULL;
                                g_currentFrameId = (-1);
                                g_currentPage = 0;

                                KAO2_LB_refreshFrameListBox(-1);
                            }

                            g_allowButtons = TRUE;
                            return 0;
                        }

                        break;
                    }

                    case BUTTON_PLAYMODE:
                    case BUTTON_RECMODE:
                    {
                        if (BN_CLICKED == HIWORD(wParam))
                        {
                            g_TAS_mode = (LOWORD(wParam) == BUTTON_PLAYMODE) ? TM_REPLAY : TM_RECORD;

                            if (INVALID_HANDLE_VALUE == KAO2_gameHandle)
                            {
                                KAO2_showStatus("Please attach \"kao2.exe\" first!");
                            }
                            else if (KAO2_firstInjection())
                            {
                                KAO2_showStatus("<- Mode changed.");
                            }
                            else
                            {
                                KAO2_showStatus(".");

                                MessageBox(KAO2_mainWindow, g_lastMessage, "Failed to inject TAS-communication code!", MB_ICONERROR);
                            }

                            g_allowButtons = TRUE;
                            return 0;
                        }

                        break;
                    }

                    case BUTTON_OPEN_FILE:
                    {
                        if (BN_CLICKED == HIWORD(wParam))
                        {
                            if (KAO2_binfileLoad())
                            {
                                g_currentPage = 0;
                                KAO2_LB_refreshFrameListBox(-1);

                                KAO2_showStatus("New set of frames loaded!");
                            }
                            else
                            {
                                MessageBox(KAO2_mainWindow, "Error while loading input frames...", "", MB_ICONERROR);
                            }

                            g_allowButtons = TRUE;
                            return 0;
                        }

                        break;
                    }

                    case BUTTON_SAVE_FILE:
                    {
                        if (BN_CLICKED == HIWORD(wParam))
                        {
                            if (KAO2_binfileSave())
                            {
                                KAO2_showStatus("Your frame set is safely stored!");
                            }
                            else
                            {
                                MessageBox(KAO2_mainWindow, "Error while saving input frames...", "", MB_ICONERROR);
                            }

                            g_allowButtons = TRUE;
                            return 0;
                        }

                        break;
                    }

                    case BUTTON_CLEAR_ALL:
                    {
                        if (BN_CLICKED == HIWORD(wParam))
                        {
                            g_currentFrame = NULL;
                            g_currentFrameId = (-1);
                            g_currentPage = 0;

                            KAO2_clearAndUpdateFrameGui(NULL);

                            FrameNode_remove(&g_frames);
                            g_totalFrames = 0;

                            KAO2_LB_refreshFrameListBox(-1);
                            KAO2_showStatus("All input frames have been deleted...");

                            g_allowButtons = TRUE;
                            return 0;
                        }

                        break;
                    }

                    case BUTTON_PREV_PAGE:
                    case BUTTON_NEXT_PAGE:
                    {
                        if (BN_CLICKED == HIWORD(wParam))
                        {
                            g_currentFrame = NULL;
                            g_currentFrameId = (-1);
                            /* "currentPage" zostaje */

                            KAO2_LB_refreshFrameListBox((LOWORD(wParam) == BUTTON_PREV_PAGE) ? LEFT : RIGHT);

                            g_allowButtons = TRUE;
                            return 0;
                        }

                        break;
                    }

                    case BUTTON_INSERT_FRAME:
                    {
                        if (BN_CLICKED == HIWORD(wParam))
                        {
                            KAO2_LB_insertNewInputFrame();

                            g_allowButtons = TRUE;
                            return 0;
                        }

                        break;
                    }

                    case BUTTON_REMOVE_FRAME:
                    {
                        if (BN_CLICKED == HIWORD(wParam))
                        {
                            if (!KAO2_LB_removeCurrentInputFrame())
                            {
                                MessageBox(KAO2_mainWindow, "Unexpected index after frame removal.", "", MB_ICONERROR);
                            }

                            g_allowButtons = TRUE;
                            return 0;
                        }

                        break;
                    }
                }

                for (a = 0; a < 2; a++)
                {
                    for (b = 0; b < 2; b++)
                    {
                        if ((HWND)lParam == KAO2_editBoxSticks[a][b])
                        {
                            if (KAO2_stickEditNotify((HWND)lParam, HIWORD(wParam), a, b))
                            {
                                g_allowButtons = TRUE;
                                return 0;
                            }
                        }
                    }
                }

                if ((HWND)lParam == KAO2_listBoxFrames)
                {
                    if (LBN_SELCHANGE == HIWORD(wParam))
                    {
                        g_currentFrame = NULL;
                        g_currentFrameId = (-1);
                        /* "currentPage" koniecznie zostaje */

                        KAO2_LB_selectInputFrameOrUpdateGui
                        (
                            SendMessage((HWND)lParam, LB_GETCURSEL, (WPARAM)NULL, (LPARAM)NULL),
                            FALSE
                        );

                        g_allowButtons = TRUE;
                        return 0;
                    }
                }

                if ((LOWORD(wParam) >= BUTTON_FRAME_PARAM) && (BN_CLICKED == HIWORD(wParam)))
                {
                    if (NULL != g_currentFrame)
                    {
                        a = (0x01 << (LOWORD(wParam) - BUTTON_FRAME_PARAM));

                        if (BST_CHECKED == SendMessage((HWND)lParam, BM_GETCHECK, (WPARAM)NULL, (LPARAM)NULL))
                        {
                            g_currentFrame->buttons |= a;
                        }
                        else
                        {
                            g_currentFrame->buttons &= (~a);
                        }

                        KAO2_LB_selectInputFrameOrUpdateGui
                        (
                            KAO2_LB_getIndexAndAdjustPage(),
                            TRUE
                        );
                    }

                    g_allowButtons = TRUE;
                    return 0;
                }

                g_allowButtons = TRUE;
            }

            break;
        }
    }

    return DefWindowProc(hWnd, Msg, wParam, lParam);
}

////////////////////////////////////////////////////////////////
// WINAPI GUI: Creating all windows and controls
////////////////////////////////////////////////////////////////

BOOL KAO2_createWindows(HINSTANCE hInstance)
{
    HWND test_window;
    WNDCLASSEX window_class;

    const LONG_PTR stickProcedures[2] =
    {
        (LONG_PTR) KAO2_winProcLeftStick,
        (LONG_PTR) KAO2_winProcRightStick
    };

    const int KAO2TAS_WINDOW_WIDTH = 640;
    const int KAO2TAS_WINDOW_HEIGHT = 640;
    const int PADDING = 8;

    const int UPPER_BUTTONS_PER_ROW = 4;

    const int BUTTON_WIDTH = (KAO2TAS_WINDOW_WIDTH -
        (1 + UPPER_BUTTONS_PER_ROW) * PADDING) / UPPER_BUTTONS_PER_ROW;
    const int BUTTON_HEIGHT = 24;

    const int LISTA_HEIGHT = 128;
    const int GALKA_WIDTH = 128;
    const int OPTION_WIDTH = 144;

    LONG i, j, x = PADDING, y = PADDING;

    RECT real_window_rect;

    /* Register Window */

    ZeroMemory(&(window_class), sizeof(WNDCLASSEX));
    window_class.cbSize = sizeof(WNDCLASSEX);
    window_class.hInstance = hInstance;
    window_class.style = (CS_OWNDC | CS_VREDRAW | CS_HREDRAW);
    window_class.hbrBackground = (HBRUSH)(1 + COLOR_BTNFACE);
    window_class.lpfnWndProc = KAO2_windowProcedure;
    window_class.lpszClassName = KAO2TAS_WINDOW_CLASSNAME;

    if (!RegisterClassEx(&window_class))
    {
        return FALSE;
    }

    /* Create fonts */

    KAO2_font01 = CreateFont
    (
        16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, (DEFAULT_PITCH | FF_DONTCARE),
        "Verdana"
    );

    if (NULL == KAO2_font01)
    {
        return FALSE;
    }

    KAO2_font02 = CreateFont
    (
        16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, (DEFAULT_PITCH | FF_MODERN),
        "Courier New"
    );

    if (NULL == KAO2_font02)
    {
        return FALSE;
    }

    /* Create Main Window */

    real_window_rect.left = 0;
    real_window_rect.right = KAO2TAS_WINDOW_WIDTH;
    real_window_rect.top = 0;
    real_window_rect.bottom = KAO2TAS_WINDOW_HEIGHT;

    i = (WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);
    AdjustWindowRect(&real_window_rect, i, FALSE);

    real_window_rect.right -= real_window_rect.left;
    real_window_rect.bottom -= real_window_rect.top;

    KAO2_mainWindow = CreateWindow
    (
        KAO2TAS_WINDOW_CLASSNAME, KAO2TAS_WINDOW_NAME, i,
        (GetSystemMetrics(SM_CXSCREEN) - KAO2TAS_WINDOW_WIDTH) / 2,
        (GetSystemMetrics(SM_CYSCREEN) - KAO2TAS_WINDOW_HEIGHT) / 2,
        real_window_rect.right, real_window_rect.bottom,
        NULL, 0, hInstance, NULL
    );

    CHECK_WINDOW_AND_SET_FONT(KAO2_mainWindow, KAO2_font01);

    /* Status label */

    KAO2_statusLabel = CreateWindowEx
    (
        WS_EX_CLIENTEDGE, "STATIC", "",
        WS_VISIBLE | WS_CHILD,
        x, y,
        KAO2TAS_WINDOW_WIDTH - 2 * PADDING, BUTTON_HEIGHT,
        KAO2_mainWindow, NULL, hInstance, NULL
    );

    CHECK_WINDOW_AND_SET_FONT(KAO2_statusLabel, KAO2_font01);

    y += (BUTTON_HEIGHT + PADDING);

    /* Upper Buttons */

    i = 0;
    while (i < UPPER_BUTTONS_COUNT)
    {
        test_window = CreateWindow
        (
            "BUTTON", BUTTON_NAMES[i],
            WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            x, y, BUTTON_WIDTH, BUTTON_HEIGHT,
            KAO2_mainWindow, (HMENU)(BUTTON_ATTACH + i), hInstance, NULL
        );

        CHECK_WINDOW_AND_SET_FONT(test_window, KAO2_font01);

        i++;

        if (0 == (i % (UPPER_BUTTONS_COUNT / 2)))
        {
            x = PADDING;
            y += (BUTTON_HEIGHT + PADDING);
        }
        else
        {
            x += (BUTTON_WIDTH + PADDING);
        }
    }

    /* ListBox with input-frames */

    KAO2_listBoxFrames = CreateWindowEx
    (
        WS_EX_CLIENTEDGE, "LISTBOX", "",
        WS_VISIBLE | WS_CHILD | WS_VSCROLL | LBS_HASSTRINGS | LBS_NOTIFY,
        x, y, (KAO2TAS_WINDOW_WIDTH - 2 * PADDING), LISTA_HEIGHT,
        KAO2_mainWindow, NULL, hInstance, NULL
    );

    CHECK_WINDOW_AND_SET_FONT(KAO2_listBoxFrames, KAO2_font02);

    y += (LISTA_HEIGHT + PADDING);

    /* Przyciski przełączania stron */

    for (i = 0; i < 2; i++)
    {
        test_window = CreateWindow
        (
            "BUTTON", BUTTON_NAMES[UPPER_BUTTONS_COUNT + i],
            WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            x, y, BUTTON_HEIGHT, BUTTON_HEIGHT,
            KAO2_mainWindow, (HMENU)(BUTTON_PREV_PAGE + i), hInstance, NULL
        );

        CHECK_WINDOW_AND_SET_FONT(test_window, KAO2_font02);

        x += (BUTTON_HEIGHT + PADDING);
    }

    /* Label under the list box */

    i = (KAO2TAS_WINDOW_WIDTH - 6 * PADDING - 4 * BUTTON_HEIGHT);

    KAO2_listBoxStatus = CreateWindowEx
    (
        WS_EX_CLIENTEDGE, "STATIC", "Press [+] to insert new Input Frame, [-] to remove Current Frame.",
        WS_VISIBLE | WS_CHILD,
        x, y, i, BUTTON_HEIGHT,
        KAO2_mainWindow, NULL, hInstance, NULL
    );

    CHECK_WINDOW_AND_SET_FONT(KAO2_listBoxStatus, KAO2_font01);

    x += (i + PADDING);

    /* Przyciski dodawania i usuwania pojedynczych input-frames */

    for (i = 0; i < 2; i++)
    {
        test_window = CreateWindow
        (
            "BUTTON", BUTTON_NAMES[UPPER_BUTTONS_COUNT + 2 + i],
            WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            x, y, BUTTON_HEIGHT, BUTTON_HEIGHT,
            KAO2_mainWindow, (HMENU)(BUTTON_INSERT_FRAME + i), hInstance, NULL
        );

        CHECK_WINDOW_AND_SET_FONT(test_window, KAO2_font02);

        x += (BUTTON_HEIGHT + PADDING);
    }

    x = PADDING;
    y += (BUTTON_HEIGHT + 3 * PADDING);

    /* Gałki */

    for (i = 0; i < 2; i++)
    {
        KAO2_staticBoxSticks[i] = CreateWindowEx
        (
            WS_EX_CLIENTEDGE, "STATIC", "",
            WS_VISIBLE | WS_CHILD,
            x, y, GALKA_WIDTH, GALKA_WIDTH,
            KAO2_mainWindow, NULL, hInstance, NULL
        );

        if (NULL == KAO2_staticBoxSticks[i])
        {
            return FALSE;
        }

        KAO2_staticSticksProcedures[i] = (WNDPROC) SetWindowLongPtrA(KAO2_staticBoxSticks[i], GWLP_WNDPROC,  stickProcedures[i]);

        if (NULL == KAO2_staticSticksProcedures[i])
        {
            return FALSE;
        }

        x += (GALKA_WIDTH + PADDING);

        test_window = CreateWindow
        (
            "STATIC", BUTTON_NAMES[UPPER_BUTTONS_COUNT + 4 + i],
            WS_VISIBLE | WS_CHILD,
            x, y, GALKA_WIDTH, BUTTON_HEIGHT,
            KAO2_mainWindow, NULL, hInstance, NULL
        );

        CHECK_WINDOW_AND_SET_FONT(test_window, KAO2_font01);

        y += (BUTTON_HEIGHT + PADDING);

        for (j = 0; j < 2; j++)
        {
            KAO2_editBoxSticks[i][j] = CreateWindowEx
            (
                WS_EX_CLIENTEDGE, "EDIT", "",
                WS_VISIBLE | WS_CHILD | ES_LEFT,
                x, y, GALKA_WIDTH, BUTTON_HEIGHT,
                KAO2_mainWindow, NULL, hInstance, NULL
            );

            CHECK_WINDOW_AND_SET_FONT(KAO2_editBoxSticks[i][j], KAO2_font02);

            y += (BUTTON_HEIGHT + PADDING);
        }

        x = PADDING;
        y = y - 3 * (BUTTON_HEIGHT + PADDING) + (GALKA_WIDTH + PADDING);
    }

    x = (PADDING + GALKA_WIDTH + PADDING + GALKA_WIDTH + 6 * PADDING);
    y = y - 2 * (GALKA_WIDTH + PADDING) + 2 * PADDING;

    /* Wizualne wgniecenie dla chechboxów */

    test_window = CreateWindow
    (
        "BUTTON", "",
        WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
        x - 2 * PADDING, y - 2 * PADDING,
        2 * (OPTION_WIDTH + PADDING) + 2 * PADDING,
        8 * (BUTTON_HEIGHT + PADDING) + 2 * PADDING,
        KAO2_mainWindow, NULL, hInstance, NULL
    );

    if (NULL == test_window)
    {
        return FALSE;
    }

    /* CheckBoxy od atrybutów danej i-klatki */

    i = 0;

    while (i < 16)
    {
        KAO2_checkBoxParams[i] = CreateWindow
        (
            "BUTTON", FRAME_PARAM_NAMES[i],
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            x, y, OPTION_WIDTH, BUTTON_HEIGHT,
            KAO2_mainWindow, (HMENU)(BUTTON_FRAME_PARAM + i), hInstance, NULL
        );

        CHECK_WINDOW_AND_SET_FONT(KAO2_checkBoxParams[i], KAO2_font01);

        i++;

        if (0 == (i % 8))
        {
            x += (OPTION_WIDTH + PADDING);
            y -= 7 * (BUTTON_HEIGHT + PADDING);
        }
        else
        {
            y += (BUTTON_HEIGHT + PADDING);
        }
    }

    /* Done! */

    KAO2_showStatus("Start by launching \"Kao the Kangaroo: Round 2\".");
    ShowWindow(KAO2_mainWindow, SW_SHOW);

    return TRUE;
}

////////////////////////////////////////////////////////////////
// WINAPI: Entry point of the application
////////////////////////////////////////////////////////////////

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    int i;

    /* Reseting global stuff */

    g_allowButtons = TRUE;

    KAO2_mainWindow    = NULL;
    KAO2_statusLabel   = NULL;
    KAO2_listBoxFrames = NULL;
    KAO2_listBoxStatus = NULL;

    for (i = 0; i < 2; i++)
    {
        KAO2_staticBoxSticks[i] = NULL;
        KAO2_staticSticksProcedures[i] = NULL;
    }

    for (i = 0; i < 4; i++)
    {
        KAO2_editBoxSticks[i / 2][i % 2] = NULL;
    }

    for (i = 0; i < (1 << 4); i++)
    {
        KAO2_checkBoxParams[i] = NULL;
    }

    KAO2_editBoxLevel = NULL;

    KAO2_font01 = NULL;
    KAO2_font02 = NULL;

    KAO2_gameHandle = INVALID_HANDLE_VALUE;

    g_frames         = NULL;
    g_currentFrame   = NULL;
    g_paintedFrame   = NULL;

    g_currentPage    = 0;
    g_currentFrameId = (-1);
    g_totalFrames    = 0;

    g_TAS_mode = TM_REPLAY;

    /* Starting the tool */

    if (!KAO2_createWindows(hInstance))
    {
        MessageBox(NULL, "Could not create the application window!", "", MB_ICONERROR);
    }
    else
    {
        while (KAO2_windowLoop())
        {
            /* Do nothing... */
        }
    }

    /* Ending the tool */

    if (NULL != KAO2_font01)
    {
        DeleteObject((HGDIOBJ)KAO2_font01);
    }

    if (NULL != KAO2_font02)
    {
        DeleteObject((HGDIOBJ)KAO2_font02);
    }

    if (NULL != g_frames)
    {
        FrameNode_remove(&g_frames);
    }

    if (INVALID_HANDLE_VALUE != KAO2_gameHandle)
    {
        CloseHandle(KAO2_gameHandle);
    }

    return 0;
}

////////////////////////////////////////////////////////////////
