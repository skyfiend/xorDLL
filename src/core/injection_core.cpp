 

#include "core/injection_core.h"
#include "core/process_manager.h"
#include "core/manual_map.h"
#include "core/thread_hijack.h"
#include "utils/string_utils.h"
#include "utils/file_utils.h"
#include "utils/logger.h"

namespace xordll {

 
 
 

InjectionCore::InjectionCore()
    : m_logCallback(nullptr)
{
}

InjectionCore::~InjectionCore()
{
}

InjectionResult InjectionCore::Inject(
    ProcessId pid,
    const std::wstring& dllPath,
    InjectionMethod method,
    ProgressCallback progressCallback
) {
    Log(LogLevel::Info, L"Starting injection into PID " + std::to_wstring(pid));
    Log(LogLevel::Info, L"DLL: " + dllPath);
    Log(LogLevel::Info, L"Method: " + GetMethodName(method));
    
     
    DllInfo dllInfo;
    if (!ValidateDll(dllPath, dllInfo)) {
        return InjectionResult::Failure(ERROR_FILE_NOT_FOUND, L"Invalid or missing DLL file");
    }
    
     
    bool targetIs64Bit = ProcessManager::IsProcess64Bit(pid);
    if (targetIs64Bit != dllInfo.is64Bit) {
        std::wstring msg = L"Architecture mismatch: Process is " + 
            std::wstring(targetIs64Bit ? L"64-bit" : L"32-bit") +
            L", DLL is " + std::wstring(dllInfo.is64Bit ? L"64-bit" : L"32-bit");
        Log(LogLevel::Error, msg);
        return InjectionResult::Failure(ERROR_BAD_EXE_FORMAT, msg);
    }
    
     
    HANDLE hProcess = ProcessManager::OpenProcessHandle(pid, 
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | 
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ);
    
    if (!hProcess) {
        DWORD error = GetLastError();
        Log(LogLevel::Error, L"Failed to open process: " + utils::FormatWindowsError(error));
        return InjectionResult::Failure(error, L"Failed to open target process");
    }
    
     
    auto injectionMethod = CreateMethod(method);
    if (!injectionMethod) {
        CloseHandle(hProcess);
        return InjectionResult::Failure(ERROR_NOT_SUPPORTED, L"Unsupported injection method");
    }
    
     
    if (progressCallback) {
        progressCallback(10, L"Preparing injection...");
    }
    
    InjectionResult result = injectionMethod->Inject(hProcess, dllPath, progressCallback);
    
    CloseHandle(hProcess);
    
    if (result.success) {
        Log(LogLevel::Info, L"Injection successful!");
    } else {
        Log(LogLevel::Error, L"Injection failed: " + result.errorMessage);
    }
    
    return result;
}

InjectionResult InjectionCore::Eject(
    ProcessId pid,
    ModuleHandle moduleHandle,
    InjectionMethod method
) {
    Log(LogLevel::Info, L"Starting ejection from PID " + std::to_wstring(pid));
    
    HANDLE hProcess = ProcessManager::OpenProcessHandle(pid,
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ);
    
    if (!hProcess) {
        DWORD error = GetLastError();
        return InjectionResult::Failure(error, L"Failed to open target process");
    }
    
    auto injectionMethod = CreateMethod(method);
    if (!injectionMethod) {
        CloseHandle(hProcess);
        return InjectionResult::Failure(ERROR_NOT_SUPPORTED, L"Unsupported injection method");
    }
    
    InjectionResult result = injectionMethod->Eject(hProcess, moduleHandle);
    
    CloseHandle(hProcess);
    
    if (result.success) {
        Log(LogLevel::Info, L"Ejection successful!");
    } else {
        Log(LogLevel::Error, L"Ejection failed: " + result.errorMessage);
    }
    
    return result;
}

std::vector<InjectionMethod> InjectionCore::GetAvailableMethods() const
{
    return {
        InjectionMethod::CreateRemoteThread,
        InjectionMethod::NtCreateThreadEx,
        InjectionMethod::QueueUserAPC,
        InjectionMethod::ManualMap,
        InjectionMethod::ThreadHijack
    };
}

std::wstring InjectionCore::GetMethodName(InjectionMethod method)
{
    switch (method) {
        case InjectionMethod::CreateRemoteThread:
            return L"CreateRemoteThread";
        case InjectionMethod::NtCreateThreadEx:
            return L"NtCreateThreadEx";
        case InjectionMethod::QueueUserAPC:
            return L"QueueUserAPC";
        case InjectionMethod::SetWindowsHookEx:
            return L"SetWindowsHookEx";
        case InjectionMethod::ManualMap:
            return L"Manual Map";
        case InjectionMethod::ThreadHijack:
            return L"Thread Hijack";
        default:
            return L"Unknown";
    }
}

std::wstring InjectionCore::GetMethodDescription(InjectionMethod method)
{
    switch (method) {
        case InjectionMethod::CreateRemoteThread:
            return L"Classic injection method using CreateRemoteThread. Most compatible but easily detected.";
        case InjectionMethod::NtCreateThreadEx:
            return L"Uses native NT API for thread creation. Works on modern Windows versions.";
        case InjectionMethod::QueueUserAPC:
            return L"Asynchronous injection using APC queue. Requires alertable thread in target.";
        case InjectionMethod::SetWindowsHookEx:
            return L"Uses Windows hooks mechanism. Good for GUI applications.";
        case InjectionMethod::ManualMap:
            return L"Manually maps DLL without using LoadLibrary. Stealthiest method.";
        case InjectionMethod::ThreadHijack:
            return L"Hijacks existing thread to execute LoadLibrary. More stealthy than creating threads.";
        default:
            return L"Unknown method";
    }
}

bool InjectionCore::MethodRequiresAdmin(InjectionMethod method)
{
     
    return true;
}

bool InjectionCore::ValidateDll(const std::wstring& dllPath, DllInfo& info)
{
    if (!utils::FileExists(dllPath)) {
        return false;
    }
    
    info.path = dllPath;
    info.name = utils::GetFileName(dllPath);
    info.fileSize = utils::GetFileSize(dllPath);
    
     
    std::vector<BYTE> data;
    if (!utils::ReadFileToMemory(dllPath, data) || data.size() < sizeof(IMAGE_DOS_HEADER)) {
        return false;
    }
    
    auto dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(data.data());
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        return false;
    }
    
