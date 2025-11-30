#pragma once

#include "core/types.h"
#include <windows.h>
#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <set>
#include <map>

namespace xordll {

 
enum class ProcessEvent {
    Started,
    Terminated
};

 
using ProcessEventCallback = std::function<void(ProcessEvent event, const ProcessInfo& process)>;

 
class ProcessMonitor {
public:
    ProcessMonitor();
    ~ProcessMonitor();
    
     
    bool Start();
    
     
    void Stop();
    
     
    bool IsRunning() const { return m_running; }
    
     
    void WatchProcess(const std::wstring& processName);
    
     
    void UnwatchProcess(const std::wstring& processName);
    
     
    void ClearWatchList();
    
     
    std::vector<std::wstring> GetWatchList() const;
    
     
    void SetCallback(ProcessEventCallback callback) { m_callback = callback; }
    
     
    void SetPollingInterval(DWORD intervalMs) { m_pollingInterval = intervalMs; }

private:
    void MonitorThread();
    void CheckForNewProcesses();
    void CheckForTerminatedProcesses();
    bool IsWatched(const std::wstring& processName) const;
    
    std::atomic<bool> m_running;
    std::thread m_thread;
    mutable std::mutex m_mutex;
    
    std::set<std::wstring> m_watchList;
    std::map<ProcessId, ProcessInfo> m_knownProcesses;
    
    ProcessEventCallback m_callback;
    DWORD m_pollingInterval;
};

 
class AutoInjector {
public:
    using LogCallback = std::function<void(LogLevel, const std::wstring&)>;
    
    AutoInjector();
    ~AutoInjector();
    
     
    void SetLogCallback(LogCallback callback) { m_logCallback = callback; }
    
     
    bool Start();
    
     
    void Stop();
    
     
    bool IsRunning() const;
    
     
    void AddRule(
        const std::wstring& processName,
        const std::wstring& dllPath,
        InjectionMethod method = InjectionMethod::CreateRemoteThread,
        DWORD delay = 0
    );
    
     
    void RemoveRule(const std::wstring& processName);
    
     
    void ClearRules();
    
     
    struct Statistics {
        int totalAttempts;
        int successfulInjections;
        int failedInjections;
    };
    Statistics GetStatistics() const;

private:
    struct InjectionRule {
        std::wstring processName;
        std::wstring dllPath;
        InjectionMethod method;
        DWORD delay;
    };
    
    void OnProcessEvent(ProcessEvent event, const ProcessInfo& process);
    void PerformInjection(const ProcessInfo& process, const InjectionRule& rule);
    void Log(LogLevel level, const std::wstring& message);
    
    std::unique_ptr<ProcessMonitor> m_monitor;
    std::vector<InjectionRule> m_rules;
    mutable std::mutex m_mutex;
    
    Statistics m_stats;
    LogCallback m_logCallback;
};

}  
