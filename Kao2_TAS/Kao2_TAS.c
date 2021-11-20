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
// (2021-11-15)
//  * Przebudowanie GUI (checkboxes) oraz rozbicie opcji programu
//   na kilka flag (tryby Tool Assisted, opcje Kontrolne)
//   oraz kilka powtarzalnych makro.
// (2021-11-16)
//  * Injekcja kodu na stabilizację dowolnego FPS. (30 do 10000)
// (2021-11-18)
//  * Ulepszone komunikaty o statusie i o synchronizacji FPS,
//   możliwość wprowadzania inputów klatka-po-klatce.
// (2021-11-19)
//  * Kolejne ciekawe definicje i makra dopisane, automatyczny
//   focus na okno gry w trybie nagrywania ręcznych inputów.
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
#pragma comment (lib,      "COMDLG32.LIB")
#pragma comment (linker, "/subsystem:windows")

////////////////////////////////////////////////////////////////
// Const Defines for GUI
////////////////////////////////////////////////////////////////

#define KAO2TAS_WINDOW_CLASSNAME  "KAO2_TAS_WINDOW_CLASS"
#define KAO2TAS_WINDOW_NAME       "KAO2 :: Tool-Assisted Speedruns :: Input Tester"

#define MSGBOX_ERROR_CAPTION  "An epic fail has occurred! :)"

enum DIRECTION
{
    LEFT, RIGHT
};

#define UPPER_BUTTONS_COUNT   6
#define UPPER_CHECKBOX_COUNT  4

enum IDM
{
    IDM_BUTTON_ATTACH = 101,
    IDM_BUTTON_RUN,
    IDM_IFRAME_ADVANCE,
    IDM_BUTTON_OPEN_FILE,
    IDM_BUTTON_SAVE_FILE,
    IDM_BUTTON_CLEAR_ALL,
    IDM_CHECK_MODE,
    IDM_CHECK_STEP,
    IDM_CHECK_CLONE,
    IDM_CHECK_FRAMERULE,
    IDM_EDIT_FRAMERULE,
    IDM_BUTTON_PREV_PAGE,
    IDM_BUTTON_NEXT_PAGE,
    IDM_BUTTON_INSERT_FRAME,
    IDM_BUTTON_REMOVE_FRAME,
    IDM_BUTTON_FRAME_PARAM
};

const char * UPPER_BUTTON_NAMES[UPPER_BUTTONS_COUNT] =
{
    "Attach <KAO2>", "RUN!  |>", "I-Frame advance",
    "Open data file", "Save data file", "Clear all inputs"
};

const char * UPPER_CHECKBOX_NAMES[UPPER_CHECKBOX_COUNT] =
{
    "RECORDING mode (checked) / REPLAY mode (unchecked)",
    "Step-by-step mode (manual inputs in REC mode)",
    "Clone previous i-frame while creating a new one",
    "Stabilize framerate of updates (enter desired FPS and press ENTER)"
};

const char * OTHER_BUTTON_NAMES[4 + 2] =
{
    "<", ">", "+", "-",
    "Left Stick (x, y)", "Right Stick (x, y)"
};

const char * FRAME_PARAM_NAMES[1 << 4] =
{
    "(x) JUMP", "(o) PUNCH", "(q) ROLL", "(t) THROW",
    "(RB) STRAFE", "(LB) FPP", "(L1) unused", "(SELECT) HUD",
    "(ESC) MENU", "(START) MENU", "(D-pad U) unused", "(D-pad L) unused",
    "(D-pad R) unused", "(D-pad D) unused", "RESET CAM", "[LOADING]"
};

////////////////////////////////////////////////////////////////
// Other defines (buffers, i-frames, binary headers)
////////////////////////////////////////////////////////////////

#define BUF_SIZE         256
#define MEDIUM_BUF_SIZE   64
#define SMALL_BUF_SIZE    32
#define LASTMSG_SIZE    (BUF_SIZE / 2)

#define ENUMERATED_PROCESSES  512

enum ToolAssistedModeFlags
{
    TA_MODE_REC_OR_PLAY  = (0x01 <<  0), /* REC if set, PLAY otherwise */
    TA_MODE_STEP_BY_STEP = (0x01 <<  1),
    TA_MODE_CLONE_FRAME  = (0x01 <<  2),
    TA_MODE_FRAMERULE    = (0x01 <<  3)
};

enum ControlFlags
{
    CTRL_FLAG_ALLOWBTNS = (0x01 <<  0),
    CTRL_FLAG_IFRAMEADV = (0x01 <<  1),
    CTRL_FLAG_FPS_FOCUS = (0x01 <<  2)
};

BOOL KAO2_writeMem(DWORD address, LPCVOID from, size_t length);
BOOL KAO2_readMem(DWORD address, LPVOID into, size_t length);

struct FrameNode;
typedef struct FrameNode FrameNode_t;

struct FrameNode
{
    float sticks[2][2];
    WORD buttons; /* 16 states */
    FrameNode_t * prev;
    FrameNode_t * next;
};

#define KAO2_ANIM_FPS    30
#define KAO2_MIN_FPS     10
#define KAO2_MAX_FPS  10000

#define KAO2_WINDOW_CLASSNAME  "GLUT"
#define KAO2_WINDOW_NAME       "kangurek Kao: 2ga runda"

const char * KAO2_EXECUTABLE_NAMES[2] =
{
    "kao2.exe", "kao2_mod.exe"
};

#define FRAMES_PER_PAGE  (10 * KAO2_ANIM_FPS)

#define TAS_BINFILE_HEADER  "KAO2TAS!"

////////////////////////////////////////////////////////////////
// Globals (common, GUI, i-frames, proc)
////////////////////////////////////////////////////////////////

BOOL g_quit;

char g_lastMessage[LASTMSG_SIZE];

BYTE g_toolAssistedMode;
BYTE g_controlFlags;
FLOAT g_updateFramePortion;

HWND KAO2_mainWindow;
HWND KAO2_statusLabel;
HWND KAO2_frameRuleEditBox;
HWND KAO2_frameRuleStatus;
HWND KAO2_listBoxFrames;
HWND KAO2_listBoxStatus;
HWND KAO2_staticBoxSticks[2];
HWND KAO2_editBoxSticks[2][2];
HWND KAO2_checkBoxParams[1 << 4];
HWND KAO2_editBoxLevel;

WNDPROC KAO2_staticSticksProcedures[2];
WNDPROC KAO2_editFrameruleProcedure;
HFONT KAO2_font01;
HFONT KAO2_font02;

FrameNode_t * g_frames;
FrameNode_t * g_currentFrame;
int g_currentPage;
int g_currentFrameId;
int g_totalFrames;

HWND KAO2_gameWindow;
HANDLE KAO2_gameHandle;

////////////////////////////////////////////////////////////////
// Cool macros
////////////////////////////////////////////////////////////////

#define EQ(__a, __b)  ((__a) == (__b))
#define NE(__a, __b)  ((__a) != (__b))

#define IS_NULL(__a)      EQ(                NULL, (__a))
#define NOT_NULL(__a)     NE(                NULL, (__a))
#define IS_ONE(__a)       EQ(                   1, (__a))
#define IS_INVALID(__a)   EQ(INVALID_HANDLE_VALUE, (__a))
#define NOT_INVALID(__a)  NE(INVALID_HANDLE_VALUE, (__a))

#define IS_EITHER(__a, __b, __c)  (EQ(__a, __b) || EQ(__a, __c))

#define FAIL_IF_NULL(__a)  if IS_NULL(__a) { return FALSE; }

#define MACRO_CLAMPF(__x, __min, __max) \
    (__x > __max) ? __max : \
        ( (__x < __min) ? __min : __x  )

#define TAS_MACRO_WINDOWLOOP \
    if (!KAO2_windowLoop()) { \
        g_lastMessage[0] = '\0'; \
        return FALSE; \
    }

#define TAS_MACRO_SHOW_INPUTS \
    KAO2_clearAndUpdateFrameGui(g_currentFrame); \
    UpdateWindow(KAO2_mainWindow); \
    TAS_MACRO_WINDOWLOOP

#define CHECK_WINDOW_AND_SET_FONT(__hwnd, __font) \
    FAIL_IF_NULL(__hwnd) \
    SendMessage(__hwnd, WM_SETFONT, (WPARAM)__font, (LPARAM) 0);

#define MACRO_ASSERT_GAME_HANDLE \
    if IS_INVALID(KAO2_gameHandle) { \
        KAO2_showStatus("Please attach <Kao2> first!"); \
    }

#define MACRO_FRAMERULE_INJECTION \
    MACRO_ASSERT_GAME_HANDLE \
    else { \
        a = KAO2_frameruleInjection(); \
        if (a) { \
            if IS_ONE(a) { sprintf_s(buf, MEDIUM_BUF_SIZE, "Enabled frame synchronization (%.2f FPS limit).", (KAO2_ANIM_FPS / g_updateFramePortion)); \
            } else { sprintf_s(buf, MEDIUM_BUF_SIZE, "Disabled frame synchronization."); } \
            KAO2_showStatus(buf); \
        } else { \
            KAO2_iAmError("Failed to inject frame-rule code!"); \
        } \
    }

#define TAS_MACRO_INJECT_CODE(__array, __at) \
    FAIL_IF_NULL(KAO2_writeMem(__at, __array, sizeof(__array)))

