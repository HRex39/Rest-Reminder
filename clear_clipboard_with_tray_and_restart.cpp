#include <windows.h>
#include <thread>
#include <chrono>

#define ID_TRAY_EXIT 1001
#define ID_TRAY_RESTART_EXPLORER 1002

NOTIFYICONDATA nid;
HMENU hMenu;
HWND hwndGlobal = NULL;
UINT_PTR trayIconTimerId = 1;

void clearClipboard();
void restartExplorer();
void scheduleClearing(int interval);
void createTrayIcon(HWND hwnd);
void removeTrayIcon();

void clearClipboard() {
    if (OpenClipboard(nullptr)) {
        EmptyClipboard();
        CloseClipboard();
    }
}

void restartExplorer() {
    // Use CreateProcess to restart Explorer without affecting the current process
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    system("taskkill /f /im explorer.exe");
    Sleep(1000); // Give it a second to ensure the process has ended
    CreateProcess(NULL, (LPSTR)"explorer.exe", NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    // Set a timer to recreate the tray icon
    SetTimer(hwndGlobal, trayIconTimerId, 3000, NULL); // 3 seconds delay
}

void scheduleClearing(int interval) {
    while (true) {
        clearClipboard();
        std::this_thread::sleep_for(std::chrono::hours(interval));
    }
}

void createTrayIcon(HWND hwnd) {
    memset(&nid, 0, sizeof(NOTIFYICONDATA));
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1001;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_USER + 1;
    nid.hIcon = LoadIcon(NULL, IDI_INFORMATION);
    strcpy_s(nid.szTip, "Clipboard Clearer");
    Shell_NotifyIcon(NIM_ADD, &nid);
}

void removeTrayIcon() {
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            createTrayIcon(hwnd);
            break;
        case WM_DESTROY:
            removeTrayIcon();
            PostQuitMessage(0);
            break;
        case WM_USER + 1:
            if (LOWORD(lParam) == WM_RBUTTONUP) {
                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hwnd);
                TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
                PostMessage(hwnd, WM_NULL, 0, 0);
            }
            break;
        case WM_TIMER:
            if (wParam == trayIconTimerId) {
                createTrayIcon(hwnd);
                KillTimer(hwnd, trayIconTimerId); // Stop the timer once the tray icon is recreated
            }
            break;
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_TRAY_EXIT) {
                DestroyWindow(hwnd);
            } else if (LOWORD(wParam) == ID_TRAY_RESTART_EXPLORER) {
                restartExplorer();
            }
            break;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    std::thread clearingThread(scheduleClearing, 1); // 1 hour interval
    clearingThread.detach();

    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WindowProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "TrayIconClass", NULL };
    RegisterClassEx(&wc);
    hwndGlobal = CreateWindow("TrayIconClass", "TrayIcon", WS_OVERLAPPEDWINDOW, 0, 0, 300, 200, NULL, NULL, wc.hInstance, NULL);

    hMenu = CreatePopupMenu();
    AppendMenu(hMenu, MF_STRING, ID_TRAY_RESTART_EXPLORER, "Restart Explorer");
    AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT, "Exit");

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
