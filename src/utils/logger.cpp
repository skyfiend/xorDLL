 

#include "utils/logger.h"
#include "utils/string_utils.h"
#include "utils/file_utils.h"
#include <iomanip>
#include <sstream>
#include <ctime>

namespace xordll {

Logger& Logger::Instance()
{
    static Logger instance;
    return instance;
}

Logger::Logger()
    : m_maxFileSize(5 * 1024 * 1024)
    , m_currentFileSize(0)
    , m_minLevel(LogLevel::Debug)
    , m_uiCallback(nullptr)
    , m_initialized(false)
{
}

Logger::~Logger()
{
    Shutdown();
}

bool Logger::Initialize(const std::wstring& logFilePath, size_t maxFileSize)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_initialized) {
        return true;
    }
    
    m_maxFileSize = maxFileSize;
    
     
    if (logFilePath.empty()) {
        m_logFilePath = GetDefaultLogPath();
    } else {
        m_logFilePath = logFilePath;
    }
    
     
    std::wstring dir = utils::GetDirectory(m_logFilePath);
    if (!dir.empty()) {
        utils::CreateDirectoryRecursive(dir);
    }
    
     
    m_logFile.open(m_logFilePath.c_str(), std::ios::app | std::ios::out);
    if (!m_logFile.is_open()) {
        return false;
    }
    
     
    m_logFile.seekp(0, std::ios::end);
    m_currentFileSize = static_cast<size_t>(m_logFile.tellp());
    
    m_initialized = true;
    
     
    Log(LogLevel::Info, L"=== Logger initialized ===");
    
    return true;
}

void Logger::Shutdown()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) {
        return;
    }
    
    if (m_logFile.is_open()) {
        m_logFile << L"=== Logger shutdown ===" << std::endl;
        m_logFile.close();
    }
    
    m_entries.clear();
    m_initialized = false;
}

void Logger::SetMinLevel(LogLevel level)
{
    m_minLevel = level;
}

void Logger::SetUICallback(LogCallback callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_uiCallback = callback;
}

void Logger::Log(LogLevel level, const std::wstring& message)
{
    if (level < m_minLevel) {
        return;
    }
    
    LogEntry entry(level, message);
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
         
        m_entries.push_back(entry);
        
         
        while (m_entries.size() > 1000) {
            m_entries.erase(m_entries.begin());
        }
        
         
        if (m_initialized) {
            WriteToFile(entry);
        }
        
         
        if (m_uiCallback) {
            m_uiCallback(level, message);
        }
    }
}

void Logger::Debug(const std::wstring& message)
{
    Log(LogLevel::Debug, message);
}

void Logger::Info(const std::wstring& message)
{
    Log(LogLevel::Info, message);
}

void Logger::Warning(const std::wstring& message)
{
    Log(LogLevel::Warning, message);
}

void Logger::Error(const std::wstring& message)
{
    Log(LogLevel::Error, message);
}

void Logger::LogWindowsError(const std::wstring& operation)
{
    DWORD error = GetLastError();
    std::wstring message = operation + L": " + utils::FormatWindowsError(error);
    Log(LogLevel::Error, message);
}

std::vector<LogEntry> Logger::GetRecentEntries(size_t count) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (count >= m_entries.size()) {
        return m_entries;
    }
    
    return std::vector<LogEntry>(
        m_entries.end() - static_cast<ptrdiff_t>(count),
        m_entries.end()
    );
}

void Logger::ClearEntries()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_entries.clear();
}

bool Logger::ExportToFile(const std::wstring& filePath) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::wofstream file(filePath.c_str(), std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }
    
    for (const auto& entry : m_entries) {
        file << FormatEntry(entry) << std::endl;
    }
    
    file.close();
    return true;
}

std::wstring Logger::LevelToString(LogLevel level)
{
    switch (level) {
        case LogLevel::Debug:   return L"DEBUG";
        case LogLevel::Info:    return L"INFO";
        case LogLevel::Warning: return L"WARNING";
        case LogLevel::Error:   return L"ERROR";
        default:                return L"UNKNOWN";
    }
}

COLORREF Logger::LevelToColor(LogLevel level)
{
    switch (level) {
        case LogLevel::Debug:   return RGB(128, 128, 128);   
        case LogLevel::Info:    return RGB(78, 201, 176);    
        case LogLevel::Warning: return RGB(220, 220, 170);   
        case LogLevel::Error:   return RGB(244, 71, 71);     
        default:                return RGB(255, 255, 255);   
    }
}

void Logger::WriteToFile(const LogEntry& entry)
{
    if (!m_logFile.is_open()) {
        return;
    }
    
     
    if (m_currentFileSize >= m_maxFileSize) {
        RotateLogFile();
    }
    
    std::wstring formatted = FormatEntry(entry);
    m_logFile << formatted << std::endl;
    m_logFile.flush();
    
    m_currentFileSize += (formatted.length() + 1) * sizeof(wchar_t);
}

void Logger::RotateLogFile()
{
    m_logFile.close();
    
     
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &time);
    
    std::wstringstream ss;
    ss << m_logFilePath << L"."
       << std::put_time(&tm, L"%Y%m%d_%H%M%S")
       << L".old";
    
    std::wstring oldPath = ss.str();
    MoveFileW(m_logFilePath.c_str(), oldPath.c_str());
    
     
    m_logFile.open(m_logFilePath.c_str(), std::ios::out | std::ios::trunc);
    m_currentFileSize = 0;
    
    Log(LogLevel::Info, L"Log file rotated");
}

std::wstring Logger::FormatEntry(const LogEntry& entry) const
{
    auto time = std::chrono::system_clock::to_time_t(entry.timestamp);
    std::tm tm;
    localtime_s(&tm, &time);
    
    std::wstringstream ss;
    ss << std::put_time(&tm, L"[%Y-%m-%d %H:%M:%S] ")
       << L"[" << LevelToString(entry.level) << L"] "
       << entry.message;
    
    return ss.str();
}

std::wstring Logger::GetDefaultLogPath() const
{
    std::wstring appData = utils::GetAppDataPath(L"xorDLL");
    if (appData.empty()) {
        return L"xorDLL.log";
    }
    return appData + L"\\xorDLL.log";
}

}  
