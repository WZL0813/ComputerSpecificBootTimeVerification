#define UNICODE
#define _UNICODE
#include <windows.h>
#include <fstream>
#include <filesystem>
#include <ctime>
#include <string>
#include <shlobj.h>
#include "./nlohmann/json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

// 函数前置声明
fs::path getAppDataPath();
std::string getStandardTime();
bool isTimeUp();

// 全局变量
HWND hWnd;
HHOOK hKeyboardHook;
bool isVerified = false;
const std::wstring PASSWORD = L"3180";

// 密码错误计数器
int wrongPasswordCount = 0;

// 获取AppData路径
fs::path getAppDataPath() {
    WCHAR appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        return fs::path(appDataPath) / L"CSBTVBRN" / L"Time";
    }
    throw std::runtime_error("无法获取AppData路径");
}

// 读取标准时间
std::string getStandardTime() {
    fs::path stdFile = getAppDataPath() / "Standard.json";
    if (!fs::exists(stdFile)) {
        return "";
    }

    std::ifstream file(stdFile);
    json data;
    file >> data;
    file.close();

    if (data.contains("standard_time")) {
        return data["standard_time"];
    }
    return "";
}

// 检查是否到达标准时间
bool isTimeUp() {
    std::string standardTime = getStandardTime();
    if (standardTime.empty()) {
        return false;
    }

    time_t now = time(nullptr);
    tm ltm;
    localtime_s(&ltm, &now);
    
    char currentTime[20];
    strftime(currentTime, sizeof(currentTime), "%H:%M:%S", &ltm);

    return std::string(currentTime) >= standardTime;
}

// 键盘钩子回调函数
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        KBDLLHOOKSTRUCT* pkbhs = (KBDLLHOOKSTRUCT*)lParam;
        
        // 只处理键按下事件
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            // 屏蔽特定关闭组合键
            if (pkbhs->vkCode == VK_F4 && (GetAsyncKeyState(VK_MENU) & 0x8000)) { // Alt+F4
                return 1;
            }
            if (pkbhs->vkCode == VK_ESCAPE && (GetAsyncKeyState(VK_CONTROL) & 0x8000)) { // Ctrl+Esc
                return 1;
            }
            if (pkbhs->vkCode == VK_LWIN || pkbhs->vkCode == VK_RWIN) { // Win键
                return 1;
            }
            
            // 允许其他组合键（如输入法切换）
            if (pkbhs->vkCode == VK_SHIFT || pkbhs->vkCode == VK_CONTROL || pkbhs->vkCode == VK_MENU) {
                return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
            }
        }
    }
    return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

