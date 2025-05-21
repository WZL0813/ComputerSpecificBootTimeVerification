#include <windows.h>
#include <fstream>
#include <filesystem>
#include <ctime>
#include <string>
#include <shlobj.h>  // 添加SHGetFolderPath和CSIDL_APPDATA声明
#include "./nlohmann/json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

// 获取AppData路径
fs::path getAppDataPath() {
    char appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        return fs::path(appDataPath) / "CSBTVBRN" / "Time";
    }
    throw std::runtime_error("无法获取AppData路径");
}

// 获取当前日期字符串
std::string getCurrentDate() {
    time_t now = time(nullptr);
    tm ltm;
    localtime_s(&ltm, &now);
    
    char buffer[11];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", &ltm);
    return std::string(buffer);
}

// 比较两个时间字符串 (格式 HH:MM:SS)
bool isEarlierThan(const std::string& time1, const std::string& time2) {
    return time1 < time2; // 直接字符串比较即可
}

#include <tlhelp32.h>

bool isProcessRunning(const wchar_t* processName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    if (!Process32FirstW(hSnapshot, &pe)) {
        CloseHandle(hSnapshot);
        return false;
    }

    do {
        if (wcscmp(pe.szExeFile, processName) == 0) {
            CloseHandle(hSnapshot);
            return true;
        }
    } while (Process32NextW(hSnapshot, &pe));

    CloseHandle(hSnapshot);
    return false;
}

int main() {
    try {
        // 获取当前程序路径
        WCHAR exePath[MAX_PATH];
        GetModuleFileNameW(NULL, exePath, MAX_PATH);
        fs::path programDir = fs::path(exePath).parent_path();
        fs::path windowExe = programDir / L"window.exe";
        
        // 获取配置目录路径
        fs::path appDataDir = getAppDataPath().parent_path();
        fs::path ifFile = appDataDir / L"if.json";
        
        // 首次启动window.exe
        if (fs::exists(windowExe)) {
            ShellExecuteW(NULL, L"open", windowExe.c_str(), NULL, NULL, SW_SHOW);
        }
        
        // 持续监控
        while (true) {
            // 检查if.json
            if (fs::exists(ifFile)) {
                std::ifstream file(ifFile);
                json config;
                file >> config;
                
                if (config.contains("value") && config["value"] == "yes") {
                    break; // 退出循环
                }
            }
            
            // 检查window.exe是否在运行
            if (!isProcessRunning(L"window.exe")) {
                // 重新启动window.exe
                if (fs::exists(windowExe)) {
                    ShellExecuteW(NULL, L"open", windowExe.c_str(), NULL, NULL, SW_SHOW);
                }
            }
            
            // 降低CPU占用
            Sleep(1000);
        }
        
    } catch (...) {
        // 忽略所有错误
    }
    
    return 0;
}