    if (data.size() < static_cast<size_t>(dosHeader->e_lfanew) + sizeof(IMAGE_NT_HEADERS)) {
        return false;
    }
    
    auto ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(data.data() + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
        return false;
    }
    
     
    if (!(ntHeaders->FileHeader.Characteristics & IMAGE_FILE_DLL)) {
        return false;
    }
    
     
    info.is64Bit = (ntHeaders->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64);
    
    return true;
}

void InjectionCore::SetLogCallback(LogCallback callback)
{
    m_logCallback = callback;
}

std::unique_ptr<IInjectionMethod> InjectionCore::CreateMethod(InjectionMethod method)
{
    switch (method) {
        case InjectionMethod::CreateRemoteThread:
            return std::make_unique<CreateRemoteThreadInjection>();
        case InjectionMethod::NtCreateThreadEx:
            return std::make_unique<NtCreateThreadExInjection>();
        case InjectionMethod::QueueUserAPC:
            return std::make_unique<QueueUserAPCInjection>();
        case InjectionMethod::ManualMap:
            return std::make_unique<ManualMapInjection>();
        case InjectionMethod::ThreadHijack:
            return std::make_unique<ThreadHijackInjection>();
        default:
            return nullptr;
    }
}

void InjectionCore::Log(LogLevel level, const std::wstring& message)
{
    if (m_logCallback) {
        m_logCallback(level, message);
    }
    
     
    switch (level) {
        case LogLevel::Debug: LOG_DEBUG(message); break;
        case LogLevel::Info: LOG_INFO(message); break;
        case LogLevel::Warning: LOG_WARNING(message); break;
        case LogLevel::Error: LOG_ERROR(message); break;
    }
}

 
 
 

std::wstring CreateRemoteThreadInjection::GetName() const
{
    return L"CreateRemoteThread";
}

std::wstring CreateRemoteThreadInjection::GetDescription() const
{
    return L"Classic injection method using CreateRemoteThread and LoadLibrary";
}

bool CreateRemoteThreadInjection::RequiresAdmin() const
{
    return true;
}

bool CreateRemoteThreadInjection::Supports32Bit() const
{
    return true;
}

bool CreateRemoteThreadInjection::Supports64Bit() const
{
    return true;
}