// 禁用任务管理器
void disableTaskManager() {
    HKEY hKey;
    RegCreateKeyExW(HKEY_CURRENT_USER, 
                  L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System", 
                  0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
    DWORD value = 1;
    RegSetValueExW(hKey, L"DisableTaskMgr", 0, REG_DWORD, (const BYTE*)&value, sizeof(value));
    RegCloseKey(hKey);
}

// 启用任务管理器
void enableTaskManager() {
    HKEY hKey;
    RegOpenKeyExW(HKEY_CURRENT_USER, 
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System", 
                0, KEY_WRITE, &hKey);
    RegDeleteValueW(hKey, L"DisableTaskMgr");
    RegCloseKey(hKey);
}

// 窗口过程函数
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            // 创建大标题
            HWND hTitle = CreateWindowW(L"STATIC", L"当前开机时间不合理！请输入密码确认！",
                        WS_CHILD | WS_VISIBLE | SS_CENTER,
                        0, 50, GetSystemMetrics(SM_CXSCREEN), 40,
                        hWnd, NULL, NULL, NULL);
            
            // 设置标题字体
            HFONT hTitleFont = CreateFont(28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                       CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                       DEFAULT_PITCH | FF_DONTCARE, L"微软雅黑");
            SendMessage(hTitle, WM_SETFONT, (WPARAM)hTitleFont, TRUE);
            SetTextColor((HDC)hTitle, RGB(255, 0, 0));
            
        // 获取屏幕尺寸
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);

        // 创建密码输入框（居中靠上）
        HWND hEdit = CreateWindowW(L"EDIT", L"", 
          WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD | ES_AUTOHSCROLL,
          (screenWidth - 400)/2, screenHeight/2 - 50, 400, 30,
          hWnd, (HMENU)1, NULL, NULL);

        // 创建验证按钮（居中靠下）
        CreateWindowW(L"BUTTON", L"验证", 
          WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
          (screenWidth - 100)/2, screenHeight/2 + 50, 100, 30,
          hWnd, (HMENU)2, NULL, NULL);

        // 强制设置输入框焦点
        SetFocus(hEdit);
        PostMessage(hWnd, WM_NEXTDLGCTL, (WPARAM)hEdit, TRUE);  
            
            // 设置输入框字体
            HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                   CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                   DEFAULT_PITCH | FF_DONTCARE, L"微软雅黑");
            SendMessage(GetDlgItem(hWnd, 1), WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(GetDlgItem(hWnd, 2), WM_SETFONT, (WPARAM)hFont, TRUE);
            
            // 设置输入框焦点
            SetFocus(hEdit);
            
            // 设置输入框焦点
            SetFocus(GetDlgItem(hWnd, 1));
            break;
        }
        
        case WM_COMMAND: {
            if (LOWORD(wParam) == 2) { // 验证按钮
                wchar_t password[100];
                GetWindowTextW(GetDlgItem(hWnd, 1), password, 100);

            if (std::wstring(password) == PASSWORD) {
                isVerified = true;
                wrongPasswordCount = 0; // 重置计数器
                // 修改if.json为yes
                fs::path ifFile = getAppDataPath().parent_path() / "if.json";
                json config;
                config["value"] = "yes";
                std::ofstream(ifFile) << config.dump(4);
                    


                    // 先启用任务管理器再关闭窗口
                    enableTaskManager();
                    // 发送关闭消息而不是直接销毁
                    PostMessage(hWnd, WM_CLOSE, 0, 0);
                } else {
                    wrongPasswordCount++;
                    // 错误提示
                    MessageBoxW(hWnd, L"密码错误!", L"错误", MB_ICONERROR);
                    // 清空输入框并重新获取焦点
                    SetWindowTextW(GetDlgItem(hWnd, 1), L"");
                    SetFocus(GetDlgItem(hWnd, 1));

                        if (wrongPasswordCount >= 3) {
                            WCHAR exePath[MAX_PATH];
                            GetModuleFileNameW(NULL, exePath, MAX_PATH);
                            fs::path wbExe = fs::path(exePath).parent_path() / "wb.exe";
        
                            if (fs::exists(wbExe)) {
                                ShellExecuteW(NULL, L"open", wbExe.c_str(), NULL, NULL, SW_SHOW);
                            }

                }
            }
        }
            break;
        }
        
        case WM_CLOSE:
        case WM_QUIT:
        case WM_DESTROY:
            if (!isVerified && !isTimeUp()) {
                // 被关闭后重新启动自己
                WCHAR exePath[MAX_PATH];
                GetModuleFileNameW(NULL, exePath, MAX_PATH);
                ShellExecuteW(NULL, L"open", exePath, NULL, NULL, SW_SHOW);
            } else {
                enableTaskManager();
                PostQuitMessage(0);
            }
            return 0;
            
        case WM_SYSCOMMAND: {
            // 禁用系统菜单命令
            switch (wParam) {
                case SC_CLOSE:
                case SC_MINIMIZE:
                case SC_MAXIMIZE:
                case SC_RESTORE:
                    return 0;
            }
            break;
        }
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

// 定时检查时间
void CALLBACK TimerProc(HWND hWnd, UINT /*msg*/, UINT_PTR /*id*/, DWORD /*time*/) {
    if (isTimeUp()) {
        isVerified = true;
        DestroyWindow(hWnd);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nCmdShow*/) {
    // 注册窗口类
    const wchar_t* WINDOW_CLASS = L"WindowClass";
    
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = WINDOW_CLASS;
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassEx(&wc);

    // 创建全屏窗口
    hWnd = CreateWindowExW(
        WS_EX_TOPMOST,
        WINDOW_CLASS,
        L"fuck",
        WS_POPUP | WS_VISIBLE,
        0, 0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL, hInstance, NULL);

    if (!hWnd) {
        MessageBoxW(NULL, L"窗口创建失败!", L"错误", MB_ICONERROR);
        return 0;
    }

    // 调试输出
    OutputDebugStringW(L"窗口创建成功\n");
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    // 设置键盘钩子
    hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, hInstance, 0);

    // 禁用任务管理器
    disableTaskManager();

    // 设置定时器检查时间
    SetTimer(hWnd, 1, 1000, TimerProc);

    // 主消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 清理
    UnhookWindowsHookEx(hKeyboardHook);
    KillTimer(hWnd, 1);

    return (int)msg.wParam;
}