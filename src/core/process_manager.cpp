 

#include "core/process_manager.h"
#include "utils/string_utils.h"
#include "utils/logger.h"
#include <shellapi.h>
#include <algorithm>

namespace xordll {

ProcessManager::ProcessManager()
    : m_isRefreshing(false)
{
}

ProcessManager::~ProcessManager()
{
     
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& proc : m_processes) {
        if (proc.icon) {
            DestroyIcon(proc.icon);
            proc.icon = nullptr;
        }
    }
}

bool ProcessManager::RefreshProcessList()
{
    if (m_isRefreshing.exchange(true)) {
        return false;  
    }
    
    std::vector<ProcessInfo> newProcesses;
    
     
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        LOG_WIN_ERROR(L"CreateToolhelp32Snapshot");
        m_isRefreshing = false;
        return false;
    }
    
    PROCESSENTRY32W pe32 = { 0 };
    pe32.dwSize = sizeof(pe32);
    
    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
             
            if (pe32.th32ProcessID == 0) continue;
            
            ProcessInfo info = CreateProcessInfo(pe32);
            newProcesses.push_back(std::move(info));
            
        } while (Process32NextW(hSnapshot, &pe32));
    }
    
    CloseHandle(hSnapshot);
    
     
    std::sort(newProcesses.begin(), newProcesses.end(),
        [](const ProcessInfo& a, const ProcessInfo& b) {
            return utils::ToLower(a.name) < utils::ToLower(b.name);
        });
    
     
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
         
        for (auto& proc : m_processes) {
            if (proc.icon) {
                DestroyIcon(proc.icon);
            }
        }
        
        m_processes = std::move(newProcesses);
    }
    
    m_isRefreshing = false;
    LOG_DEBUG(L"Process list refreshed: " + std::to_wstring(m_processes.size()) + L" processes");
    return true;
}

std::vector<ProcessInfo> ProcessManager::GetProcessList() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_processes;
}

std::vector<ProcessInfo> ProcessManager::FilterByName(const std::wstring& filter) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (filter.empty()) {
        return m_processes;
    }
    
    std::vector<ProcessInfo> filtered;
    std::wstring lowerFilter = utils::ToLower(filter);
    
    for (const auto& proc : m_processes) {
        if (utils::ContainsIgnoreCase(proc.name, filter)) {
            filtered.push_back(proc);
        }
    }
    
    return filtered;
}

std::optional<ProcessInfo> ProcessManager::FindByPid(ProcessId pid) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (const auto& proc : m_processes) {
        if (proc.pid == pid) {
            return proc;
        }
    }
    
    return std::nullopt;
}

bool ProcessManager::IsProcess64Bit(ProcessId pid)
{
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProcess) {
        return false;
    }
    
    BOOL isWow64 = FALSE;
    BOOL result = IsWow64Process(hProcess, &isWow64);
    CloseHandle(hProcess);
    
    if (!result) {
        return false;
    }
    
     
     
     
    #ifdef _WIN64
        return !isWow64;
    #else
        return false;  
    #endif
}

bool ProcessManager::CanAccessProcess(ProcessId pid, DWORD desiredAccess)
{
    HANDLE hProcess = OpenProcess(desiredAccess, FALSE, pid);
    if (hProcess) {
        CloseHandle(hProcess);
        return true;
    }
    return false;
}

std::wstring ProcessManager::GetProcessPath(ProcessId pid)
{
    std::wstring path;
    
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (hProcess) {
        wchar_t buffer[MAX_PATH] = { 0 };
        DWORD size = MAX_PATH;
        
        if (QueryFullProcessImageNameW(hProcess, 0, buffer, &size)) {
            path = buffer;
        }
        
        CloseHandle(hProcess);
    }
    
    return path;
}

HICON ProcessManager::GetProcessIcon(const std::wstring& path)
{
    if (path.empty()) {
        return nullptr;
    }
    
    SHFILEINFOW sfi = { 0 };
    if (SHGetFileInfoW(path.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_SMALLICON)) {
        return sfi.hIcon;
    }
    
    return nullptr;
}

bool ProcessManager::IsSystemProcess(ProcessId pid)
{
     
    if (pid == 0 || pid == 4) {
        return true;
    }
    
     
    std::wstring path = GetProcessPath(pid);
    if (path.empty()) {
        return false;
    }
    
    wchar_t winDir[MAX_PATH];
    GetWindowsDirectoryW(winDir, MAX_PATH);
    
    std::wstring lowerPath = utils::ToLower(path);
    std::wstring lowerWinDir = utils::ToLower(winDir);
    
    return lowerPath.find(lowerWinDir) == 0;
}

HANDLE ProcessManager::OpenProcessHandle(ProcessId pid, DWORD desiredAccess)
{
    return OpenProcess(desiredAccess, FALSE, pid);
}

size_t ProcessManager::GetProcessCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_processes.size();
}

bool ProcessManager::IsRunningAsAdmin()
{
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;
    
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2,
        SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0, &adminGroup)) {
        
        CheckTokenMembership(nullptr, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    
    return isAdmin != FALSE;
}

bool ProcessManager::EnableDebugPrivilege()
{
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        return false;
    }
    
    LUID luid;
    if (!LookupPrivilegeValueW(nullptr, SE_DEBUG_NAME, &luid)) {
        CloseHandle(hToken);
        return false;
    }
    
    TOKEN_PRIVILEGES tp = { 0 };
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    
    BOOL result = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr);
    DWORD error = GetLastError();
    
    CloseHandle(hToken);
    
    return result && error == ERROR_SUCCESS;
}

ProcessInfo ProcessManager::CreateProcessInfo(const PROCESSENTRY32W& entry)
{
    ProcessInfo info;
    info.pid = entry.th32ProcessID;
    info.name = entry.szExeFile;
    info.path = GetProcessPath(entry.th32ProcessID);
    info.is64Bit = IsProcess64Bit(entry.th32ProcessID);
    info.icon = GetProcessIcon(info.path);
    
    return info;
}

}  