InjectionResult CreateRemoteThreadInjection::Inject(
    HANDLE processHandle,
    const std::wstring& dllPath,
    ProgressCallback progressCallback
) {
    if (progressCallback) progressCallback(20, L"Allocating memory in target process...");
    
     
    size_t pathSize = (dllPath.length() + 1) * sizeof(wchar_t);
    
     
    LPVOID remoteMem = VirtualAllocEx(
        processHandle,
        nullptr,
        pathSize,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );
    
    if (!remoteMem) {
        DWORD error = GetLastError();
        return InjectionResult::Failure(error, 
            L"VirtualAllocEx failed: " + utils::FormatWindowsError(error));
    }
    
    if (progressCallback) progressCallback(40, L"Writing DLL path to target process...");
    
     
    SIZE_T bytesWritten;
    if (!WriteProcessMemory(processHandle, remoteMem, dllPath.c_str(), pathSize, &bytesWritten)) {
        DWORD error = GetLastError();
        VirtualFreeEx(processHandle, remoteMem, 0, MEM_RELEASE);
        return InjectionResult::Failure(error,
            L"WriteProcessMemory failed: " + utils::FormatWindowsError(error));
    }
    
    if (progressCallback) progressCallback(60, L"Getting LoadLibraryW address...");
    
     
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!hKernel32) {
        VirtualFreeEx(processHandle, remoteMem, 0, MEM_RELEASE);
        return InjectionResult::Failure(ERROR_MOD_NOT_FOUND, L"Failed to get kernel32.dll handle");
    }
    
    LPTHREAD_START_ROUTINE loadLibraryAddr = 
        reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(hKernel32, "LoadLibraryW"));
    
    if (!loadLibraryAddr) {
        VirtualFreeEx(processHandle, remoteMem, 0, MEM_RELEASE);
        return InjectionResult::Failure(ERROR_PROC_NOT_FOUND, L"Failed to get LoadLibraryW address");
    }
    
    if (progressCallback) progressCallback(80, L"Creating remote thread...");
    
     
    HANDLE hThread = CreateRemoteThread(
        processHandle,
        nullptr,
        0,
        loadLibraryAddr,
        remoteMem,
        0,
        nullptr
    );
    
    if (!hThread) {
        DWORD error = GetLastError();
        VirtualFreeEx(processHandle, remoteMem, 0, MEM_RELEASE);
        return InjectionResult::Failure(error,
            L"CreateRemoteThread failed: " + utils::FormatWindowsError(error));
    }
    
     
    WaitForSingleObject(hThread, INFINITE);
    
     
    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);
    
    CloseHandle(hThread);
    VirtualFreeEx(processHandle, remoteMem, 0, MEM_RELEASE);
    
    if (progressCallback) progressCallback(100, L"Injection complete!");
    
    if (exitCode == 0) {
        return InjectionResult::Failure(ERROR_MOD_NOT_FOUND, L"LoadLibraryW returned NULL");
    }
    
    return InjectionResult::Success(reinterpret_cast<ModuleHandle>(static_cast<ULONG_PTR>(exitCode)));
}

InjectionResult CreateRemoteThreadInjection::Eject(
    HANDLE processHandle,
    ModuleHandle moduleHandle
) {
     
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!hKernel32) {
        return InjectionResult::Failure(ERROR_MOD_NOT_FOUND, L"Failed to get kernel32.dll handle");
    }
    
    LPTHREAD_START_ROUTINE freeLibraryAddr =
        reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(hKernel32, "FreeLibrary"));
    
    if (!freeLibraryAddr) {
        return InjectionResult::Failure(ERROR_PROC_NOT_FOUND, L"Failed to get FreeLibrary address");
    }
    
     
    HANDLE hThread = CreateRemoteThread(
        processHandle,
        nullptr,
        0,
        freeLibraryAddr,
        moduleHandle,
        0,
        nullptr
    );
    
    if (!hThread) {
        DWORD error = GetLastError();
        return InjectionResult::Failure(error,
            L"CreateRemoteThread failed: " + utils::FormatWindowsError(error));
    }
    
    WaitForSingleObject(hThread, INFINITE);
    
    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);
    CloseHandle(hThread);
    
    if (exitCode == 0) {
        return InjectionResult::Failure(ERROR_MOD_NOT_FOUND, L"FreeLibrary returned FALSE");
    }
    
    return InjectionResult::Success();
}

 
 
 

 
typedef NTSTATUS(NTAPI* pNtCreateThreadEx)(
    PHANDLE ThreadHandle,
    ACCESS_MASK DesiredAccess,
    PVOID ObjectAttributes,
    HANDLE ProcessHandle,
    PVOID StartRoutine,
    PVOID Argument,
    ULONG CreateFlags,
    SIZE_T ZeroBits,
    SIZE_T StackSize,
    SIZE_T MaximumStackSize,
    PVOID AttributeList
);

