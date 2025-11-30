 

#include "core/thread_hijack.h"
#include "utils/string_utils.h"
#include <tlhelp32.h>

namespace xordll {

 
 
 

ThreadHijacker::ThreadHijacker() = default;
ThreadHijacker::~ThreadHijacker() = default;

InjectionResult ThreadHijacker::Inject(ProcessId processId, const std::wstring& dllPath) {
    InjectionResult result;
    result.success = false;
    result.method = InjectionMethod::ThreadHijack;
    
    Log(LogLevel::Info, L"Starting thread hijack injection to PID: " + std::to_wstring(processId));
    
     
    HANDLE hProcess = OpenProcess(
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
        FALSE, processId);
    
    if (!hProcess) {
        result.errorCode = GetLastError();
        result.errorMessage = L"Failed to open process: " + utils::FormatWindowsError(result.errorCode);
        Log(LogLevel::Error, result.errorMessage);
        return result;
    }
    
     
    HANDLE hThread = FindSuitableThread(processId);
    if (!hThread) {
        result.errorCode = GetLastError();
        result.errorMessage = L"Failed to find suitable thread";
        Log(LogLevel::Error, result.errorMessage);
        CloseHandle(hProcess);
        return result;
    }
    
    result = InjectWithThread(hProcess, hThread, dllPath);
    
    CloseHandle(hThread);
    CloseHandle(hProcess);
    
    return result;
}

InjectionResult ThreadHijacker::InjectWithThread(
    HANDLE processHandle,
    HANDLE threadHandle,
    const std::wstring& dllPath
) {
    InjectionResult result;
    result.success = false;
    result.method = InjectionMethod::ThreadHijack;
    
     
    if (SuspendThread(threadHandle) == (DWORD)-1) {
        result.errorCode = GetLastError();
        result.errorMessage = L"Failed to suspend thread";
        Log(LogLevel::Error, result.errorMessage);
        return result;
    }
    
    Log(LogLevel::Debug, L"Thread suspended");
    
     
    CONTEXT originalCtx = { 0 };
    originalCtx.ContextFlags = CONTEXT_FULL;
    
    if (!GetThreadContext(threadHandle, &originalCtx)) {
        result.errorCode = GetLastError();
        result.errorMessage = L"Failed to get thread context";
        Log(LogLevel::Error, result.errorMessage);
        ResumeThread(threadHandle);
        return result;
    }
    
    Log(LogLevel::Debug, L"Original context saved");
    
     
    size_t pathSize = (dllPath.length() + 1) * sizeof(wchar_t);
    LPVOID remotePath = VirtualAllocEx(processHandle, nullptr, pathSize,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    
    if (!remotePath) {
        result.errorCode = GetLastError();
        result.errorMessage = L"Failed to allocate memory for DLL path";
        Log(LogLevel::Error, result.errorMessage);
        ResumeThread(threadHandle);
        return result;
    }
    
     
    SIZE_T written;
    if (!WriteProcessMemory(processHandle, remotePath, dllPath.c_str(), pathSize, &written)) {
        result.errorCode = GetLastError();
        result.errorMessage = L"Failed to write DLL path";
        Log(LogLevel::Error, result.errorMessage);
        VirtualFreeEx(processHandle, remotePath, 0, MEM_RELEASE);
        ResumeThread(threadHandle);
        return result;
    }
    
     
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    LPVOID pLoadLibrary = reinterpret_cast<LPVOID>(GetProcAddress(hKernel32, "LoadLibraryW"));
    
    if (!pLoadLibrary) {
        result.errorCode = GetLastError();
        result.errorMessage = L"Failed to get LoadLibraryW address";
        Log(LogLevel::Error, result.errorMessage);
        VirtualFreeEx(processHandle, remotePath, 0, MEM_RELEASE);
        ResumeThread(threadHandle);
        return result;
    }
    
     
#ifdef _WIN64
    bool is64Bit = true;
    LPVOID returnAddr = reinterpret_cast<LPVOID>(originalCtx.Rip);
#else
    bool is64Bit = false;
    LPVOID returnAddr = reinterpret_cast<LPVOID>(originalCtx.Eip);
#endif
    
     
    std::vector<BYTE> shellcode = GenerateShellcode(pLoadLibrary, remotePath, returnAddr, is64Bit);
    
     
    LPVOID remoteShellcode = VirtualAllocEx(processHandle, nullptr, shellcode.size(),
        MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    
    if (!remoteShellcode) {
        result.errorCode = GetLastError();
        result.errorMessage = L"Failed to allocate memory for shellcode";
        Log(LogLevel::Error, result.errorMessage);
        VirtualFreeEx(processHandle, remotePath, 0, MEM_RELEASE);
        ResumeThread(threadHandle);
        return result;
    }
    
     
    if (!WriteProcessMemory(processHandle, remoteShellcode, shellcode.data(), shellcode.size(), &written)) {
        result.errorCode = GetLastError();
        result.errorMessage = L"Failed to write shellcode";
        Log(LogLevel::Error, result.errorMessage);
        VirtualFreeEx(processHandle, remoteShellcode, 0, MEM_RELEASE);
        VirtualFreeEx(processHandle, remotePath, 0, MEM_RELEASE);
        ResumeThread(threadHandle);
        return result;
    }
    
    Log(LogLevel::Debug, L"Shellcode written at: 0x" + 
        utils::Utf8ToWide(std::to_string(reinterpret_cast<uintptr_t>(remoteShellcode))));
    
     
    CONTEXT newCtx = originalCtx;
    
#ifdef _WIN64
    newCtx.Rip = reinterpret_cast<DWORD64>(remoteShellcode);
#else
    newCtx.Eip = reinterpret_cast<DWORD>(remoteShellcode);
#endif
    
    if (!SetThreadContext(threadHandle, &newCtx)) {
        result.errorCode = GetLastError();
        result.errorMessage = L"Failed to set thread context";
        Log(LogLevel::Error, result.errorMessage);
        VirtualFreeEx(processHandle, remoteShellcode, 0, MEM_RELEASE);
        VirtualFreeEx(processHandle, remotePath, 0, MEM_RELEASE);
        ResumeThread(threadHandle);
        return result;
    }
    
    Log(LogLevel::Debug, L"Thread context modified");
    
     
    ResumeThread(threadHandle);
    
    Log(LogLevel::Info, L"Thread resumed, waiting for injection...");
    
     
     
    Sleep(100);
    
     
     
     
     
    
     
    result.success = true;
    result.moduleHandle = nullptr;   
    
    Log(LogLevel::Info, L"Thread hijack injection completed");
    
    return result;
}

HANDLE ThreadHijacker::FindSuitableThread(ProcessId processId) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return nullptr;
    }
    
    THREADENTRY32 te = { sizeof(te) };
    HANDLE hThread = nullptr;
    
    if (Thread32First(hSnapshot, &te)) {
        do {
            if (te.th32OwnerProcessID == processId) {
                 
                hThread = OpenThread(
                    THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | 
                    THREAD_SET_CONTEXT | THREAD_QUERY_INFORMATION,
                    FALSE, te.th32ThreadID);
                
                if (hThread) {
                    Log(LogLevel::Debug, L"Found suitable thread: " + std::to_wstring(te.th32ThreadID));
                    break;
                }
            }
        } while (Thread32Next(hSnapshot, &te));
    }
    
    CloseHandle(hSnapshot);
    return hThread;
}

std::vector<BYTE> ThreadHijacker::GenerateShellcode(
    LPVOID loadLibraryAddr,
    LPVOID dllPathAddr,
    LPVOID returnAddr,
    bool is64Bit
) {
    std::vector<BYTE> shellcode;
    
    if (is64Bit) {
         
         
         
         
         
        
        shellcode = {
             
            0x50,                                            
            0x51,                                            
            0x52,                                            
            0x41, 0x50,                                      
            0x41, 0x51,                                      
            0x41, 0x52,                                      
            0x41, 0x53,                                      
            
             
            0x48, 0x83, 0xEC, 0x28,
            
             
            0x48, 0xB9
        };
        
         
        auto pathAddr = reinterpret_cast<ULONGLONG>(dllPathAddr);
        for (int i = 0; i < 8; i++) {
            shellcode.push_back(static_cast<BYTE>(pathAddr >> (i * 8)));
        }
        
         
        shellcode.push_back(0x48);
        shellcode.push_back(0xB8);
        auto loadLibAddr = reinterpret_cast<ULONGLONG>(loadLibraryAddr);
        for (int i = 0; i < 8; i++) {
            shellcode.push_back(static_cast<BYTE>(loadLibAddr >> (i * 8)));
        }
        
         
        shellcode.push_back(0xFF);
        shellcode.push_back(0xD0);
        
         
        shellcode.push_back(0x48);
        shellcode.push_back(0x83);
        shellcode.push_back(0xC4);
        shellcode.push_back(0x28);
        
         
        shellcode.push_back(0x41);
        shellcode.push_back(0x5B);   
        shellcode.push_back(0x41);
        shellcode.push_back(0x5A);   
        shellcode.push_back(0x41);
        shellcode.push_back(0x59);   
        shellcode.push_back(0x41);
        shellcode.push_back(0x58);   
        shellcode.push_back(0x5A);   
        shellcode.push_back(0x59);   
        shellcode.push_back(0x58);   
        
         
        shellcode.push_back(0x48);
        shellcode.push_back(0xB8);   
        auto retAddr = reinterpret_cast<ULONGLONG>(returnAddr);
        for (int i = 0; i < 8; i++) {
            shellcode.push_back(static_cast<BYTE>(retAddr >> (i * 8)));
        }
        shellcode.push_back(0xFF);
        shellcode.push_back(0xE0);   
        
    } else {
         
        shellcode = {
             
            0x60,
            
             
            0x68
        };
        
        auto pathAddr = static_cast<DWORD>(reinterpret_cast<uintptr_t>(dllPathAddr));
        for (int i = 0; i < 4; i++) {
            shellcode.push_back(static_cast<BYTE>(pathAddr >> (i * 8)));
        }
        
         
        shellcode.push_back(0xB8);
        auto loadLibAddr = static_cast<DWORD>(reinterpret_cast<uintptr_t>(loadLibraryAddr));
        for (int i = 0; i < 4; i++) {
            shellcode.push_back(static_cast<BYTE>(loadLibAddr >> (i * 8)));
        }
        
         
        shellcode.push_back(0xFF);
        shellcode.push_back(0xD0);
        
         
        shellcode.push_back(0x61);
        
         
        shellcode.push_back(0x68);   
        auto retAddr = static_cast<DWORD>(reinterpret_cast<uintptr_t>(returnAddr));
        for (int i = 0; i < 4; i++) {
            shellcode.push_back(static_cast<BYTE>(retAddr >> (i * 8)));
        }
        shellcode.push_back(0xC3);   
    }
    
    return shellcode;
}

void ThreadHijacker::Log(LogLevel level, const std::wstring& message) {
    if (m_logCallback) {
        m_logCallback(level, L"[ThreadHijack] " + message);
    }
}

 
 
 

bool ContextHelper::SaveContext(HANDLE hThread, CONTEXT& ctx) {
    ctx.ContextFlags = CONTEXT_FULL;
    return GetThreadContext(hThread, &ctx) != FALSE;
}

bool ContextHelper::RestoreContext(HANDLE hThread, const CONTEXT& ctx) {
    return SetThreadContext(hThread, &ctx) != FALSE;
}

bool ContextHelper::SetInstructionPointer(HANDLE hThread, LPVOID address) {
    CONTEXT ctx = { 0 };
    ctx.ContextFlags = CONTEXT_CONTROL;
    
    if (!GetThreadContext(hThread, &ctx)) {
        return false;
    }
    
#ifdef _WIN64
    ctx.Rip = reinterpret_cast<DWORD64>(address);
#else
    ctx.Eip = reinterpret_cast<DWORD>(address);
#endif
    
    return SetThreadContext(hThread, &ctx) != FALSE;
}

LPVOID ContextHelper::GetInstructionPointer(HANDLE hThread) {
    CONTEXT ctx = { 0 };
    ctx.ContextFlags = CONTEXT_CONTROL;
    
    if (!GetThreadContext(hThread, &ctx)) {
        return nullptr;
    }
    
#ifdef _WIN64
    return reinterpret_cast<LPVOID>(ctx.Rip);
#else
    return reinterpret_cast<LPVOID>(ctx.Eip);
#endif
}

bool ContextHelper::IsThreadAlertable(HANDLE hThread) {
     
     
    return true;
}

}  
