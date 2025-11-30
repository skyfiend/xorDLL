#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <optional>

namespace xordll {

 
class ProcessManager;
class InjectionCore;
class Logger;
class MainWindow;

 
using ProcessId = DWORD;
using ThreadId = DWORD;
using ModuleHandle = HMODULE;

 
struct ProcessInfo {
    ProcessId pid;
    std::wstring name;
    std::wstring path;
    bool is64Bit;
    HICON icon;
    
    ProcessInfo() : pid(0), is64Bit(false), icon(nullptr) {}
    
    ProcessInfo(ProcessId id, const std::wstring& n, const std::wstring& p, bool x64)
        : pid(id), name(n), path(p), is64Bit(x64), icon(nullptr) {}
};

 
struct DllInfo {
    std::wstring path;
    std::wstring name;
    std::wstring description;
    std::wstring version;
    bool is64Bit;
    bool isSigned;
    size_t fileSize;
    
    DllInfo() : is64Bit(false), isSigned(false), fileSize(0) {}
};

 
enum class InjectionMethod {
    CreateRemoteThread,
    NtCreateThreadEx,
    QueueUserAPC,
    SetWindowsHookEx,
    ManualMap,
    ThreadHijack
};

 
constexpr InjectionMethod ManualMapping = InjectionMethod::ManualMap;

 
struct InjectionResult {
    bool success;
    DWORD errorCode;
    std::wstring errorMessage;
    ModuleHandle remoteModule;
    ModuleHandle moduleHandle;   
    InjectionMethod method;
    LPVOID baseAddress;          
    SIZE_T mappedSize;           
    
    InjectionResult() 
        : success(false)
        , errorCode(0)
        , remoteModule(nullptr)
        , moduleHandle(nullptr)
        , method(InjectionMethod::CreateRemoteThread)
        , baseAddress(nullptr)
        , mappedSize(0)
    {}
    
    static InjectionResult Success(ModuleHandle module = nullptr) {
        InjectionResult result;
        result.success = true;
        result.remoteModule = module;
        result.moduleHandle = module;
        return result;
    }
    
    static InjectionResult Failure(DWORD code, const std::wstring& message) {
        InjectionResult result;
        result.success = false;
        result.errorCode = code;
        result.errorMessage = message;
        return result;
    }
};

 
enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

 
struct AppSettings {
    bool darkMode;
    bool autoRefresh;
    int refreshInterval;   
    InjectionMethod defaultMethod;
    std::wstring language;
    std::vector<std::wstring> recentDlls;
    
    AppSettings()
        : darkMode(true)
        , autoRefresh(true)
        , refreshInterval(5000)
        , defaultMethod(InjectionMethod::CreateRemoteThread)
        , language(L"en") {}
};

 
struct ColorScheme {
    COLORREF background;
    COLORREF backgroundAlt;
    COLORREF foreground;
    COLORREF accent;
    COLORREF accentHover;
    COLORREF success;
    COLORREF warning;
    COLORREF error;
    COLORREF border;
    
     
    static ColorScheme Dark() {
        ColorScheme scheme;
        scheme.background = RGB(18, 24, 20);       
        scheme.backgroundAlt = RGB(24, 32, 26);    
        scheme.foreground = RGB(220, 235, 220);    
        scheme.accent = RGB(46, 139, 87);          
        scheme.accentHover = RGB(60, 179, 113);    
        scheme.success = RGB(50, 205, 50);         
        scheme.warning = RGB(218, 165, 32);        
        scheme.error = RGB(220, 60, 60);           
        scheme.border = RGB(40, 60, 45);           
        return scheme;
    }
    
     
    static ColorScheme Light() {
        ColorScheme scheme;
        scheme.background = RGB(255, 255, 255);    
        scheme.backgroundAlt = RGB(243, 243, 243);  
        scheme.foreground = RGB(30, 30, 30);       
        scheme.accent = RGB(0, 122, 204);          
        scheme.accentHover = RGB(28, 151, 234);    
        scheme.success = RGB(22, 130, 93);         
        scheme.warning = RGB(156, 121, 0);         
        scheme.error = RGB(196, 43, 28);           
        scheme.border = RGB(204, 204, 204);        
        return scheme;
    }
};

 
using LogCallback = std::function<void(LogLevel, const std::wstring&)>;
using ProcessCallback = std::function<void(const ProcessInfo&)>;
using ProgressCallback = std::function<void(int percent, const std::wstring& status)>;

}  
