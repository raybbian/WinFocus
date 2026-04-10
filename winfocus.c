/*
 * WinFocus - Focus-follows-mouse without auto-raise; raise on keypress.
 *
 * Build: cl winfocus.c /link user32.lib /SUBSYSTEM:WINDOWS
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* SPI constants in case headers lack them */
#ifndef SPI_GETACTIVEWINDOWTRACKING
#define SPI_GETACTIVEWINDOWTRACKING 0x1000
#endif
#ifndef SPI_SETACTIVEWINDOWTRACKING
#define SPI_SETACTIVEWINDOWTRACKING 0x1001
#endif
#ifndef SPI_GETACTIVEWNDTRKZORDER
#define SPI_GETACTIVEWNDTRKZORDER   0x100C
#endif
#ifndef SPI_SETACTIVEWNDTRKZORDER
#define SPI_SETACTIVEWNDTRKZORDER   0x100D
#endif
#ifndef SPI_GETACTIVEWNDTRKTIMEOUT
#define SPI_GETACTIVEWNDTRKTIMEOUT  0x2002
#endif
#ifndef SPI_SETACTIVEWNDTRKTIMEOUT
#define SPI_SETACTIVEWNDTRKTIMEOUT  0x2003
#endif

static HHOOK          g_kbHook       = NULL;
static BOOL           g_origTracking = FALSE;
static BOOL           g_origZOrder   = FALSE;
static DWORD          g_origTimeout  = 0;
static volatile LONG  g_cleanedUp    = 0;

static int is_modifier_key(DWORD vk) {
    return vk == VK_SHIFT   || vk == VK_LSHIFT   || vk == VK_RSHIFT   ||
           vk == VK_CONTROL || vk == VK_LCONTROL  || vk == VK_RCONTROL ||
           vk == VK_MENU    || vk == VK_LMENU     || vk == VK_RMENU    ||
           vk == VK_LWIN    || vk == VK_RWIN;
}

static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT *kb = (KBDLLHOOKSTRUCT *)lParam;
        if (!is_modifier_key(kb->vkCode) && kb->vkCode != VK_NONAME) {
            POINT pt;
            GetCursorPos(&pt);
            HWND hwnd = WindowFromPoint(pt);
            if (hwnd) {
                hwnd = GetAncestor(hwnd, GA_ROOT);
                if (hwnd) {
                    SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0,
                                 SWP_NOMOVE | SWP_NOSIZE);
                }
            }
        }
    }
    return CallNextHookEx(g_kbHook, nCode, wParam, lParam);
}

static void cleanup(void) {
    if (InterlockedExchange(&g_cleanedUp, 1) != 0)
        return;

    if (g_kbHook) {
        UnhookWindowsHookEx(g_kbHook);
        g_kbHook = NULL;
    }

    SystemParametersInfo(SPI_SETACTIVEWINDOWTRACKING, 0,
                         (PVOID)(INT_PTR)g_origTracking, SPIF_SENDCHANGE);
    SystemParametersInfo(SPI_SETACTIVEWNDTRKZORDER, 0,
                         (PVOID)(INT_PTR)g_origZOrder, SPIF_SENDCHANGE);
    SystemParametersInfo(SPI_SETACTIVEWNDTRKTIMEOUT, 0,
                         (PVOID)(INT_PTR)g_origTimeout, SPIF_SENDCHANGE);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    (void)hPrev; (void)lpCmd; (void)nShow;

    /* Prevent multiple instances */
    HANDLE mutex = CreateMutex(NULL, TRUE, TEXT("WinFocus_SingleInstance"));
    if (GetLastError() == ERROR_ALREADY_EXISTS)
        return 0;

    /* Save original settings */
    SystemParametersInfo(SPI_GETACTIVEWINDOWTRACKING, 0, &g_origTracking, 0);
    SystemParametersInfo(SPI_GETACTIVEWNDTRKZORDER,   0, &g_origZOrder,   0);
    SystemParametersInfo(SPI_GETACTIVEWNDTRKTIMEOUT,   0, &g_origTimeout,  0);

    /* Enable focus-follows-mouse without auto-raise */
    SystemParametersInfo(SPI_SETACTIVEWINDOWTRACKING, 0,
                         (PVOID)(INT_PTR)TRUE, SPIF_SENDCHANGE);
    SystemParametersInfo(SPI_SETACTIVEWNDTRKZORDER, 0,
                         (PVOID)(INT_PTR)FALSE, SPIF_SENDCHANGE);
    SystemParametersInfo(SPI_SETACTIVEWNDTRKTIMEOUT, 0,
                         (PVOID)(INT_PTR)0, SPIF_SENDCHANGE);

    /* Install low-level keyboard hook for raise-on-keypress */
    g_kbHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, hInst, 0);
    if (!g_kbHook) {
        cleanup();
        ReleaseMutex(mutex);
        CloseHandle(mutex);
        return 1;
    }

    /* Message loop (required for low-level hooks) */
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    cleanup();
    ReleaseMutex(mutex);
    CloseHandle(mutex);
    return 0;
}
