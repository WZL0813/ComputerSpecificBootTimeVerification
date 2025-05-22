#include <windows.h>
#include <fstream>
#include <filesystem>
#include <ctime>
#include <string>
#include <shellapi.h>
#include <shlobj.h>

#include <iostream>
#include "./nlohmann/json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

// 获取AppData路径
fs::path getAppDataPath() {
    char appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        fs::path path(appDataPath);
        path /= "CSBTVBRN";
        fs::create_directories(path);
        return path;
    }
    throw std::runtime_error("无法获取AppData路径");
}

// 检查是否是首次运行 
bool isFirstRun() {
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return true;
    }
    
    DWORD type;
    DWORD size = 0;
    if (RegQueryValueEx(hKey, "CSBTVBRNTimeLogger", NULL, &type, NULL, &size) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }
    
    RegCloseKey(hKey);
    return false;
}

// 添加程序到开机自启动
void addToStartup(const std::string& programPath) {
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        return;
    }
    
    RegSetValueEx(hKey, "CSBTVBRNTimeLogger", 0, REG_SZ, (const BYTE*)programPath.c_str(), programPath.size() + 1);
    RegCloseKey(hKey);
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

// 记录启动时间
void logBootTime() {
    auto appDataPath = getAppDataPath() / "Time";
    if (!fs::exists(appDataPath)) {
        fs::create_directories(appDataPath);
    }
    
    std::string filename = getCurrentDate() + ".json";
    fs::path filePath = appDataPath / filename;
    
    json logData;
    if (fs::exists(filePath)) {
        std::ifstream inFile(filePath);
        inFile >> logData;
        inFile.close();
    }
    
    time_t now = time(nullptr);
    tm ltm;
    localtime_s(&ltm, &now);
    
    char timeStr[20];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &ltm);
    
    if (!logData.contains("boot_times")) {
        logData["boot_times"] = json::array();
    }
    logData["boot_times"].push_back(timeStr);
    
    std::ofstream outFile(filePath);
    outFile << logData.dump(4);
    outFile.close();
}

// 打开配置文件中指定的软件
void openConfiguredSoftware() {
    namespace fs = std::filesystem;
    
    // 首先启动同目录下的verify.exe
    char mainPath[MAX_PATH];
    GetModuleFileName(NULL, mainPath, MAX_PATH);
    fs::path verifyPath = fs::path(mainPath).parent_path() / "verify.exe";
    if (fs::exists(verifyPath)) {
        ShellExecute(NULL, "open", verifyPath.string().c_str(), NULL, NULL, SW_SHOW);
    }
    
    // 然后处理配置文件中的软件
    fs::path startDir = getAppDataPath() / "Start";
    fs::path configPath = startDir / "main.json";
    
    // 确保目录和文件存在
    if (!fs::exists(startDir)) {
        fs::create_directories(startDir);
    }
    if (!fs::exists(configPath)) {
        std::ofstream(configPath).close(); // 创建空文件
        return;
    }
    
    try {
        std::ifstream configFile(configPath);
        if (configFile.peek() == std::ifstream::traits_type::eof()) {
            return; // 空文件不执行任何操作
        }
        
        // 逐行读取路径
        std::string line;
        while (std::getline(configFile, line)) {
            // 移除前后空白字符
            line.erase(0, line.find_first_not_of(" \t\n\r\f\v"));
            line.erase(line.find_last_not_of(" \t\n\r\f\v") + 1);
            
            // 跳过空行和注释
            if (line.empty() || line[0] == '#') {
                continue;
            }
            
            // 移除可能的引号
            if (line.front() == '"' && line.back() == '"') {
                line = line.substr(1, line.size() - 2);
            }
            
            // 检查文件是否存在
            if (fs::exists(line)) {
                ShellExecute(NULL, "open", line.c_str(), NULL, NULL, SW_SHOW);
            }
        }
    } catch (...) {
        // 忽略所有解析错误
    }
}

int main() {
    try {
        // 创建if.json配置文件
        fs::path configPath = getAppDataPath().parent_path() / "if.json";
        if (!fs::exists(configPath)) {
            fs::create_directories(configPath.parent_path());
            json config;
            config["value"] = "no";
            std::ofstream(configPath) << config.dump(4);
        }

        // 获取当前程序路径
        char path[MAX_PATH];
        GetModuleFileName(NULL, path, MAX_PATH);
        std::string programPath(path);
        
        // 如果是首次运行，添加自启动
        if (isFirstRun()) {
            addToStartup(programPath);
        }
        
        // 确保Standard.json存在
        fs::path stdFile = getAppDataPath() / "./Time/Standard.json";
        if (!fs::exists(stdFile)) {
            fs::create_directories(stdFile.parent_path());
            json stdData;
            stdData["standard_time"] = "07:10:00";
            std::ofstream(stdFile) << stdData.dump(4);
        }
        
        // 记录启动时间
        logBootTime();
        
        // 打开配置的软件
        openConfiguredSoftware();
    } catch (const std::exception& e) {
        // 静默处理异常
    }
    
    return 0;
}