#define TAS_MACRO_INJECT_STR(__str, __at) \
    FAIL_IF_NULL(KAO2_writeMem(__at, __str, 0x01 + strlen(__str)))

#define TAS_MACRO_STORE_DWORD(__dword, __at) \
    FAIL_IF_NULL(KAO2_writeMem(__at, &__dword, 0x04))

#define TAS_MACRO_STORE_FLAG(__value, __at) \
    flag = __value; \
    FAIL_IF_NULL(KAO2_writeMem(__at, &flag, 0x01))

#define TAS_MACRO_WAIT_FLAG_EQ(__value, __at) \
    do { \
        FAIL_IF_NULL(KAO2_readMem(__at, &flag, 0x01)) \
    } while EQ(__value, flag);

#define TAS_MACRO_WAIT_FLAG_NE_WINLOOP(__value, __at) \
    do { \
        FAIL_IF_NULL(KAO2_readMem(__at, &flag, 0x01)) \
        TAS_MACRO_WINDOWLOOP \
    } while NE(__value, flag);

#define ENABLE_TAM_FLAG(__flag) \
    g_toolAssistedMode |= (BYTE) TA_MODE_##__flag;

#define DISABLE_TAM_FLAG(__flag) \
    g_toolAssistedMode &= (BYTE) (~TA_MODE_##__flag);

#define CHECK_TAM_FLAG(__flag) \
    (TA_MODE_##__flag & g_toolAssistedMode)

#define ENABLE_CTRL_FLAG(__flag) \
    g_controlFlags |= (BYTE) CTRL_FLAG_##__flag;

#define DISABLE_CTRL_FLAG(__flag) \
    g_controlFlags &= (BYTE) (~CTRL_FLAG_##__flag);

#define CHECK_CTRL_FLAG(__flag) \
    (CTRL_FLAG_##__flag & g_controlFlags)

#define WPARAM_CLICKED \
    EQ(BN_CLICKED, HIWORD(wParam))

#define LPARAM_CHECKED \
    EQ(BST_CHECKED, SendMessage((HWND)lParam, BM_GETCHECK, (WPARAM)NULL, (LPARAM)NULL))

#define TAS_MACRO_RECORDING_MSG(details) \
    sprintf_s(buf, BUF_SIZE, "Recording inputs... [%05i] %s", a, details); \
    KAO2_showStatus(buf); \

////////////////////////////////////////////////////////////////
// KAO2 Addresses and Code Replacements
////////////////////////////////////////////////////////////////

#define KAO2_INPUTS_SEMAPHORE_ADDRESS  0x00626EA5
#define KAO2_CODE_INPUTS_ADDRESS       0x004919E0

const BYTE KAO2_INJECTION_PLAYMODE[51] =
{
  0x57,0x31,0xC0,0x31,0xC9,0xB1,0x18,0xBF,0xBC,0x69,0x62,0x00,0xF3,0xAB,0x5F,0x80,0x3D,0xA5,0x6E,0x62,0x00,0x01,0x76,0x11,0xC6,0x05,0xA5,0x6E,0x62,0x00,0x03,0x80,0x3D,0xA5,0x6E,0x62,0x00,0x03,0x74,0xF7,0xC3,0xE8,0xC2,0xF4,0xFF,0xFF,0xE9,0xED,0xFB,0xFF,0xFF
};

const BYTE KAO2_INJECTION_RECMODE[51] =
{
  0x57,0x31,0xC0,0x31,0xC9,0xB1,0x18,0xBF,0xBC,0x69,0x62,0x00,0xF3,0xAB,0x5F,0xE8,0xDC,0xF4,0xFF,0xFF,0xE8,0x07,0xFC,0xFF,0xFF,0x80,0x3D,0xA5,0x6E,0x62,0x00,0x01,0x76,0x10,0xC6,0x05,0xA5,0x6E,0x62,0x00,0x03,0x80,0x3D,0xA5,0x6E,0x62,0x00,0x03,0x74,0xF7,0xC3
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
  0xD9,0x47,0x2C,0x83,0xEC,0x08,0xDD,0x1C,0xE4,0x68,0x0C,0x97,0x60,0x00,0xA1,0xA8,0x67,0x62,0x00,0x83,0xE8,0x14,0x50,0xA1,0xAC,0x67,0x62,0x00,0x2D,0xB0,0x00,0x00,0x00,0x50,0xE8,0x33,0x9A,0x02,0x00,0x83,0xC4,0x14,0x68,0xF0,0x96,0x60,0x00,0xA1,0xA8,0x67,0x62,0x00,0x83,0xE8,0x28,0x50,0xA1,0xAC,0x67,0x62,0x00,0x2D,0xD0,0x00,0x00,0x00,0x50,0xE8,0x12,0x9A,0x02,0x00,0x83,0xC4,0x0C,0xE9,0x30,0x03,0x00,0x00
};

#define KAO2_CODE_TICK_ADDRESS    0x0048C198
#define KAO2_CONST_TICK_STEPTIME  0x0048C138
#define KAO2_CONST_TICK_FPORTION  0x0048C13C

const BYTE KAO2_INJECTION_TICK[2][158] =
{
  {0xE8,0x43,0x58,0x00,0x00,0x8A,0x86,0x84,0x03,0x00,0x00,0x84,0xC0,0x74,0x1D,0x8B,0x8E,0x80,0x03,0x00,0x00,0x85,0xC9,0x0F,0x8E,0x95,0x00,0x00,0x00,0x84,0xC0,0x74,0x0B,0x8B,0xC1,0x48,0x89,0x86,0x80,0x03,0x00,0x00,0xEB,0x0A,0xC7,0x86,0x80,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0xD9,0x46,0x10,0x83,0xEC,0x08,0xD8,0x4C,0x24,0x18,0xD9,0x54,0x24,0x18,0xD8,0x0D,0xC0,0xE1,0x5C,0x00,0xDD,0x1C,0x24,0xFF,0x15,0x74,0xD1,0x5C,0x00,0x83,0xC4,0x08,0xE8,0x2D,0x97,0x11,0x00,0x8B,0xF8,0x89,0x7C,0x24,0x08,0xDB,0x44,0x24,0x08,0xB9,0x88,0xB6,0x62,0x00,0xD8,0x7C,0x24,0x10,0xD9,0x5C,0x24,0x10,0xE8,0xD1,0xEE,0x02,0x00,0x83,0xFF,0x01,0x8B,0x54,0x24,0x10,0x89,0x96,0x64,0x03,0x00,0x00,0x7C,0x2C,0x8B,0xFF,0xD9,0x44,0x24,0x10,0x8B,0x96,0x64,0x03,0x00,0x00,0xD8,0x86,0x60,0x03,0x00,0x00,0x8B,0x06,0xD9,0x54,0x24,0x08},
  {0x8A,0x86,0x84,0x03,0x00,0x00,0x84,0xC0,0x0F,0x85,0x7E,0x00,0x00,0x00,0xD9,0x46,0x10,0xD8,0x4C,0xE4,0x10,0xD8,0x0D,0xB0,0xDF,0x5C,0x00,0xD8,0x86,0x64,0x03,0x00,0x00,0xD9,0x54,0xE4,0x10,0x83,0xEC,0x08,0xD8,0x35,0x3C,0xC1,0x48,0x00,0xDD,0x1C,0xE4,0xFF,0x15,0x6C,0xD1,0x5C,0x00,0xDB,0x1C,0xE4,0x8B,0x3C,0xE4,0x83,0xC4,0x08,0xD9,0x44,0xE4,0x10,0xD9,0x05,0x3C,0xC1,0x48,0x00,0xE8,0xE9,0x97,0x11,0x00,0xD9,0x9E,0x64,0x03,0x00,0x00,0x85,0xFF,0x74,0x38,0xA1,0x38,0xC1,0x48,0x00,0x89,0x44,0xE4,0x08,0xE8,0xE1,0x57,0x00,0x00,0xD9,0x44,0xE4,0x08,0xD8,0x86,0x60,0x03,0x00,0x00,0xD9,0x96,0x60,0x03,0x00,0x00,0x51,0xD9,0x1C,0xE4,0x8B,0x44,0xE4,0x0C,0x50,0x89,0xF1,0xE8,0xB1,0xF9,0xFF,0xFF,0x4F,0x75,0xD8,0xEB,0x05,0xE8,0xB7,0x57,0x00,0x00,0x89,0xF1,0xE8,0x90,0xB9,0xF7,0xFF,0x5F,0x5E,0x59,0xC2,0x04,0x00}
};

#define KAO2_CODE_ANIMNOTYFIER_FIXA_ADDRESS  0x00444C66
#define KAO2_CODE_ANIMNOTYFIER_FIXB_ADDRESS  0x00444D16

const BYTE KAO2_INJECTION_ANIMNOTYFIER_FIXA[11] =
{
  0xF6,0xC4,0x41,0x0F,0x85,0x9D,0x00,0x00,0x00,0x90,0x90
};

const BYTE KAO2_INJECTION_ANIMNOTYFIER_FIXB[11] =
{
  0xF6,0xC4,0x41,0x0F,0x85,0xAF,0x00,0x00,0x00,0x90,0x90
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
// FRAME NODE: Reset local input frame
////////////////////////////////////////////////////////////////

VOID FrameNode_reset(FrameNode_t * node)
{
    if NOT_NULL(node)
    {
        node->sticks[0][0] = 0;
        node->sticks[0][1] = 0;
        node->sticks[1][0] = 0;
        node->sticks[1][1] = 0;
        node->buttons = 0;
        node->prev = NULL;
        node->next = NULL;
    }
}

////////////////////////////////////////////////////////////////
// FRAME NODE: Write into the game
////////////////////////////////////////////////////////////////

BOOL FrameNode_writeToGame(FrameNode_t * node)
{
    DWORD a, b, address = KAO2_PAD_STICKS_ADDRESS;
    BYTE flag;

    for (a = 0; a < 2; a++)
    {
        for (b = 0; b < 2; b++)
        {
            FAIL_IF_NULL(KAO2_writeMem(address, &(node->sticks[a][b]), 0x04))

            address += 0x04;
        }
    }

    address = KAO2_BUTTONS_LIST_ADDRESS;
    flag = 0x01;
    b = 0x01;

    for (a = 0; a < ((1 << 4) - 1); a++)
    {
        if (b & (node->buttons))
        {
            FAIL_IF_NULL(KAO2_writeMem(address, &flag, 0x01))
        }

        b = (b << 1);
        address += 0x04;
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////
// FRAME NODE: Read from the game
////////////////////////////////////////////////////////////////

BOOL FrameNode_readFromGame(FrameNode_t * node)
{
    DWORD a, b, address = KAO2_PAD_STICKS_ADDRESS;
    BYTE flag;

    for (a = 0; a < 2; a++)
    {
        for (b = 0; b < 2; b++)
        {
            FAIL_IF_NULL(KAO2_readMem(address, &(node->sticks[a][b]), 0x04))

            address += 0x04;
        }
    }

    address = KAO2_BUTTONS_LIST_ADDRESS;
    (node->buttons) = 0;
    b = 0x01;

    for (a = 0; a < ((1 << 4) - 1); a++)
    {
        FAIL_IF_NULL(KAO2_readMem(address, &flag, 0x01))

        if (flag)
        {
            (node->buttons) |= b;
        }

        b = (b << 1);
        address += 0x04;
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////
// FRAME NODE: Create new object
////////////////////////////////////////////////////////////////

FrameNode_t * FrameNode_create(float l_x, float l_y, float r_x, float r_y, WORD buttons, FrameNode_t * parent)
{
    FrameNode_t * node = (FrameNode_t *) malloc(sizeof(FrameNode_t));
    FAIL_IF_NULL(node);

    node->sticks[0][0] = l_x;
    node->sticks[0][1] = l_y;
    node->sticks[1][0] = r_x;
    node->sticks[1][1] = r_y;

    node->buttons = buttons;

    if NOT_NULL(parent)
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
// FRAME NODE: Create empty object
////////////////////////////////////////////////////////////////

FrameNode_t * FrameNode_createEmpty(FrameNode_t * parent)
{
    return FrameNode_create(0, 0, 0, 0, 0, parent);
}

////////////////////////////////////////////////////////////////
// FRAME NODE: Create new object from an existing node
////////////////////////////////////////////////////////////////

FrameNode_t * FrameNode_createFromCopy(FrameNode_t * node, FrameNode_t * parent)
{
    if NOT_NULL(node)
    {
        return FrameNode_create
        (
            node->sticks[0][0], node->sticks[0][1],
            node->sticks[1][0], node->sticks[1][1],
            node->buttons, parent
        );
    }

    return NULL;
}

////////////////////////////////////////////////////////////////
// FRAME NODE: remove chain of input-frames
////////////////////////////////////////////////////////////////

VOID FrameNode_remove(FrameNode_t ** head_ref)
{
    FrameNode_t * current = * head_ref;
    FrameNode_t * next = NULL;

    * head_ref = NULL;

    while NOT_NULL(current)
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

    while (NOT_NULL(head) && (id > 0))
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

    if ( EQ('.', name[i - 4]) &&
        IS_EITHER(name[i - 3], 'd', 'D') &&
        IS_EITHER(name[i - 2], 'a', 'A') &&
        IS_EITHER(name[i - 1], 't', 'T') )
    {
        return TRUE;
    }

    return FALSE;
}

////////////////////////////////////////////////////////////////
// BINARY FILE WITH INPUT FRAMES: Write bytes
////////////////////////////////////////////////////////////////

BOOL KAO2_writeFile(HANDLE file, LPCVOID what, size_t length)
{
    if (!WriteFile(file, what, length, NULL, NULL))
    {
        sprintf_s(g_lastMessage, LASTMSG_SIZE, "Could not write %d bytes to the opened file!", length);
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////
// BINARY FILE WITH INPUT FRAMES: Read bytes
////////////////////////////////////////////////////////////////

BOOL KAO2_readFile(HANDLE file, LPVOID what, size_t length)
{
    if (!ReadFile(file, what, length, NULL, NULL))
    {
        sprintf_s(g_lastMessage, LASTMSG_SIZE, "Could not read %d bytes from the opened file!", length);
    }

    return TRUE;
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

    if NOT_NULL(GetSaveFileName(&ofn))
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

        if IS_INVALID(file)
        {
            sprintf_s(g_lastMessage, LASTMSG_SIZE, "Could not create that file.");
            return FALSE;
        }

        FAIL_IF_NULL(KAO2_writeFile(file, (LPCVOID) TAS_BINFILE_HEADER, 0x08))

        FAIL_IF_NULL(KAO2_writeFile(file, (LPCVOID) &g_totalFrames, 0x04))

        frame = g_frames;

        for (i = 0; i < g_totalFrames; i++)
        {
            if IS_NULL(frame)
            {
                sprintf_s(g_lastMessage, LASTMSG_SIZE, "NULL i-frame found. This should not happen...");
                CloseHandle(file);
                return FALSE;
            }

            FAIL_IF_NULL(KAO2_writeFile(file, (LPCVOID) &(frame->sticks[0][0]), 0x04))
            FAIL_IF_NULL(KAO2_writeFile(file, (LPCVOID) &(frame->sticks[0][1]), 0x04))
            FAIL_IF_NULL(KAO2_writeFile(file, (LPCVOID) &(frame->sticks[1][0]), 0x04))
            FAIL_IF_NULL(KAO2_writeFile(file, (LPCVOID) &(frame->sticks[1][1]), 0x04))
            FAIL_IF_NULL(KAO2_writeFile(file, (LPCVOID) &(frame->buttons), 0x02))

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
    DWORD a, b;
    WORD buttons;
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

    if NOT_NULL(GetOpenFileName(&ofn))
    {
        file = CreateFile(buf, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

        if IS_INVALID(file)
        {
            sprintf_s(g_lastMessage, LASTMSG_SIZE, "Could not open that file.");
            return FALSE;
        }

        FAIL_IF_NULL(KAO2_readFile(file, (LPVOID) buf, 0x08))

        if NOT_NULL(memcmp(buf, TAS_BINFILE_HEADER, 0x08))
        {
            sprintf_s(g_lastMessage, LASTMSG_SIZE, "Invalid data file header! Expected \"%s\".", TAS_BINFILE_HEADER);
            CloseHandle(file);
            return FALSE;
        }

        FAIL_IF_NULL(KAO2_readFile(file, (LPVOID) &b, 0x04))

        prev = NULL;
        new_head = NULL;

        for (a = 0; a < b; a++)
        {
            FAIL_IF_NULL(KAO2_readFile(file, (LPVOID) &l_x, 0x04))
            FAIL_IF_NULL(KAO2_readFile(file, (LPVOID) &l_y, 0x04))
            FAIL_IF_NULL(KAO2_readFile(file, (LPVOID) &r_x, 0x04))
            FAIL_IF_NULL(KAO2_readFile(file, (LPVOID) &r_y, 0x04))
            FAIL_IF_NULL(KAO2_readFile(file, (LPVOID) &buttons, 0x02))

            frame = FrameNode_create(l_x, l_y, r_x, r_y, buttons, prev);

            prev = frame;

            if IS_NULL(new_head)
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
// WINAPI GUI: Main window loop
////////////////////////////////////////////////////////////////

BOOL KAO2_windowLoop()
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
// WINAPI GUI: Updating EditBox control of Gamepad Stick co-ord
////////////////////////////////////////////////////////////////

VOID KAO2_updateStickEdit(const FrameNode_t * frame, int id, int coord)
{
    char buf[SMALL_BUF_SIZE];

    if NOT_NULL(frame)
    {
        sprintf_s(buf, SMALL_BUF_SIZE, "%.7f", (frame->sticks[id][coord]));
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

BOOL KAO2_stickEditNotify(HWND editbox, WORD notify, int id, int coord)
{
    int a, b;
    char buf[SMALL_BUF_SIZE];
    float dummy;

    if (EQ(EN_CHANGE, notify) && NOT_NULL(g_currentFrame))
    {
        GetWindowText(editbox, buf, SMALL_BUF_SIZE);

        dummy = strtof(buf, NULL);

        if IS_EITHER(errno, HUGE_VAL, ERANGE)
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
    WORD buttons;

    for (i = 0; i < 2; i++)
    {
        KAO2_updateStickStaticAndEdits(frame, i, TRUE);
    }

    for (i = 0; i < (1 << 4); i++)
    {
        SendMessage(KAO2_checkBoxParams[i], BM_SETCHECK, (WPARAM) BST_UNCHECKED, (LPARAM) NULL);
    }

    if NOT_NULL(frame)
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
    SetWindowText(KAO2_statusLabel, msg);
    /* UpdateWindow(KAO2_mainWindow); */
}

////////////////////////////////////////////////////////////////
// WINAPI GUI: Show error message
////////////////////////////////////////////////////////////////

VOID KAO2_iAmError(const char * caption)
{
    char buf[BUF_SIZE];

    KAO2_showStatus(caption);

    if NOT_NULL(g_lastMessage[0])
    {
        sprintf_s(buf, BUF_SIZE, "%s\n\n%s", caption, g_lastMessage);

        MessageBox(KAO2_mainWindow, buf, MSGBOX_ERROR_CAPTION, MB_ICONERROR);
    }
}

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
// TOOL-ASSISTED MANIPULATION: Inject anim-frame updating
// synchronization code into "Kao the Kangaroo: Round 2" (retail)
////////////////////////////////////////////////////////////////

LONG KAO2_frameruleInjection()
{
    const FLOAT updatePortion = g_updateFramePortion;
    const FLOAT stepTime      = (updatePortion / KAO2_ANIM_FPS);

    if (CHECK_TAM_FLAG(FRAMERULE))
    {
        TAS_MACRO_STORE_DWORD(updatePortion, KAO2_CONST_TICK_FPORTION);
        TAS_MACRO_STORE_DWORD(stepTime,      KAO2_CONST_TICK_STEPTIME);

        TAS_MACRO_INJECT_CODE(KAO2_INJECTION_TICK[1], KAO2_CODE_TICK_ADDRESS);

        return 1;
    }
    else
    {
        TAS_MACRO_INJECT_CODE(KAO2_INJECTION_TICK[0], KAO2_CODE_TICK_ADDRESS);

        return 2;
    }
}

////////////////////////////////////////////////////////////////
// TOOL-ASSISTED MANIPULATION: Show text status and prepare
// floating-point data values before injecting anim-frame
// updating sycnrhonization code.
////////////////////////////////////////////////////////////////

VOID KAO2_frameruleInjectionPrepare(BOOL sendCode, float fps)
{
    char buf[BUF_SIZE];
    LONG a = FALSE;
    float fp;

    if (fps <= KAO2_ANIM_FPS)
    {
        fps = KAO2_ANIM_FPS;
        a = TRUE;
    }
    else if (fps >= KAO2_MAX_FPS)
    {
        fps = KAO2_MAX_FPS;
        a = TRUE;
    }

    if (a)
    {
        sprintf_s(buf, BUF_SIZE, "%.2f", fps);
        SetWindowText(KAO2_frameRuleEditBox, buf);
    }

    fp = (KAO2_ANIM_FPS / fps);
    a = (LONG) (fps / KAO2_MIN_FPS);  // ((1.0 / fp) * (KAO2_ANIM_FPS / KAO2_MIN_FPS))

    sprintf_s
    (
        buf, BUF_SIZE,
            "| Anim-frame portion per update: | %.5f of 1.0 |\n"
            "| Max updates per Display Callback (delta %.2f s): | %4i |\n"
            "| Frame advance on lowest (%d FPS) framerate: | %.5f |",
        fp, (1.0 / KAO2_MIN_FPS), a, KAO2_MIN_FPS, (a * fp)
    );
    SetWindowText(KAO2_frameRuleStatus, buf);

    g_updateFramePortion = fp;

    if (sendCode)
    {
        MACRO_FRAMERULE_INJECTION
    }
}

////////////////////////////////////////////////////////////////
// TOOL-ASSISTED MANIPULATION: Make dynamic changes to
// "Kao the Kangaroo: Round 2" (retail) game code when
// switching Tool-Assisted Modes.
////////////////////////////////////////////////////////////////

BOOL KAO2_nextInjections()
{
    const BYTE * code;
    size_t length;

    /* InputManager - Semaphore unlock */

    BYTE
    TAS_MACRO_STORE_FLAG(0x00, KAO2_INPUTS_SEMAPHORE_ADDRESS);

    /* InputManager - Choosing algorithm */

    if (CHECK_TAM_FLAG(REC_OR_PLAY))
    {
        code = KAO2_INJECTION_RECMODE;
        length = sizeof(KAO2_INJECTION_RECMODE);
    }
    else
    {
        code = KAO2_INJECTION_PLAYMODE;
        length = sizeof(KAO2_INJECTION_PLAYMODE);
    }

    FAIL_IF_NULL(KAO2_writeMem(KAO2_CODE_INPUTS_ADDRESS, (LPCVOID)code, length))

    /* Continue injecting new code... :) */

    return KAO2_frameruleInjection();
}

////////////////////////////////////////////////////////////////
// TOOL-ASSISTED MANIPULATION: Prepare the retail version
// of "Kao the Kangaroo: Round 2" for usage with this cool tool
////////////////////////////////////////////////////////////////

BOOL KAO2_firstInjection()
{
    /* eAnimNotyfier::onUpdate() - fix double notifies */

    TAS_MACRO_INJECT_CODE(KAO2_INJECTION_ANIMNOTYFIER_FIXA, KAO2_CODE_ANIMNOTYFIER_FIXA_ADDRESS);

    TAS_MACRO_INJECT_CODE(KAO2_INJECTION_ANIMNOTYFIER_FIXB, KAO2_CODE_ANIMNOTYFIER_FIXB_ADDRESS);

    /* Micro messages (FPS and TAS status, visible in-game) */

    TAS_MACRO_INJECT_STR(KAO2_TASMSG_STANDBY, KAO2_TASMSG_ADDRESS);

    TAS_MACRO_INJECT_STR(KAO2_AVGFPS, KAO2_AVGFPS_ADDRESS);

    TAS_MACRO_INJECT_CODE(KAO2_INJECTION_MSGS, KAO2_CODE_MSGS_ADDRESS);

    /* Continue injecting new code... :) */

    return KAO2_nextInjections();
}

////////////////////////////////////////////////////////////////
// TOOL-ASSISTED MANIPULATION: Recording input frames
////////////////////////////////////////////////////////////////

BOOL KAO2_runRecordingAlgorithm()
{
    DWORD a, b, rec = TRUE;
    char buf[BUF_SIZE];
    BYTE flag;
    FrameNode_t dummy;
    FrameNode_t * dynamic_frame;

    /* Remove current set of i-frames... */

    FrameNode_remove(& g_frames);
    g_totalFrames = 0;

    g_currentFrame = (& dummy);
    FrameNode_reset(& dummy);
    KAO2_clearAndUpdateFrameGui(NULL);

    while (rec)
    {
        /* Status message (in-game + tool) */

        a = (1 + g_totalFrames);

        sprintf_s(buf, MEDIUM_BUF_SIZE, KAO2_TASMSG_REC, a);
        TAS_MACRO_INJECT_STR(buf, KAO2_TASMSG_ADDRESS)

        TAS_MACRO_RECORDING_MSG("");

        /* Is this loading mode? */

        FAIL_IF_NULL(KAO2_readMem(KAO2_LOADING_FLAG_ADDRESS, &flag, 0x01))

        if IS_ONE(flag)
        {
            /* Remember that the game is loading and wait */

            FrameNode_reset(& dummy);
            dummy.buttons = (0x01 << KF_ACTION_LOADING);

            TAS_MACRO_STORE_FLAG(0x00, KAO2_INPUTS_SEMAPHORE_ADDRESS);

            TAS_MACRO_WAIT_FLAG_EQ(0x01, KAO2_LOADING_FLAG_ADDRESS);

            TAS_MACRO_STORE_FLAG(0x02, KAO2_INPUTS_SEMAPHORE_ADDRESS);
        }
        else
        {
            /* Notify Kao that I want to read user's input frame... */

            TAS_MACRO_STORE_FLAG(0x02, KAO2_INPUTS_SEMAPHORE_ADDRESS);

            TAS_MACRO_RECORDING_MSG("(set focus to the game window now!)");

            /* Kao accepts the request - wait for synchronization. */

            a = CHECK_TAM_FLAG(STEP_BY_STEP);

            if (a)
            {
                SetForegroundWindow(KAO2_gameWindow);
            }

            TAS_MACRO_WAIT_FLAG_NE_WINLOOP(0x03, KAO2_INPUTS_SEMAPHORE_ADDRESS);

            if (!a)
            {
                a = CHECK_TAM_FLAG(STEP_BY_STEP);

                /* Step-by-step mode was disabled before this request */
                /* was sent. Read inputs prepared by the game. */

                FAIL_IF_NULL(FrameNode_readFromGame(& dummy))

                /* Present input frame in GUI */

                TAS_MACRO_SHOW_INPUTS
            }

            if (a)
            {
                /* Step-by-step mode was enabled before this request */
                /* was sent, or was enabled just-in-time while waiting */
                /* for synchronization, so wait until the user prepares */
                /* his own set of inputs to override game's set. */

                a = (1 + g_totalFrames);
                TAS_MACRO_RECORDING_MSG
                (
                    "(uncheck \"Step-by-step\" to unfreeze the game on next frame;"
                    " press \"I-Frame advance\" when the new input set is ready)"
                );

                if (!CHECK_TAM_FLAG(CLONE_FRAME))
                {
                    FrameNode_reset(& dummy);
                    KAO2_clearAndUpdateFrameGui(NULL);
                }

                DISABLE_CTRL_FLAG(IFRAMEADV);

                do
                {
                    TAS_MACRO_WINDOWLOOP
                }
                while (!CHECK_CTRL_FLAG(IFRAMEADV));

                TAS_MACRO_RECORDING_MSG("");

                /* Replace in-game input status */

                FAIL_IF_NULL(FrameNode_writeToGame(& dummy))
            }
        }

        /* Create dynamically-allocated input frame */

        dynamic_frame = FrameNode_createFromCopy((& dummy), (dummy.prev));

        g_totalFrames++;

        if IS_NULL(dummy.prev)
        {
            g_frames = dynamic_frame;
        }

        (dummy.prev) = dynamic_frame;

        /* Condition for stopping the recording */

        if ((0x01 << KF_R1) & (dummy.buttons))
        {
            rec = FALSE;
        }
    }

    /* Micro message */

    TAS_MACRO_INJECT_STR(KAO2_TASMSG_STANDBY, KAO2_TASMSG_ADDRESS)

    /* The game will not have to wait on its current input loop */

    TAS_MACRO_STORE_FLAG(0x00, KAO2_INPUTS_SEMAPHORE_ADDRESS);

    KAO2_clearAndUpdateFrameGui(NULL);

    return TRUE;
}

////////////////////////////////////////////////////////////////
// TOOL-ASSISTED MANIPULATION: Replaying input frames
////////////////////////////////////////////////////////////////

BOOL KAO2_runReplayingAlgorithm()
{
    DWORD a, b, counter = 1;
    char buf[SMALL_BUF_SIZE];
    BYTE flag;

    g_currentFrame = g_frames;

    while NOT_NULL(g_currentFrame)
    {
        /* Micro message */

        sprintf_s(buf, SMALL_BUF_SIZE, KAO2_TASMSG_PLAY, counter, g_totalFrames);
        TAS_MACRO_INJECT_STR(buf, KAO2_TASMSG_ADDRESS)

        /* Present input frame in GUI */

        TAS_MACRO_SHOW_INPUTS;

        /* Is this loading mode? */

        if ((0x01 << KF_ACTION_LOADING) & (g_currentFrame->buttons))
        {
            /* Unlock the game - it must process inputs from the last frame... */

            TAS_MACRO_STORE_FLAG(0x00, KAO2_INPUTS_SEMAPHORE_ADDRESS);

            /* Wait for the loading to begin */

            TAS_MACRO_WAIT_FLAG_EQ(0x00, KAO2_LOADING_FLAG_ADDRESS);

            /* Block inputs (just in case) */

            TAS_MACRO_STORE_FLAG(0x02, KAO2_INPUTS_SEMAPHORE_ADDRESS);

            /* Wait for the loading to finish */

            TAS_MACRO_WAIT_FLAG_EQ(0x01, KAO2_LOADING_FLAG_ADDRESS);
        }
        else
        {
            /* Notify Kao that I want to insert a custom input frame... */

            TAS_MACRO_STORE_FLAG(0x02, KAO2_INPUTS_SEMAPHORE_ADDRESS);

            /* Wait for Kao to process game logic (from prev frame) */
            /* and clear input data (for this frame) */

            TAS_MACRO_WAIT_FLAG_NE_WINLOOP(0x03, KAO2_INPUTS_SEMAPHORE_ADDRESS);

            /* Send data for this input-frame */

            FAIL_IF_NULL(FrameNode_writeToGame(g_currentFrame))
        }

        /* The game will unfreeze once the the next iteration */
        /* of this loop starts, or when this loop ends - until then */
        /* it waits for the semaphore to change from "0x03" to anything else */

        g_currentFrame = (g_currentFrame->next);
        counter++;
    }

    /* Micro message */

    TAS_MACRO_INJECT_STR(KAO2_TASMSG_STANDBY, KAO2_TASMSG_ADDRESS)

    /* Unlocks the game from its current input loop */

    TAS_MACRO_STORE_FLAG(0x00, KAO2_INPUTS_SEMAPHORE_ADDRESS);

    KAO2_clearAndUpdateFrameGui(NULL);

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
            (1 + id), g_totalFrames,
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

    if EQ(LEFT, direction)
    {
        g_currentPage -= 1;

        if (g_currentPage < 0)
        {
            g_currentPage = 0;
        }
    }
    else if EQ(RIGHT, direction)
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

    while (NOT_NULL(frame) && (counter > 0))
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

    if (IS_NULL(g_currentFrame) && (lb_id >= 0))
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

            sprintf_s(buf, BUF_SIZE, "(Updated frame #%05d)", (1 + g_currentFrameId));
        }
        else
        {
            SendMessage(KAO2_listBoxFrames, LB_SETCURSEL, (WPARAM) lb_id, (LPARAM) NULL);

            sprintf_s(buf, BUF_SIZE, "(Selected frame #%05d)", (1 + g_currentFrameId));
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
    char buf[SMALL_BUF_SIZE];
    FrameNode_t * prev_frame;

    if IS_NULL(g_frames)
    {
        g_frames = FrameNode_createEmpty(NULL);
        g_currentFrame = g_frames;
        g_currentFrameId = 0;
        g_totalFrames = 1;
        g_currentPage = 0;
    }
    else
    {
        if IS_NULL(g_currentFrame)
        {
            g_currentFrame = g_frames;
            prev_frame = NULL;

            g_currentFrameId = 0;
            while NOT_NULL(g_currentFrame)
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

        if (CHECK_TAM_FLAG(CLONE_FRAME))
        {
            g_currentFrame = FrameNode_createFromCopy(prev_frame, prev_frame);
        }
        else
        {
            g_currentFrame = FrameNode_createEmpty(prev_frame);
        }

        g_totalFrames++;
    }

    sprintf_s(buf, SMALL_BUF_SIZE, "Inserted frame #%03d.", (1 + g_currentFrameId));
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
    int i = (1 + g_currentFrameId);
    char buf[BUF_SIZE];

    FrameNode_t * prev_frame;
    FrameNode_t * next_frame;

    if NOT_NULL(g_currentFrame)
    {
        prev_frame = (g_currentFrame->prev);
        next_frame = (g_currentFrame->next);

        if (NOT_NULL(prev_frame) && (g_currentFrameId > 0))
        {
            /* There is at least one frame before */

            (prev_frame->next) = next_frame;
        }
        else if (IS_NULL(prev_frame) && IS_NULL(g_currentFrameId))
        {
            /* Must be the first frame */

            if NE(g_currentFrame, g_frames)
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

        if (IS_NULL(next_frame) && EQ(g_currentFrameId, g_totalFrames))
        {
            /* Was the last frame (or the only frame) */

            g_currentFrame = prev_frame;
            g_currentFrameId--;
        }
        else if (IS_NULL(next_frame) && (g_currentFrameId < g_totalFrames))
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

            if NOT_NULL(g_currentFrame)
            {
                SetDCPenColor(ps.hdc, RGB(255, 0, 0));

                /* Clamped direction (-1.f to +1.f) extended */
                /* to the size of the half of square control */
                fp.x = MACRO_CLAMPF((g_currentFrame->sticks[id][0]), (-1.f), (+1.f)) * halves.x;
                fp.y = MACRO_CLAMPF((g_currentFrame->sticks[id][1]), (-1.f), (+1.f)) * halves.y;

                /* Line end point on the square control. */
                /* Notice how Left Stick is inverted on Y-axis, */
                /* while the Right Stick is not! */
                line_end.x = halves.x + ((int)fp.x);
                line_end.y = halves.y + (IS_NULL(id) ? (-1) : 1) * ((int) fp.y);

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
            if NOT_NULL(g_currentFrame)
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
                    g_currentFrame->sticks[id][1] = (IS_NULL(id) ? (-1) : 1) * fp.y;

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
// WINAPI GUI: Indirect callback procedure
// for accepting input in an EditBox control
////////////////////////////////////////////////////////////////

LRESULT CALLBACK KAO2_winProcEditFramerule(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    char buf[SMALL_BUF_SIZE];
    float fps;

    if (EQ(WM_KEYDOWN, Msg) && CHECK_CTRL_FLAG(ALLOWBTNS) && EQ(VK_RETURN, wParam))
    {
        DISABLE_CTRL_FLAG(ALLOWBTNS)

        GetWindowText(hWnd, buf, SMALL_BUF_SIZE);

        fps = strtof(buf, NULL);

        if IS_EITHER(errno, HUGE_VAL, ERANGE)
        {
            fps = 0;
        }
        else
        {
            fps = MACRO_CLAMPF(fps, KAO2_ANIM_FPS, KAO2_MAX_FPS);
        }

        KAO2_frameruleInjectionPrepare(TRUE, fps);

        ENABLE_CTRL_FLAG(ALLOWBTNS)
        return 0;
    }

    return CallWindowProc(KAO2_editFrameruleProcedure, hWnd, Msg, wParam, lParam);
}

////////////////////////////////////////////////////////////////
// WINAPI GUI: Callback procedure for the main window of this tool
////////////////////////////////////////////////////////////////

LRESULT CALLBACK KAO2_windowProcedure(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    BOOL check_more_buttons = TRUE;
    DWORD a, b;
    char buf[BUF_SIZE];

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

        /* Left mouse button pressed */
        /* (skipping BUTTONS ALLOWED control flag) */
        case WM_LBUTTONDOWN:
        {
            SendMessage(KAO2_staticBoxSticks[0], WM_LBUTTONDOWN, wParam, lParam);
            SendMessage(KAO2_staticBoxSticks[1], WM_LBUTTONDOWN, wParam, lParam);

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
                    if (CHECK_CTRL_FLAG(ALLOWBTNS) && WPARAM_CLICKED)
                    {
                        DISABLE_CTRL_FLAG(ALLOWBTNS)

                        KAO2_findGameProcess();

                        if IS_INVALID(KAO2_gameHandle)
                        {
                            KAO2_iAmError("Could not attach the \"kao2.exe\" game process!");
                        }
                        else
                        {
                            KAO2_showStatus("KAO2 attached. ^^");

                            if (KAO2_firstInjection())
                            {
                                KAO2_showStatus("KAO2 injected and ready!");
                            }
                            else
                            {
                                KAO2_iAmError("Failed to inject TAS-communication code!");
                            }
                        }

                        ENABLE_CTRL_FLAG(ALLOWBTNS)
                        return 0;
                    }

                    check_more_buttons = FALSE;
                    break;
                }

                /* Run TAS algorithm */
                case IDM_BUTTON_RUN:
                {
                    if (CHECK_CTRL_FLAG(ALLOWBTNS) && WPARAM_CLICKED)
                    {
                        DISABLE_CTRL_FLAG(ALLOWBTNS)

                        MACRO_ASSERT_GAME_HANDLE
                        else if (CHECK_TAM_FLAG(REC_OR_PLAY))
                        {
                            a = 0;
                            TAS_MACRO_RECORDING_MSG("")

                            if (KAO2_runRecordingAlgorithm())
                            {
                                KAO2_showStatus("Done and ready.");
                            }
                            else
                            {
                                /* Semaphore unlock in case of a sudden shutdown */
                                a = 0;
                                KAO2_writeMem(KAO2_INPUTS_SEMAPHORE_ADDRESS, &a, 0x01);

                                KAO2_iAmError("Unable to continue execution of the input-reading algorithm!");
                            }

                            g_currentFrame = NULL;
                            g_currentFrameId = (-1);
                            g_currentPage = 0;

                            KAO2_LB_refreshFrameListBox(-1);
                        }
                        else
                        {
                            KAO2_showStatus("Please wait, replaying inputs...");

                            if (KAO2_runReplayingAlgorithm())
                            {
                                KAO2_showStatus("Done and ready.");
                            }
                            else
                            {
                                /* Semaphore unlock in case of a sudden shutdown */
                                a = 0;
                                KAO2_writeMem(KAO2_INPUTS_SEMAPHORE_ADDRESS, &a, 0x01);

                                KAO2_iAmError("Unable to continue execution of the input-recording algorithm!");
                            }

                            g_currentFrame = NULL;
                        }

                        ENABLE_CTRL_FLAG(ALLOWBTNS)
                        return 0;
                    }

                    check_more_buttons = FALSE;
                    break;
                }

                /* Advance one input-frame */
                /* (skipping BUTTONS ALLOWED control flag) */
                case IDM_IFRAME_ADVANCE:
                {
                    if (WPARAM_CLICKED)
                    {
                        ENABLE_CTRL_FLAG(IFRAMEADV)

                        return 0;
                    }

                    check_more_buttons = FALSE;
                    break;
                }

                /* Switching the TAS mode */
                case IDM_CHECK_MODE:
                {
                    if (CHECK_CTRL_FLAG(ALLOWBTNS) && WPARAM_CLICKED)
                    {
                        DISABLE_CTRL_FLAG(ALLOWBTNS)

                        if (LPARAM_CHECKED)
                        {
                            a = TRUE;
                            ENABLE_TAM_FLAG(REC_OR_PLAY)
                        }
                        else
                        {
                            a = FALSE;
                            DISABLE_TAM_FLAG(REC_OR_PLAY)
                        }

                        MACRO_ASSERT_GAME_HANDLE
                        else if (KAO2_nextInjections())
                        {
                            sprintf_s(buf, MEDIUM_BUF_SIZE, "TAS mode changed to RE%s :)", (a ? "CORDING" : "PLAY"));
                            KAO2_showStatus(buf);
                        }
                        else
                        {
                            KAO2_iAmError("Failed to inject TAS-communication code!");
                        }

                        ENABLE_CTRL_FLAG(ALLOWBTNS)
                        return 0;
                    }

                    check_more_buttons = FALSE;
                    break;
                }

                /* Switching the step-by-step option */
                /* (skipping BUTTONS ALLOWED control flag) */
                case IDM_CHECK_STEP:
                {
                    if (WPARAM_CLICKED)
                    {
                        if (LPARAM_CHECKED)
                        {
                            a = TRUE;
                            ENABLE_TAM_FLAG(STEP_BY_STEP)
                        }
                        else
                        {
                            a = FALSE;
                            DISABLE_TAM_FLAG(STEP_BY_STEP)
                        }

                        return 0;
                    }

                    check_more_buttons = FALSE;
                    break;
                }

                /* Switching the frame-cloning option */
                /* (skipping BUTTONS ALLOWED control flag) */
                case IDM_CHECK_CLONE:
                {
                    if (WPARAM_CLICKED)
                    {
                        if (LPARAM_CHECKED)
                        {
                            ENABLE_TAM_FLAG(CLONE_FRAME)
                        }
                        else
                        {
                            DISABLE_TAM_FLAG(CLONE_FRAME)
                        }

                        return 0;
                    }

                    check_more_buttons = FALSE;
                    break;
                }

                /* Switching the framerule option */
                case IDM_CHECK_FRAMERULE:
                {
                    if (CHECK_CTRL_FLAG(ALLOWBTNS) && WPARAM_CLICKED)
                    {
                        DISABLE_CTRL_FLAG(ALLOWBTNS)

                        if (LPARAM_CHECKED)
                        {
                            ENABLE_TAM_FLAG(FRAMERULE)
                        }
                        else
                        {
                            DISABLE_TAM_FLAG(FRAMERULE)
                        }

                        MACRO_FRAMERULE_INJECTION

                        ENABLE_CTRL_FLAG(ALLOWBTNS)
                        return 0;
                    }

                    check_more_buttons = FALSE;
                    break;
                }

                /* Framerule EditBox */
                case IDM_EDIT_FRAMERULE:
                {
                    if (CHECK_CTRL_FLAG(ALLOWBTNS))
                    {
                        DISABLE_CTRL_FLAG(ALLOWBTNS)
                        a = FALSE;

                        if EQ(EN_SETFOCUS, HIWORD(wParam))
                        {
                            a = TRUE;
                            ENABLE_CTRL_FLAG(FPS_FOCUS);
                        }
                        else if EQ(EN_KILLFOCUS, HIWORD(wParam))
                        {
                            a = TRUE;
                            DISABLE_CTRL_FLAG(FPS_FOCUS);
                        }

                        ENABLE_CTRL_FLAG(ALLOWBTNS)
                        if (a) { return 0; }
                    }

                    check_more_buttons = FALSE;
                    break;
                }

                /* Reading inputs from a file */
                case IDM_BUTTON_OPEN_FILE:
                {
                    if (CHECK_CTRL_FLAG(ALLOWBTNS) && WPARAM_CLICKED)
                    {
                        DISABLE_CTRL_FLAG(ALLOWBTNS)

                        if (KAO2_binfileLoad())
                        {
                            g_currentPage = 0;
                            KAO2_LB_refreshFrameListBox(-1);

                            KAO2_showStatus("New set of frames loaded!");
                        }
                        else
                        {
                            KAO2_iAmError("An error occurred while loading input frames...");
                        }

                        ENABLE_CTRL_FLAG(ALLOWBTNS)
                        return 0;
                    }

                    check_more_buttons = FALSE;
                    break;
                }

                /* Writing inputs to a file */
                case IDM_BUTTON_SAVE_FILE:
                {
                    if (CHECK_CTRL_FLAG(ALLOWBTNS) && WPARAM_CLICKED)
                    {
                        DISABLE_CTRL_FLAG(ALLOWBTNS)

                        if (KAO2_binfileSave())
                        {
                            KAO2_showStatus("Your frame set is safely stored!");
                        }
                        else
                        {
                            KAO2_iAmError("An error occurred while saving input frames...");
                        }

                        ENABLE_CTRL_FLAG(ALLOWBTNS)
                        return 0;
                    }

                    check_more_buttons = FALSE;
                    break;
                }

                /* Clear all input frames */
                case IDM_BUTTON_CLEAR_ALL:
                {
                    if (CHECK_CTRL_FLAG(ALLOWBTNS) && WPARAM_CLICKED)
                    {
                        DISABLE_CTRL_FLAG(ALLOWBTNS)

                        g_currentFrame = NULL;
                        g_currentFrameId = (-1);
                        g_currentPage = 0;

                        KAO2_clearAndUpdateFrameGui(NULL);

                        FrameNode_remove(&g_frames);
                        g_totalFrames = 0;

                        KAO2_LB_refreshFrameListBox(-1);
                        KAO2_showStatus("All input frames have been deleted...");

                        ENABLE_CTRL_FLAG(ALLOWBTNS)
                        return 0;
                    }

                    check_more_buttons = FALSE;
                    break;
                }

                /* Switching list-box pages */
                case IDM_BUTTON_PREV_PAGE:
                case IDM_BUTTON_NEXT_PAGE:
                {
                    if (CHECK_CTRL_FLAG(ALLOWBTNS) && WPARAM_CLICKED)
                    {
                        DISABLE_CTRL_FLAG(ALLOWBTNS)

                        g_currentFrame = NULL;
                        g_currentFrameId = (-1);
                        /* "currentPage" stays unchanged */

                        KAO2_LB_refreshFrameListBox(EQ(LOWORD(wParam), IDM_BUTTON_PREV_PAGE) ? LEFT : RIGHT);

                        ENABLE_CTRL_FLAG(ALLOWBTNS)
                        return 0;
                    }

                    check_more_buttons = FALSE;
                    break;
                }

                /* Inserting a new i-frame */
                case IDM_BUTTON_INSERT_FRAME:
                {
                    if (CHECK_CTRL_FLAG(ALLOWBTNS) && WPARAM_CLICKED)
                    {
                        DISABLE_CTRL_FLAG(ALLOWBTNS)

                        KAO2_LB_insertNewInputFrame();

                        ENABLE_CTRL_FLAG(ALLOWBTNS)
                        return 0;
                    }

                    check_more_buttons = FALSE;
                    break;
                }

                /* Removing current i-frame */
                case IDM_BUTTON_REMOVE_FRAME:
                {
                    if (CHECK_CTRL_FLAG(ALLOWBTNS) && WPARAM_CLICKED)
                    {
                        DISABLE_CTRL_FLAG(ALLOWBTNS)

                        if (!KAO2_LB_removeCurrentInputFrame())
                        {
                            g_lastMessage[0] = '\0';
                            KAO2_iAmError("Unexpected index after frame removal.");
                        }

                        ENABLE_CTRL_FLAG(ALLOWBTNS)
                        return 0;
                    }

                    check_more_buttons = FALSE;
                    break;
                }
            }

            /* Gamepad Sticks, 4 EditBoxes */
            /* (skipping BUTTONS ALLOWED control flag) */
            if (check_more_buttons)
            {
                for (a = 0; a < 2; a++)
                {
                    for (b = 0; b < 2; b++)
                    {
                        if EQ((HWND)lParam, KAO2_editBoxSticks[a][b])
                        {
                            if (KAO2_stickEditNotify((HWND)lParam, HIWORD(wParam), a, b))
                            {
                                return 0;
                            }
                        }
                    }
                }
            }

            /* Listbox of i-frames */
            if (check_more_buttons && EQ((HWND)lParam, KAO2_listBoxFrames))
            {
                if (CHECK_CTRL_FLAG(ALLOWBTNS) && EQ(LBN_SELCHANGE, HIWORD(wParam)))
                {
                    DISABLE_CTRL_FLAG(ALLOWBTNS)

                    g_currentFrame = NULL;
                    g_currentFrameId = (-1);
                    /* "currentPage" must stay unchanged */

                    KAO2_LB_selectInputFrameOrUpdateGui
                    (
                        SendMessage((HWND)lParam, LB_GETCURSEL, (WPARAM)NULL, (LPARAM)NULL),
                        FALSE
                    );

                    ENABLE_CTRL_FLAG(ALLOWBTNS)
                    return 0;
                }

                check_more_buttons = FALSE;
            }

            /* Current i-frame parameters, CheckBoxes */
            if (check_more_buttons && (LOWORD(wParam) >= IDM_BUTTON_FRAME_PARAM))
            {
                if (WPARAM_CLICKED && NOT_NULL(g_currentFrame))
                {
                    a = (0x01 << (LOWORD(wParam) - IDM_BUTTON_FRAME_PARAM));

                    if (LPARAM_CHECKED)
                    {
                        (g_currentFrame->buttons) |= a;
                    }
                    else
                    {
                        (g_currentFrame->buttons) &= (~a);
                    }

                    /* listBox update blocked by BUTTONS ALLOWED flag */
                    /* (disabled while in REC or PLAY modes) */
                    if (CHECK_CTRL_FLAG(ALLOWBTNS))
                    {
                        KAO2_LB_selectInputFrameOrUpdateGui
                        (
                            KAO2_LB_getIndexAndAdjustPage(),
                            TRUE
                        );
                    }

                    return 0;
                }

                check_more_buttons = FALSE;
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
    RECT real_window_rect;

    const LONG_PTR stickProcedures[2] =
    {
        (LONG_PTR) KAO2_winProcLeftStick,
        (LONG_PTR) KAO2_winProcRightStick
    };

    const int KAO2TAS_WINDOW_WIDTH    = 640;
    const int KAO2TAS_WINDOW_HEIGHT   = 680;
    const int PADDING                 =   8;
    const int WINDOW_WIDTH_WO_PADDING = KAO2TAS_WINDOW_WIDTH - 2 * PADDING;

    const int UPPER_BUTTONS_PER_ROW = 3;
    const int BUTTON_WIDTH          = (KAO2TAS_WINDOW_WIDTH -
        (1 + UPPER_BUTTONS_PER_ROW) * PADDING) / UPPER_BUTTONS_PER_ROW;

    const int FONT_HEIGHT   = 16;
    const int LABEL_HEIGHT  = FONT_HEIGHT + 4;
    const int BUTTON_HEIGHT = 24;

    const int LISTBOX_HEIGHT    =  96;
    const int BLACKSQUARE_WIDTH = 128;
    const int OPTION_WIDTH      = 144;

    LONG i, j, x = PADDING, y = PADDING, x2, y2;

    /* Register Window */

    ZeroMemory(&(window_class), sizeof(WNDCLASSEX));
    window_class.cbSize = sizeof(WNDCLASSEX);
    window_class.hInstance = hInstance;
    window_class.style = (CS_OWNDC | CS_VREDRAW | CS_HREDRAW);
    window_class.hbrBackground = (HBRUSH)(1 + COLOR_BTNFACE);
    window_class.lpfnWndProc = KAO2_windowProcedure;
    window_class.lpszClassName = KAO2TAS_WINDOW_CLASSNAME;

    FAIL_IF_NULL(RegisterClassEx(&window_class))

    /* Create fonts */

    KAO2_font01 = CreateFont
    (
        FONT_HEIGHT, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, (DEFAULT_PITCH | FF_DONTCARE),
        "Verdana"
    );

    FAIL_IF_NULL(KAO2_font01)

    KAO2_font02 = CreateFont
    (
        FONT_HEIGHT, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, (DEFAULT_PITCH | FF_MODERN),
        "Courier New"
    );

    FAIL_IF_NULL(KAO2_font02)

    /* Create Main Window */

    real_window_rect.left   = 0;
    real_window_rect.right  = KAO2TAS_WINDOW_WIDTH;
    real_window_rect.top    = 0;
    real_window_rect.bottom = KAO2TAS_WINDOW_HEIGHT;

    i = (WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);
    AdjustWindowRect(&real_window_rect, i, FALSE);

    real_window_rect.right  -= real_window_rect.left;
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

    y2 = (2 * LABEL_HEIGHT);

    KAO2_statusLabel = CreateWindowEx
    (
        WS_EX_CLIENTEDGE, "STATIC", "",
        WS_VISIBLE | WS_CHILD,
        x, y, WINDOW_WIDTH_WO_PADDING, y2,
        KAO2_mainWindow, NULL, hInstance, NULL
    );

    CHECK_WINDOW_AND_SET_FONT(KAO2_statusLabel, KAO2_font01);

    y += (y2 + PADDING);

    /* Upper Buttons */

    y2 = y;

    i = 0;
    while (i < UPPER_BUTTONS_COUNT)
    {
        test_window = CreateWindow
        (
            "BUTTON", UPPER_BUTTON_NAMES[i],
            WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            x, y, BUTTON_WIDTH, BUTTON_HEIGHT,
            KAO2_mainWindow, (HMENU)(IDM_BUTTON_ATTACH + i), hInstance, NULL
        );

        CHECK_WINDOW_AND_SET_FONT(test_window, KAO2_font01);

        i++;

        if IS_NULL(i % UPPER_BUTTONS_PER_ROW)
        {
            x = PADDING;
            y += (BUTTON_HEIGHT + PADDING);
        }
        else
        {
            x += (BUTTON_WIDTH + PADDING);
        }
    }

    x = PADDING;
    y = y2 + 2 * (BUTTON_HEIGHT + PADDING);
    y2 = y;

    /* Upper Checkboxes */

    for (i = 0; i < UPPER_CHECKBOX_COUNT; i++)
    {
        test_window = CreateWindow
        (
            "BUTTON", UPPER_CHECKBOX_NAMES[i],
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            x, y, WINDOW_WIDTH_WO_PADDING, LABEL_HEIGHT,
            KAO2_mainWindow, (HMENU)(IDM_CHECK_MODE + i), hInstance, NULL
        );

        CHECK_WINDOW_AND_SET_FONT(test_window, KAO2_font01);

        y += LABEL_HEIGHT;
    }

    /* EditBox and FrameRate status */

    x2 = ((KAO2TAS_WINDOW_WIDTH - 3 * PADDING) / 5);

    KAO2_frameRuleEditBox = CreateWindowEx
    (
        WS_EX_CLIENTEDGE, "EDIT", "",
        WS_VISIBLE | WS_CHILD | ES_LEFT,
        x, y, x2, BUTTON_HEIGHT,
        KAO2_mainWindow, (HMENU) IDM_EDIT_FRAMERULE, hInstance, NULL
    );

    CHECK_WINDOW_AND_SET_FONT(KAO2_frameRuleEditBox, KAO2_font01);

    KAO2_editFrameruleProcedure = (WNDPROC) SetWindowLongPtrA(KAO2_frameRuleEditBox, GWLP_WNDPROC, (LONG_PTR) KAO2_winProcEditFramerule);

    FAIL_IF_NULL(KAO2_editFrameruleProcedure)

    x += (x2 + PADDING);
    y2 = (3 * LABEL_HEIGHT);

    KAO2_frameRuleStatus = CreateWindowEx
    (
        WS_EX_CLIENTEDGE, "STATIC", "",
        WS_VISIBLE | WS_CHILD,
        x, y, 4 * x2, y2,
        KAO2_mainWindow, NULL, hInstance, NULL
    );

    CHECK_WINDOW_AND_SET_FONT(KAO2_frameRuleStatus, KAO2_font01);

    x = PADDING;
    y += (y2 + PADDING);

    /* ListBox with input-frames */

    KAO2_listBoxFrames = CreateWindowEx
    (
        WS_EX_CLIENTEDGE, "LISTBOX", "",
        WS_VISIBLE | WS_CHILD | WS_VSCROLL | LBS_HASSTRINGS | LBS_NOTIFY,
        x, y, WINDOW_WIDTH_WO_PADDING, LISTBOX_HEIGHT,
        KAO2_mainWindow, NULL, hInstance, NULL
    );

    CHECK_WINDOW_AND_SET_FONT(KAO2_listBoxFrames, KAO2_font02);

    y += LISTBOX_HEIGHT;

    /* Buttons for page switching */

    for (i = 0; i < 2; i++)
    {
        test_window = CreateWindow
        (
            "BUTTON", OTHER_BUTTON_NAMES[i],
            WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            x, y, BUTTON_HEIGHT, LABEL_HEIGHT,
            KAO2_mainWindow, (HMENU)(IDM_BUTTON_PREV_PAGE + i), hInstance, NULL
        );

        CHECK_WINDOW_AND_SET_FONT(test_window, KAO2_font01);

        x += (BUTTON_HEIGHT + PADDING);
    }

    /* Label under the list box */

    x2 = (KAO2TAS_WINDOW_WIDTH - 6 * PADDING - 4 * BUTTON_HEIGHT);

    KAO2_listBoxStatus = CreateWindowEx
    (
        WS_EX_CLIENTEDGE, "STATIC", "Press [+] to insert new Input Frame, [-] to remove Current Frame.",
        WS_VISIBLE | WS_CHILD,
        x, y, x2, LABEL_HEIGHT,
        KAO2_mainWindow, NULL, hInstance, NULL
    );

    CHECK_WINDOW_AND_SET_FONT(KAO2_listBoxStatus, KAO2_font01);

    x += (x2 + PADDING);

    /* Buttons for adding and removing a single input-frame */

    for (i = 0; i < 2; i++)
    {
        test_window = CreateWindow
        (
            "BUTTON", OTHER_BUTTON_NAMES[2 + i],
            WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            x, y, BUTTON_HEIGHT, LABEL_HEIGHT,
            KAO2_mainWindow, (HMENU)(IDM_BUTTON_INSERT_FRAME + i), hInstance, NULL
        );

        CHECK_WINDOW_AND_SET_FONT(test_window, KAO2_font01);

        x += (BUTTON_HEIGHT + PADDING);
    }

    x = PADDING;
    y += (LABEL_HEIGHT + 2 * PADDING);

    /* Gamepad Sticks - static controls */

    for (i = 0; i < 2; i++)
    {
        KAO2_staticBoxSticks[i] = CreateWindowEx
        (
            WS_EX_CLIENTEDGE, "STATIC", "",
            WS_VISIBLE | WS_CHILD,
            x, y, BLACKSQUARE_WIDTH, BLACKSQUARE_WIDTH,
            KAO2_mainWindow, NULL, hInstance, NULL
        );

        FAIL_IF_NULL(KAO2_staticBoxSticks[i])

        KAO2_staticSticksProcedures[i] = (WNDPROC) SetWindowLongPtrA(KAO2_staticBoxSticks[i], GWLP_WNDPROC,  stickProcedures[i]);

        FAIL_IF_NULL(KAO2_staticSticksProcedures[i])

        x += (BLACKSQUARE_WIDTH + PADDING);

        test_window = CreateWindow
        (
            "STATIC", OTHER_BUTTON_NAMES[4 + i],
            WS_VISIBLE | WS_CHILD,
            x, y, BLACKSQUARE_WIDTH, BUTTON_HEIGHT,
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
                x, y, BLACKSQUARE_WIDTH, BUTTON_HEIGHT,
                KAO2_mainWindow, NULL, hInstance, NULL
            );

            CHECK_WINDOW_AND_SET_FONT(KAO2_editBoxSticks[i][j], KAO2_font02);

            y += (BUTTON_HEIGHT + PADDING);
        }

        x = PADDING;
        y = y - 3 * (BUTTON_HEIGHT + PADDING) + (BLACKSQUARE_WIDTH + PADDING);
    }

    x = (PADDING + BLACKSQUARE_WIDTH + PADDING + BLACKSQUARE_WIDTH + 6 * PADDING);
    y = y - 2 * (BLACKSQUARE_WIDTH + PADDING) + 2 * PADDING;

    /* Visual dent for checkboxes area */

    test_window = CreateWindow
    (
        "BUTTON", "",
        WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
        x - 2 * PADDING, y - 2 * PADDING,
        2 * (OPTION_WIDTH + PADDING) + 2 * PADDING,
        8 * (LABEL_HEIGHT + PADDING) + 2 * PADDING,
        KAO2_mainWindow, NULL, hInstance, NULL
    );

    FAIL_IF_NULL(test_window)

    /* CheckBoxes representing the i-frame attributes */

    i = 0;

    while (i < 16)
    {
        KAO2_checkBoxParams[i] = CreateWindow
        (
            "BUTTON", FRAME_PARAM_NAMES[i],
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            x, y, OPTION_WIDTH, LABEL_HEIGHT,
            KAO2_mainWindow, (HMENU)(IDM_BUTTON_FRAME_PARAM + i), hInstance, NULL
        );

        CHECK_WINDOW_AND_SET_FONT(KAO2_checkBoxParams[i], KAO2_font01);

        i++;

        if IS_NULL(i % 8)
        {
            x += (OPTION_WIDTH + PADDING);
            y -= 7 * (LABEL_HEIGHT + PADDING);
        }
        else
        {
            y += (LABEL_HEIGHT + PADDING);
        }
    }

    /* Quick setup with default options */

    KAO2_frameruleInjectionPrepare(FALSE, 0);
    KAO2_showStatus("Start by launching \"Kao the Kangaroo: Round 2\".");

    ShowWindow(KAO2_mainWindow, SW_SHOW);
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

    g_toolAssistedMode   = 0;
    g_controlFlags       = CTRL_FLAG_ALLOWBTNS;
    g_updateFramePortion = 1.0f;

    KAO2_mainWindow  = NULL;
    KAO2_statusLabel = NULL;

    KAO2_frameRuleEditBox = NULL;
    KAO2_frameRuleStatus  = NULL;

    KAO2_listBoxFrames = NULL;
    KAO2_listBoxStatus = NULL;

    for (i = 0; i < 2; i++)
    {
        KAO2_staticBoxSticks[i] = NULL;
        KAO2_staticSticksProcedures[i] = NULL;
    }

    KAO2_editFrameruleProcedure = NULL;

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

    KAO2_gameWindow = NULL;
    KAO2_gameHandle = INVALID_HANDLE_VALUE;

    g_frames         = NULL;
    g_currentFrame   = NULL;

    g_currentPage    = 0;
    g_currentFrameId = (-1);
    g_totalFrames    = 0;

    /* Starting the program */

    if (!KAO2_createWindows(hInstance))
    {
        MessageBox(NULL, "Could not create the application window!", MSGBOX_ERROR_CAPTION, MB_ICONERROR);
    }
    else
    {
        while (KAO2_windowLoop())
        {
            if (NOT_INVALID(KAO2_gameHandle) && GetExitCodeProcess(KAO2_gameHandle, &i))
            {
                if NE(STILL_ACTIVE, i)
                {
                    KAO2_gameWindow = NULL;
                    KAO2_gameHandle = INVALID_HANDLE_VALUE;

                    KAO2_showStatus("Game has been closed! Please re-launch \"Kao the Kangaroo: Round 2\".");
                }
            }
        }
    }

    /* Ending the program */

    if NOT_NULL(KAO2_font01)
    {
        DeleteObject((HGDIOBJ)KAO2_font01);
    }

    if NOT_NULL(KAO2_font02)
    {
        DeleteObject((HGDIOBJ)KAO2_font02);
    }

    if NOT_NULL(g_frames)
    {
        FrameNode_remove(&g_frames);
    }

    if NOT_INVALID(KAO2_gameHandle)
    {
        CloseHandle(KAO2_gameHandle);
    }

    return 0;
}

////////////////////////////////////////////////////////////////
