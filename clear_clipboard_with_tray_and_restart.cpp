#include <windows.h>
#include <thread>
#include <chrono>
#include <string>
#include <ctime>
#include <shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

#define ID_TRAY_EXIT 1001
#define ID_TRAY_RESTART_EXPLORER 1002
#define ID_TRAY_TOGGLE_STARTUP 1003
#define MAX_RESTART_ATTEMPTS 5
#define TOOLTIP_UPDATE_INTERVAL 60000 // 60秒

NOTIFYICONDATA nid;
HMENU hMenu;
HWND hwndGlobal = NULL;
UINT_PTR trayIconTimerId = 1;
UINT_PTR tooltipUpdateTimerId = 2;
int clearInterval = 60; // Interval in minutes to clear clipboard
bool isStartupEnabled = false;

void clearClipboard();
void restartExplorer();
void scheduleClearing(int interval);
void createTrayIcon(HWND hwnd);
void updateTrayIconTooltip();
void removeTrayIcon();
bool hasWallpaper();
void showBalloonTip(const char* title, const char* msg);
void formatTooltipMessage(char* buffer, int bufferSize, int minutesRemaining);
void setStartup();
void removeStartup();
bool checkStartup();

void clearClipboard() {
    if (OpenClipboard(nullptr)) {
        EmptyClipboard();
        CloseClipboard();
    }
    updateTrayIconTooltip(); // 更新ToolTip
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
        std::this_thread::sleep_for(std::chrono::minutes(interval));
        clearClipboard();
    }
}

void createTrayIcon(HWND hwnd) {
    memset(&nid, 0, sizeof(NOTIFYICONDATA));
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1001;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
    nid.uCallbackMessage = WM_USER + 1;
    nid.hIcon = LoadIcon(NULL, IDI_INFORMATION);
    strcpy_s(nid.szTip, "Clipboard Clearer");
    Shell_NotifyIcon(NIM_ADD, &nid);

    updateTrayIconTooltip(); // 初始化时更新ToolTip
    // Set a timer to update the tooltip every minute
    SetTimer(hwnd, tooltipUpdateTimerId, TOOLTIP_UPDATE_INTERVAL, NULL);
}

void removeTrayIcon() {
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

bool hasWallpaper() {
    char wallpaperPath[MAX_PATH];
    SystemParametersInfo(SPI_GETDESKWALLPAPER, MAX_PATH, wallpaperPath, 0);
    return strlen(wallpaperPath) > 0;
}

void showBalloonTip(const char* title, const char* msg) {
    nid.uFlags = NIF_INFO;
    strcpy_s(nid.szInfoTitle, title);
    strcpy_s(nid.szInfo, msg);
    nid.dwInfoFlags = NIIF_ERROR;
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

void restartExplorerUntilWallpaper() {
    restartExplorer();
    Sleep(1000); // Wait for a second before checking again
    int restartAttempts = 0;
    while (!hasWallpaper() && restartAttempts < MAX_RESTART_ATTEMPTS) {
        restartExplorer();
        Sleep(1000); // Wait for a second before checking again
        restartAttempts++;
    }
    if (restartAttempts == MAX_RESTART_ATTEMPTS && !hasWallpaper()) {
        showBalloonTip("Error", "Wallpaper not detected. Please check system settings.");
    }
}

void updateTrayIconTooltip() {
    time_t now = time(0);
    tm *ltm = localtime(&now);
    int minutesRemaining = clearInterval - (ltm->tm_min % clearInterval);
    char tooltip[64];
    formatTooltipMessage(tooltip, sizeof(tooltip), minutesRemaining);
    strcpy_s(nid.szTip, tooltip);
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

void formatTooltipMessage(char* buffer, int bufferSize, int minutesRemaining) {
    sprintf_s(buffer, bufferSize, "Clipboard Clearer - Clears in %d minutes", minutesRemaining);
}

void setStartup() {
    char exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);

    std::string taskCmd = "schtasks /create /f /tn \"ClipboardClearer\" /tr \"";
    taskCmd += exePath;
    taskCmd += "\" /sc onlogon /rl highest";
    system(taskCmd.c_str());

    isStartupEnabled = true;
}

void removeStartup() {
    system("schtasks /delete /f /tn \"ClipboardClearer\"");
    isStartupEnabled = false;
}

bool checkStartup() {
    char exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);

    char cmd[1024];
    sprintf_s(cmd, sizeof(cmd), "schtasks /query /tn \"ClipboardClearer\" /fo LIST | findstr /i \"%s\"", exePath);
    return system(cmd) == 0;
}

void toggleStartup() {
    if (isStartupEnabled) {
        removeStartup();
    } else {
        setStartup();
    }
}

void updateMenu() {
    CheckMenuItem(hMenu, ID_TRAY_TOGGLE_STARTUP, isStartupEnabled ? MF_CHECKED : MF_UNCHECKED);
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
                updateMenu();
                TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
                PostMessage(hwnd, WM_NULL, 0, 0);
            }
            break;
        case WM_TIMER:
            if (wParam == trayIconTimerId) {
                createTrayIcon(hwnd);
                KillTimer(hwnd, trayIconTimerId); // Stop the timer once the tray icon is recreated
            } else if (wParam == tooltipUpdateTimerId) {
                updateTrayIconTooltip(); // Update tooltip every minute
            }
            break;
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_TRAY_EXIT) {
                DestroyWindow(hwnd);
            } else if (LOWORD(wParam) == ID_TRAY_RESTART_EXPLORER) {
                std::thread(restartExplorerUntilWallpaper).detach();
            } else if (LOWORD(wParam) == ID_TRAY_TOGGLE_STARTUP) {
                toggleStartup();
                updateMenu();
            }
            break;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    isStartupEnabled = checkStartup();

    std::thread clearingThread(scheduleClearing, clearInterval); // 1 hour interval
    clearingThread.detach();

    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WindowProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "TrayIconClass", NULL };
    RegisterClassEx(&wc);
    hwndGlobal = CreateWindow("TrayIconClass", "TrayIcon", WS_OVERLAPPEDWINDOW, 0, 0, 300, 200, NULL, NULL, wc.hInstance, NULL);

    hMenu = CreatePopupMenu();
    AppendMenu(hMenu, MF_STRING, ID_TRAY_RESTART_EXPLORER, "Restart Explorer");
    AppendMenu(hMenu, MF_STRING, ID_TRAY_TOGGLE_STARTUP, "Toggle Startup");
    AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT, "Exit");

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