std::wstring NtCreateThreadExInjection::GetName() const
{
    return L"NtCreateThreadEx";
}

std::wstring NtCreateThreadExInjection::GetDescription() const
{
    return L"Uses native NT API NtCreateThreadEx for thread creation";
}

bool NtCreateThreadExInjection::RequiresAdmin() const
{
    return true;
}

bool NtCreateThreadExInjection::Supports32Bit() const
{
    return true;
}

bool NtCreateThreadExInjection::Supports64Bit() const
{
    return true;
}

InjectionResult NtCreateThreadExInjection::Inject(
    HANDLE processHandle,
    const std::wstring& dllPath,
    ProgressCallback progressCallback
) {
    if (progressCallback) progressCallback(10, L"Getting NtCreateThreadEx address...");
    
     
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtdll) {
        return InjectionResult::Failure(ERROR_MOD_NOT_FOUND, L"Failed to get ntdll.dll handle");
    }
    
    auto NtCreateThreadEx = reinterpret_cast<pNtCreateThreadEx>(
        GetProcAddress(hNtdll, "NtCreateThreadEx"));
    
    if (!NtCreateThreadEx) {
        return InjectionResult::Failure(ERROR_PROC_NOT_FOUND, L"Failed to get NtCreateThreadEx address");
    }
    
    if (progressCallback) progressCallback(20, L"Allocating memory in target process...");
    
     
    size_t pathSize = (dllPath.length() + 1) * sizeof(wchar_t);
    LPVOID remoteMem = VirtualAllocEx(processHandle, nullptr, pathSize,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    
    if (!remoteMem) {
        DWORD error = GetLastError();
        return InjectionResult::Failure(error,
            L"VirtualAllocEx failed: " + utils::FormatWindowsError(error));
    }
    
    if (progressCallback) progressCallback(40, L"Writing DLL path...");
    
     
    SIZE_T bytesWritten;
    if (!WriteProcessMemory(processHandle, remoteMem, dllPath.c_str(), pathSize, &bytesWritten)) {
        DWORD error = GetLastError();
        VirtualFreeEx(processHandle, remoteMem, 0, MEM_RELEASE);
        return InjectionResult::Failure(error,
            L"WriteProcessMemory failed: " + utils::FormatWindowsError(error));
    }
    
    if (progressCallback) progressCallback(60, L"Getting LoadLibraryW address...");
    
     
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    LPVOID loadLibraryAddr = reinterpret_cast<LPVOID>(GetProcAddress(hKernel32, "LoadLibraryW"));
    
    if (!loadLibraryAddr) {
        VirtualFreeEx(processHandle, remoteMem, 0, MEM_RELEASE);
        return InjectionResult::Failure(ERROR_PROC_NOT_FOUND, L"Failed to get LoadLibraryW address");
    }
    
    if (progressCallback) progressCallback(80, L"Creating thread with NtCreateThreadEx...");
    
     
    HANDLE hThread = nullptr;
    NTSTATUS status = NtCreateThreadEx(
        &hThread,
        THREAD_ALL_ACCESS,
        nullptr,
        processHandle,
        loadLibraryAddr,
        remoteMem,
        0,
        0,
        0,
        0,
        nullptr
    );
    
    if (status != 0 || !hThread) {
        VirtualFreeEx(processHandle, remoteMem, 0, MEM_RELEASE);
        return InjectionResult::Failure(static_cast<DWORD>(status),
            L"NtCreateThreadEx failed with NTSTATUS: " + std::to_wstring(status));
    }
    
    WaitForSingleObject(hThread, INFINITE);
    
    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);
    
    CloseHandle(hThread);
    VirtualFreeEx(processHandle, remoteMem, 0, MEM_RELEASE);
    
    if (progressCallback) progressCallback(100, L"Injection complete!");
    
    if (exitCode == 0) {
        return InjectionResult::Failure(ERROR_MOD_NOT_FOUND, L"LoadLibraryW returned NULL");
    }
    
    return InjectionResult::Success(reinterpret_cast<ModuleHandle>(static_cast<ULONG_PTR>(exitCode)));
}

