#pragma once

#include "core/types.h"
#include <fstream>
#include <mutex>
#include <queue>
#include <chrono>

namespace xordll {

 
struct LogEntry {
    LogLevel level;
    std::wstring message;
    std::chrono::system_clock::time_point timestamp;
    
    LogEntry(LogLevel lvl, const std::wstring& msg)
        : level(lvl)
        , message(msg)
        , timestamp(std::chrono::system_clock::now()) {}
};

 
class Logger {
public:
     
    static Logger& Instance();
    
     
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;
    
     
    bool Initialize(
        const std::wstring& logFilePath = L"",
        size_t maxFileSize = 5 * 1024 * 1024   
    );
    
     
    void Shutdown();
    
     
    void SetMinLevel(LogLevel level);
    
     
    void SetUICallback(LogCallback callback);
    
     
    void Log(LogLevel level, const std::wstring& message);
    
     
    void Debug(const std::wstring& message);
    
     
    void Info(const std::wstring& message);
    
     
    void Warning(const std::wstring& message);
    
     
    void Error(const std::wstring& message);
    
     
    void LogWindowsError(const std::wstring& operation);
    
     
    std::vector<LogEntry> GetRecentEntries(size_t count = 100) const;
    
     
    void ClearEntries();
    
     
    bool ExportToFile(const std::wstring& filePath) const;
    
     
    static std::wstring LevelToString(LogLevel level);
    
     
    static COLORREF LevelToColor(LogLevel level);

private:
    Logger();
    ~Logger();
    
    void WriteToFile(const LogEntry& entry);
    void RotateLogFile();
    std::wstring FormatEntry(const LogEntry& entry) const;
    std::wstring GetDefaultLogPath() const;
    
    std::wofstream m_logFile;
    std::wstring m_logFilePath;
    size_t m_maxFileSize;
    size_t m_currentFileSize;
    
    LogLevel m_minLevel;
    LogCallback m_uiCallback;
    
    std::vector<LogEntry> m_entries;
    mutable std::mutex m_mutex;
    
    bool m_initialized;
};

 
#define LOG_DEBUG(msg) xordll::Logger::Instance().Debug(msg)
#define LOG_INFO(msg) xordll::Logger::Instance().Info(msg)
#define LOG_WARNING(msg) xordll::Logger::Instance().Warning(msg)
#define LOG_ERROR(msg) xordll::Logger::Instance().Error(msg)
#define LOG_WIN_ERROR(op) xordll::Logger::Instance().LogWindowsError(op)

}  
