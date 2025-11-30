#pragma once

#include "core/types.h"
#include <tlhelp32.h>
#include <psapi.h>
#include <mutex>
#include <atomic>

namespace xordll {

 
class ProcessManager {
public:
    ProcessManager();
    ~ProcessManager();
    
     
    ProcessManager(const ProcessManager&) = delete;
    ProcessManager& operator=(const ProcessManager&) = delete;
    
     
    bool RefreshProcessList();
    
     
    std::vector<ProcessInfo> GetProcessList() const;
    
     
    std::vector<ProcessInfo> FilterByName(const std::wstring& filter) const;
    
     
    std::optional<ProcessInfo> FindByPid(ProcessId pid) const;
    
     
    static bool IsProcess64Bit(ProcessId pid);
    
     
    static bool CanAccessProcess(ProcessId pid, DWORD desiredAccess = PROCESS_ALL_ACCESS);
    
     
    static std::wstring GetProcessPath(ProcessId pid);
    
     
    static HICON GetProcessIcon(const std::wstring& path);
    
     
    static bool IsSystemProcess(ProcessId pid);
    
     
    static HANDLE OpenProcessHandle(ProcessId pid, DWORD desiredAccess = PROCESS_ALL_ACCESS);
    
     
    size_t GetProcessCount() const;
    
     
    static bool IsRunningAsAdmin();
    
     
    static bool EnableDebugPrivilege();

private:
    std::vector<ProcessInfo> m_processes;
    mutable std::mutex m_mutex;
    std::atomic<bool> m_isRefreshing;
    
     
    ProcessInfo CreateProcessInfo(const PROCESSENTRY32W& entry);
};

}  