InjectionResult NtCreateThreadExInjection::Eject(
    HANDLE processHandle,
    ModuleHandle moduleHandle
) {
     
    CreateRemoteThreadInjection crt;
    return crt.Eject(processHandle, moduleHandle);
}

 
 
 

std::wstring QueueUserAPCInjection::GetName() const
{
    return L"QueueUserAPC";
}

std::wstring QueueUserAPCInjection::GetDescription() const
{
    return L"Asynchronous injection using APC queue";
}

bool QueueUserAPCInjection::RequiresAdmin() const
{
    return true;
}

bool QueueUserAPCInjection::Supports32Bit() const
{
    return true;
}

bool QueueUserAPCInjection::Supports64Bit() const
{
    return true;
}

InjectionResult QueueUserAPCInjection::Inject(
    HANDLE processHandle,
    const std::wstring& dllPath,
    ProgressCallback progressCallback
) {
    if (progressCallback) progressCallback(10, L"Allocating memory...");
    
     
    size_t pathSize = (dllPath.length() + 1) * sizeof(wchar_t);
    LPVOID remoteMem = VirtualAllocEx(processHandle, nullptr, pathSize,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    
    if (!remoteMem) {
        DWORD error = GetLastError();
        return InjectionResult::Failure(error,
            L"VirtualAllocEx failed: " + utils::FormatWindowsError(error));
    }
    
    if (progressCallback) progressCallback(30, L"Writing DLL path...");
    
     
    SIZE_T bytesWritten;
    if (!WriteProcessMemory(processHandle, remoteMem, dllPath.c_str(), pathSize, &bytesWritten)) {
        DWORD error = GetLastError();
        VirtualFreeEx(processHandle, remoteMem, 0, MEM_RELEASE);
        return InjectionResult::Failure(error,
            L"WriteProcessMemory failed: " + utils::FormatWindowsError(error));
    }
    
    if (progressCallback) progressCallback(50, L"Getting LoadLibraryW address...");
    
     
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    PAPCFUNC loadLibraryAddr = reinterpret_cast<PAPCFUNC>(GetProcAddress(hKernel32, "LoadLibraryW"));
    
    if (!loadLibraryAddr) {
        VirtualFreeEx(processHandle, remoteMem, 0, MEM_RELEASE);
        return InjectionResult::Failure(ERROR_PROC_NOT_FOUND, L"Failed to get LoadLibraryW address");
    }
    
    if (progressCallback) progressCallback(70, L"Enumerating threads...");
    
     
    DWORD processId = GetProcessId(processHandle);
    
     
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        VirtualFreeEx(processHandle, remoteMem, 0, MEM_RELEASE);
        return InjectionResult::Failure(GetLastError(), L"Failed to create thread snapshot");
    }
    
    THREADENTRY32 te32 = { 0 };
    te32.dwSize = sizeof(te32);
    
    int apcQueued = 0;
    
    if (Thread32First(hSnapshot, &te32)) {
        do {
            if (te32.th32OwnerProcessID == processId) {
                HANDLE hThread = OpenThread(THREAD_SET_CONTEXT, FALSE, te32.th32ThreadID);
                if (hThread) {
                    if (QueueUserAPC(loadLibraryAddr, hThread, reinterpret_cast<ULONG_PTR>(remoteMem))) {
                        apcQueued++;
                    }
                    CloseHandle(hThread);
                }
            }
        } while (Thread32Next(hSnapshot, &te32));
    }
    
    CloseHandle(hSnapshot);
    
    if (progressCallback) progressCallback(100, L"APC queued to " + std::to_wstring(apcQueued) + L" threads");
    
    if (apcQueued == 0) {
        VirtualFreeEx(processHandle, remoteMem, 0, MEM_RELEASE);
        return InjectionResult::Failure(ERROR_NO_MORE_ITEMS, 
            L"Failed to queue APC to any thread. Target may not have alertable threads.");
    }
    
     
     
    
    return InjectionResult::Success();
}

InjectionResult QueueUserAPCInjection::Eject(
    HANDLE processHandle,
    ModuleHandle moduleHandle
) {
     
    CreateRemoteThreadInjection crt;
    return crt.Eject(processHandle, moduleHandle);
}

 
 
 

