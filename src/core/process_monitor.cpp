 

#include "core/process_monitor.h"
#include "core/injection_core.h"
#include "core/process_manager.h"
#include "utils/string_utils.h"
#include <tlhelp32.h>
#include <algorithm>

namespace xordll {

 
 
 

ProcessMonitor::ProcessMonitor()
    : m_running(false)
    , m_pollingInterval(1000)
{
}

ProcessMonitor::~ProcessMonitor() {
    Stop();
}

bool ProcessMonitor::Start() {
    if (m_running) {
        return true;
    }
    
    m_running = true;
    m_thread = std::thread(&ProcessMonitor::MonitorThread, this);
    
    return true;
}

void ProcessMonitor::Stop() {
    if (!m_running) {
        return;
    }
    
    m_running = false;
    
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void ProcessMonitor::WatchProcess(const std::wstring& processName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_watchList.insert(utils::ToLower(processName));
}

void ProcessMonitor::UnwatchProcess(const std::wstring& processName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_watchList.erase(utils::ToLower(processName));
}

void ProcessMonitor::ClearWatchList() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_watchList.clear();
}

std::vector<std::wstring> ProcessMonitor::GetWatchList() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return std::vector<std::wstring>(m_watchList.begin(), m_watchList.end());
}

void ProcessMonitor::MonitorThread() {
     
    CheckForNewProcesses();
    
    while (m_running) {
        Sleep(m_pollingInterval);
        
        if (!m_running) break;
        
        CheckForNewProcesses();
        CheckForTerminatedProcesses();
    }
}

void ProcessMonitor::CheckForNewProcesses() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return;
    }
    
    PROCESSENTRY32W pe = { sizeof(pe) };
    
    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            ProcessId pid = pe.th32ProcessID;
            std::wstring name = pe.szExeFile;
            
             
            std::lock_guard<std::mutex> lock(m_mutex);
            
            if (m_knownProcesses.find(pid) == m_knownProcesses.end()) {
                 
                ProcessInfo info;
                info.pid = pid;
                info.name = name;
                info.path = L"";   
                
                m_knownProcesses[pid] = info;
                
                 
                if (IsWatched(name)) {
                    if (m_callback) {
                        m_callback(ProcessEvent::Started, info);
                    }
                }
            }
        } while (Process32NextW(hSnapshot, &pe));
    }
    
    CloseHandle(hSnapshot);
}

void ProcessMonitor::CheckForTerminatedProcesses() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<ProcessId> terminated;
    
    for (const auto& pair : m_knownProcesses) {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pair.first);
        
        if (!hProcess) {
             
            terminated.push_back(pair.first);
            
            if (IsWatched(pair.second.name)) {
                if (m_callback) {
                    m_callback(ProcessEvent::Terminated, pair.second);
                }
            }
        } else {
             
            DWORD exitCode;
            if (GetExitCodeProcess(hProcess, &exitCode) && exitCode != STILL_ACTIVE) {
                terminated.push_back(pair.first);
                
                if (IsWatched(pair.second.name)) {
                    if (m_callback) {
                        m_callback(ProcessEvent::Terminated, pair.second);
                    }
                }
            }
            CloseHandle(hProcess);
        }
    }
    
     
    for (ProcessId pid : terminated) {
        m_knownProcesses.erase(pid);
    }
}

bool ProcessMonitor::IsWatched(const std::wstring& processName) const {
    std::wstring lowerName = utils::ToLower(processName);
    return m_watchList.find(lowerName) != m_watchList.end();
}

 
 
 

AutoInjector::AutoInjector()
    : m_monitor(std::make_unique<ProcessMonitor>())
{
    m_stats = { 0, 0, 0 };
    
    m_monitor->SetCallback([this](ProcessEvent event, const ProcessInfo& process) {
        OnProcessEvent(event, process);
    });
}

AutoInjector::~AutoInjector() {
    Stop();
}

bool AutoInjector::Start() {
     
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& rule : m_rules) {
            m_monitor->WatchProcess(rule.processName);
        }
    }
    
    return m_monitor->Start();
}

void AutoInjector::Stop() {
    m_monitor->Stop();
}

bool AutoInjector::IsRunning() const {
    return m_monitor->IsRunning();
}

void AutoInjector::AddRule(
    const std::wstring& processName,
    const std::wstring& dllPath,
    InjectionMethod method,
    DWORD delay
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    InjectionRule rule;
    rule.processName = utils::ToLower(processName);
    rule.dllPath = dllPath;
    rule.method = method;
    rule.delay = delay;
    
    m_rules.push_back(rule);
    
    if (m_monitor->IsRunning()) {
        m_monitor->WatchProcess(processName);
    }
    
    Log(LogLevel::Info, L"Added auto-inject rule for: " + processName);
}

void AutoInjector::RemoveRule(const std::wstring& processName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::wstring lowerName = utils::ToLower(processName);
    
    m_rules.erase(
        std::remove_if(m_rules.begin(), m_rules.end(),
            [&lowerName](const InjectionRule& rule) {
                return rule.processName == lowerName;
            }),
        m_rules.end()
    );
    
    m_monitor->UnwatchProcess(processName);
    
    Log(LogLevel::Info, L"Removed auto-inject rule for: " + processName);
}

void AutoInjector::ClearRules() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_rules.clear();
    m_monitor->ClearWatchList();
}

AutoInjector::Statistics AutoInjector::GetStatistics() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stats;
}

void AutoInjector::OnProcessEvent(ProcessEvent event, const ProcessInfo& process) {
    if (event != ProcessEvent::Started) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::wstring lowerName = utils::ToLower(process.name);
    
    for (const auto& rule : m_rules) {
        if (rule.processName == lowerName) {
            Log(LogLevel::Info, L"Auto-inject triggered for: " + process.name + 
                L" (PID: " + std::to_wstring(process.pid) + L")");
            
             
            std::thread([this, process, rule]() {
                PerformInjection(process, rule);
            }).detach();
            
            break;
        }
    }
}

void AutoInjector::PerformInjection(const ProcessInfo& process, const InjectionRule& rule) {
     
    if (rule.delay > 0) {
        Log(LogLevel::Debug, L"Waiting " + std::to_wstring(rule.delay) + L"ms before injection");
        Sleep(rule.delay);
    }
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stats.totalAttempts++;
    }
    
     
    InjectionCore injector;
    injector.SetLogCallback([this](LogLevel level, const std::wstring& msg) {
        Log(level, msg);
    });
    
    InjectionResult result = injector.Inject(process.pid, rule.dllPath, rule.method);
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (result.success) {
            m_stats.successfulInjections++;
            Log(LogLevel::Info, L"Auto-injection successful: " + process.name);
        } else {
            m_stats.failedInjections++;
            Log(LogLevel::Error, L"Auto-injection failed: " + result.errorMessage);
        }
    }
}

void AutoInjector::Log(LogLevel level, const std::wstring& message) {
    if (m_logCallback) {
        m_logCallback(level, L"[AutoInject] " + message);
    }
}

}  
