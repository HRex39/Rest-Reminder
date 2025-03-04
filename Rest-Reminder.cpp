#include <windows.h>
#include <thread>
#include <chrono>
#include <string>
#include <ctime>
#include <shlwapi.h>
#include <shlobj.h>

#pragma comment(lib, "Shlwapi.lib")

#define ID_TRAY_EXIT 1001
#define ID_TRAY_RESTART_EXPLORER 1002
#define ID_TRAY_TOGGLE_STARTUP 1003
#define ID_TRAY_CLEAR_TIMER 1004
#define ID_TRAY_ABOUT 1005
#define MAX_RESTART_ATTEMPTS 5
#define TOOLTIP_UPDATE_INTERVAL 1000 // 1 second
#define WORK_NOTIFICATION_INTERVAL 3600000 // 1 hour in milliseconds
#define WORK_NOTIFICATION_TIMER_ID 3

#define VERSION "4.5 BugFix3 20250305"
#define AUTHOR "Huang Chenrui"

NOTIFYICONDATA nid;
HMENU hMenu;
HWND hwndGlobal = NULL;
UINT_PTR trayIconTimerId = 1;
UINT_PTR tooltipUpdateTimerId = 2;
int clearInterval = 60; // Interval in minutes to clear clipboard
bool isStartupEnabled = false;
time_t startTime;

void clearClipboard();
void restartExplorer();
void scheduleClearing(int interval);
void createTrayIcon(HWND hwnd);
void updateTrayIconTooltip();
void removeTrayIcon();
bool hasWallpaper();
void showBalloonTip(const char* title, const char* msg);
void formatTooltipMessage(char* buffer, int bufferSize, int minutesWorked, int remainingSeconds);
void setStartup();
void removeStartup();
bool checkStartup();
void showWorkNotification();
void clearTimer();
void updateMenu();
void toggleStartup();
void restartExplorerUntilWallpaper();

void clearClipboard() {
    if (OpenClipboard(nullptr)) {
        EmptyClipboard();
        CloseClipboard();
    }
}

void restartExplorer() {
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
    strcpy_s(nid.szTip, "Rest Reminder");
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
    nid.dwInfoFlags = NIIF_INFO;
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
    int secondsWorked = static_cast<int>(now - startTime); // 获取工作秒数
    int minutesWorked = secondsWorked / 60; // 转换为分钟
    int remainingSeconds = secondsWorked % 60; // 获取剩余秒数
    char tooltip[64];
    formatTooltipMessage(tooltip, sizeof(tooltip), minutesWorked, remainingSeconds);
    strcpy_s(nid.szTip, tooltip);
    nid.uFlags = NIF_TIP;
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

void formatTooltipMessage(char* buffer, int bufferSize, int minutesWorked, int remainingSeconds) {
    sprintf_s(buffer, bufferSize, "You have been working for %02d:%02d", minutesWorked, remainingSeconds);
}

void setStartup() {
    char exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);

    HKEY hKey;
    RegOpenKey(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", &hKey);
    RegSetValueEx(hKey, "ClipboardClearer", 0, REG_SZ, (BYTE*)exePath, strlen(exePath) + 1);
    RegCloseKey(hKey);

    isStartupEnabled = true;
}

void removeStartup() {
    HKEY hKey;
    RegOpenKey(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", &hKey);
    RegDeleteValue(hKey, "ClipboardClearer");
    RegCloseKey(hKey);

    isStartupEnabled = false;
}

bool checkStartup() {
    HKEY hKey;
    char value[MAX_PATH];
    DWORD value_length = MAX_PATH;
    RegOpenKey(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", &hKey);
    if (RegQueryValueEx(hKey, "ClipboardClearer", NULL, NULL, (BYTE*)value, &value_length) == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }
    RegCloseKey(hKey);
    return false;
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

void showWorkNotification() {
    // 重置 startTime
    startTime = time(0);
    // Clear any existing notification
    nid.uFlags = NIF_INFO;
    strcpy_s(nid.szInfoTitle, "");
    strcpy_s(nid.szInfo, "");
    Shell_NotifyIcon(NIM_MODIFY, &nid);

    // Set up the new notification
    strcpy_s(nid.szInfoTitle, "Rest Reminder");
    strcpy_s(nid.szInfo, "You have been working for 1 hour. Please take a break.");
    nid.dwInfoFlags = NIIF_INFO;

    // Show the new notification
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

void clearTimer() {
    startTime = time(0); // 重置计时器
    // Clear any existing notification
    nid.uFlags = NIF_INFO;
    strcpy_s(nid.szInfoTitle, "");
    strcpy_s(nid.szInfo, "");
    Shell_NotifyIcon(NIM_MODIFY, &nid);

    updateTrayIconTooltip(); // 更新ToolTip
    showBalloonTip("Timer Cleared", "The timer has been cleared.");

    // 重新启动计时器
    KillTimer(hwndGlobal, WORK_NOTIFICATION_TIMER_ID);
    SetTimer(hwndGlobal, WORK_NOTIFICATION_TIMER_ID, WORK_NOTIFICATION_INTERVAL, NULL);
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
            } else if (LOWORD(lParam) == WM_LBUTTONDBLCLK) {
                clearTimer(); // 双击左键清除计时器
            }
            break;
        case WM_TIMER:
            if (wParam == trayIconTimerId) {
                createTrayIcon(hwnd);
                KillTimer(hwnd, trayIconTimerId); // Stop the timer once the tray icon is recreated
            } else if (wParam == tooltipUpdateTimerId) {
                updateTrayIconTooltip(); // Update tooltip every minute
            } else if (wParam == WORK_NOTIFICATION_TIMER_ID) {
                showWorkNotification(); // Show work notification every hour
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
            } else if (LOWORD(wParam) == ID_TRAY_CLEAR_TIMER) {
                clearTimer();
            } else if (LOWORD(wParam) == ID_TRAY_ABOUT) {
                static bool aboutBoxShown = false;
                if (!aboutBoxShown) {
                    aboutBoxShown = true;
                    char aboutMessage[256];
                    sprintf_s(aboutMessage, sizeof(aboutMessage), "Rest Reminder\nVersion: %s\nAuthor: %s", VERSION, AUTHOR);
                    MessageBox(hwnd, aboutMessage, "About", MB_OK | MB_ICONINFORMATION);
                    aboutBoxShown = false;
                }
            }
            break;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    isStartupEnabled = checkStartup();

    // 记录程序启动时间
    startTime = time(0);

    std::thread clearingThread(scheduleClearing, clearInterval); // 1 hour interval
    clearingThread.detach();

    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WindowProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "TrayIconClass", NULL };
    RegisterClassEx(&wc);
    hwndGlobal = CreateWindow("TrayIconClass", "TrayIcon", WS_OVERLAPPEDWINDOW, 0, 0, 300, 200, NULL, NULL, wc.hInstance, NULL);

    hMenu = CreatePopupMenu();
    AppendMenu(hMenu, MF_STRING, ID_TRAY_ABOUT, "About");
    AppendMenu(hMenu, MF_STRING, ID_TRAY_TOGGLE_STARTUP, "Auto Startup");
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL); // 添加分隔符
    AppendMenu(hMenu, MF_STRING, ID_TRAY_RESTART_EXPLORER, "Restart Explorer");
    AppendMenu(hMenu, MF_STRING, ID_TRAY_CLEAR_TIMER, "Clear Timer");
    AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT, "Exit");

    // Set a timer to show work notification every hour
    SetTimer(hwndGlobal, WORK_NOTIFICATION_TIMER_ID, WORK_NOTIFICATION_INTERVAL, NULL);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