std::wstring ManualMapInjection::GetName() const {
    return L"Manual Map";
}

std::wstring ManualMapInjection::GetDescription() const {
    return L"Maps DLL manually without LoadLibrary. Stealthiest method, harder to detect.";
}

bool ManualMapInjection::RequiresAdmin() const {
    return true;
}

bool ManualMapInjection::Supports32Bit() const {
    return true;
}

bool ManualMapInjection::Supports64Bit() const {
    return true;
}

InjectionResult ManualMapInjection::Inject(
    HANDLE processHandle,
    const std::wstring& dllPath,
    ProgressCallback progressCallback
) {
    if (progressCallback) progressCallback(10, L"Initializing manual mapper...");
    
    ManualMapper mapper;
    mapper.SetLogCallback([](LogLevel level, const std::wstring& msg) {
        switch (level) {
            case LogLevel::Debug: LOG_DEBUG(msg); break;
            case LogLevel::Info: LOG_INFO(msg); break;
            case LogLevel::Warning: LOG_WARNING(msg); break;
            case LogLevel::Error: LOG_ERROR(msg); break;
        }
    });
    
    if (progressCallback) progressCallback(30, L"Parsing PE headers...");
    
    ManualMapResult mapResult = mapper.Map(processHandle, dllPath, ManualMapFlags::Default);
    
    if (!mapResult.success) {
        return InjectionResult::Failure(mapResult.errorCode, mapResult.errorMessage);
    }
    
    if (progressCallback) progressCallback(100, L"Manual mapping complete!");
    
    InjectionResult result = InjectionResult::Success(
        reinterpret_cast<ModuleHandle>(mapResult.baseAddress));
    result.method = InjectionMethod::ManualMap;
    result.baseAddress = mapResult.baseAddress;
    result.mappedSize = mapResult.mappedSize;
    
    return result;
}

InjectionResult ManualMapInjection::Eject(
    HANDLE processHandle,
    ModuleHandle moduleHandle
) {
     
     
    ManualMapper mapper;
    
    if (mapper.Unmap(processHandle, reinterpret_cast<LPVOID>(moduleHandle))) {
        return InjectionResult::Success();
    }
    
    return InjectionResult::Failure(GetLastError(), L"Failed to unmap manually mapped DLL");
}

 
 
 

std::wstring ThreadHijackInjection::GetName() const {
    return L"Thread Hijack";
}

std::wstring ThreadHijackInjection::GetDescription() const {
    return L"Hijacks existing thread to load DLL. More stealthy than creating new threads.";
}

bool ThreadHijackInjection::RequiresAdmin() const {
    return true;
}

bool ThreadHijackInjection::Supports32Bit() const {
    return true;
}

bool ThreadHijackInjection::Supports64Bit() const {
    return true;
}

InjectionResult ThreadHijackInjection::Inject(
    HANDLE processHandle,
    const std::wstring& dllPath,
    ProgressCallback progressCallback
) {
    if (progressCallback) progressCallback(10, L"Finding suitable thread...");
    
     
    DWORD processId = GetProcessId(processHandle);
    if (processId == 0) {
        return InjectionResult::Failure(GetLastError(), L"Failed to get process ID");
    }
    
    ThreadHijacker hijacker;
    hijacker.SetLogCallback([](LogLevel level, const std::wstring& msg) {
        switch (level) {
            case LogLevel::Debug: LOG_DEBUG(msg); break;
            case LogLevel::Info: LOG_INFO(msg); break;
            case LogLevel::Warning: LOG_WARNING(msg); break;
            case LogLevel::Error: LOG_ERROR(msg); break;
        }
    });
    
    if (progressCallback) progressCallback(30, L"Hijacking thread...");
    
    InjectionResult result = hijacker.Inject(processId, dllPath);
    result.method = InjectionMethod::ThreadHijack;
    
    if (progressCallback) {
        if (result.success) {
            progressCallback(100, L"Thread hijack injection complete!");
        } else {
            progressCallback(100, L"Thread hijack failed: " + result.errorMessage);
        }
    }
    
    return result;
}

InjectionResult ThreadHijackInjection::Eject(
    HANDLE processHandle,
    ModuleHandle moduleHandle
) {
     
    CreateRemoteThreadInjection crt;
    return crt.Eject(processHandle, moduleHandle);
}

}  
