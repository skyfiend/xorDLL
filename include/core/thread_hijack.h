#pragma once

#include "core/types.h"
#include <windows.h>
#include <string>
#include <functional>

namespace xordll {

 
class ThreadHijacker {
public:
    using LogCallback = std::function<void(LogLevel, const std::wstring&)>;
    
    ThreadHijacker();
    ~ThreadHijacker();
    
     
    void SetLogCallback(LogCallback callback) { m_logCallback = callback; }
    
     
    InjectionResult Inject(ProcessId processId, const std::wstring& dllPath);
    
     
    InjectionResult InjectWithThread(
        HANDLE processHandle,
        HANDLE threadHandle,
        const std::wstring& dllPath
    );

private:
     
    HANDLE FindSuitableThread(ProcessId processId);
    
     
    std::vector<BYTE> GenerateShellcode(
        LPVOID loadLibraryAddr,
        LPVOID dllPathAddr,
        LPVOID returnAddr,
        bool is64Bit
    );
    
     
    void Log(LogLevel level, const std::wstring& message);
    
    LogCallback m_logCallback;
};

 
class ContextHelper {
public:
     
    static bool SaveContext(HANDLE hThread, CONTEXT& ctx);
    
     
    static bool RestoreContext(HANDLE hThread, const CONTEXT& ctx);
    
     
    static bool SetInstructionPointer(HANDLE hThread, LPVOID address);
    
     
    static LPVOID GetInstructionPointer(HANDLE hThread);
    
     
    static bool IsThreadAlertable(HANDLE hThread);
};

}  
