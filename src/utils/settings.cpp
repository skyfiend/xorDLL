 

#include "utils/settings.h"
#include "utils/file_utils.h"
#include "utils/string_utils.h"
#include "utils/logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace xordll {

Settings& Settings::Instance() {
    static Settings instance;
    return instance;
}

Settings::Settings() {
    ResetToDefaults();
}

void Settings::ResetToDefaults() {
     
    m_darkMode = true;
    m_autoRefresh = true;
    m_refreshInterval = 5000;
    m_minimizeToTray = false;
    m_confirmInjection = true;
    
     
    m_defaultMethod = InjectionMethod::CreateRemoteThread;
    m_autoSelectMethod = false;
    
     
    m_recentDlls.clear();
    m_maxRecentDlls = 10;
    
     
    m_windowX = -1;
    m_windowY = -1;
    m_windowWidth = 700;
    m_windowHeight = 550;
    
     
    m_language = L"en";
    
     
    m_logLevel = LogLevel::Info;
    m_logToFile = true;
    
     
    m_showSystemProcesses = false;
    m_showOnlyAccessible = true;
}

std::wstring Settings::GetSettingsPath() const {
    return utils::GetAppDataPath(L"xorDLL") + L"\\settings.ini";
}

bool Settings::Load() {
    std::wstring path = GetSettingsPath();
    
    if (!utils::FileExists(path)) {
        LOG_INFO(L"Settings file not found, using defaults");
        return true;   
    }
    
    std::wifstream file(path.c_str());
    if (!file.is_open()) {
        LOG_WARNING(L"Failed to open settings file");
        return false;
    }
    
    std::wstring line;
    std::wstring currentSection;
    
    while (std::getline(file, line)) {
        line = utils::Trim(line);
        
         
        if (line.empty() || line[0] == L';' || line[0] == L'#') {
            continue;
        }
        
         
        if (line[0] == L'[' && line.back() == L']') {
            currentSection = line.substr(1, line.length() - 2);
            continue;
        }
        
         
        size_t eqPos = line.find(L'=');
        if (eqPos == std::wstring::npos) continue;
        
        std::wstring key = utils::Trim(line.substr(0, eqPos));
        std::wstring value = utils::Trim(line.substr(eqPos + 1));
        
         
        if (currentSection == L"General") {
            if (key == L"DarkMode") m_darkMode = (value == L"true" || value == L"1");
            else if (key == L"AutoRefresh") m_autoRefresh = (value == L"true" || value == L"1");
            else if (key == L"RefreshInterval") m_refreshInterval = std::stoi(value);
            else if (key == L"MinimizeToTray") m_minimizeToTray = (value == L"true" || value == L"1");
            else if (key == L"ConfirmInjection") m_confirmInjection = (value == L"true" || value == L"1");
            else if (key == L"Language") m_language = value;
        }
        else if (currentSection == L"Injection") {
            if (key == L"DefaultMethod") m_defaultMethod = StringToInjectionMethod(value);
            else if (key == L"AutoSelectMethod") m_autoSelectMethod = (value == L"true" || value == L"1");
        }
        else if (currentSection == L"RecentDLLs") {
            if (key.find(L"DLL") == 0 && !value.empty()) {
                m_recentDlls.push_back(value);
            }
            else if (key == L"MaxCount") m_maxRecentDlls = std::stoi(value);
        }
        else if (currentSection == L"Window") {
            if (key == L"X") m_windowX = std::stoi(value);
            else if (key == L"Y") m_windowY = std::stoi(value);
            else if (key == L"Width") m_windowWidth = std::stoi(value);
            else if (key == L"Height") m_windowHeight = std::stoi(value);
        }
        else if (currentSection == L"Logging") {
            if (key == L"Level") m_logLevel = StringToLogLevel(value);
            else if (key == L"ToFile") m_logToFile = (value == L"true" || value == L"1");
        }
        else if (currentSection == L"ProcessFilter") {
            if (key == L"ShowSystem") m_showSystemProcesses = (value == L"true" || value == L"1");
            else if (key == L"ShowOnlyAccessible") m_showOnlyAccessible = (value == L"true" || value == L"1");
        }
    }
    
    file.close();
    LOG_INFO(L"Settings loaded from " + path);
    return true;
}

bool Settings::Save() {
    std::wstring path = GetSettingsPath();
    
     
    std::wstring dir = utils::GetDirectory(path);
    utils::CreateDirectoryRecursive(dir);
    
    std::wofstream file(path.c_str());
    if (!file.is_open()) {
        LOG_ERROR(L"Failed to save settings to " + path);
        return false;
    }
    
     
    file << L"; xorDLL Settings File" << std::endl;
    file << L"; Auto-generated - Do not edit manually" << std::endl;
    file << std::endl;
    
     
    file << L"[General]" << std::endl;
    file << L"DarkMode=" << (m_darkMode ? L"true" : L"false") << std::endl;
    file << L"AutoRefresh=" << (m_autoRefresh ? L"true" : L"false") << std::endl;
    file << L"RefreshInterval=" << m_refreshInterval << std::endl;
    file << L"MinimizeToTray=" << (m_minimizeToTray ? L"true" : L"false") << std::endl;
    file << L"ConfirmInjection=" << (m_confirmInjection ? L"true" : L"false") << std::endl;
    file << L"Language=" << m_language << std::endl;
    file << std::endl;
    
     
    file << L"[Injection]" << std::endl;
    file << L"DefaultMethod=" << InjectionMethodToString(m_defaultMethod) << std::endl;
    file << L"AutoSelectMethod=" << (m_autoSelectMethod ? L"true" : L"false") << std::endl;
    file << std::endl;
    
     
    file << L"[RecentDLLs]" << std::endl;
    file << L"MaxCount=" << m_maxRecentDlls << std::endl;
    for (size_t i = 0; i < m_recentDlls.size(); ++i) {
        file << L"DLL" << i << L"=" << m_recentDlls[i] << std::endl;
    }
    file << std::endl;
    
     
    file << L"[Window]" << std::endl;
    file << L"X=" << m_windowX << std::endl;
    file << L"Y=" << m_windowY << std::endl;
    file << L"Width=" << m_windowWidth << std::endl;
    file << L"Height=" << m_windowHeight << std::endl;
    file << std::endl;
    
     
    file << L"[Logging]" << std::endl;
    file << L"Level=" << LogLevelToString(m_logLevel) << std::endl;
    file << L"ToFile=" << (m_logToFile ? L"true" : L"false") << std::endl;
    file << std::endl;
    
     
    file << L"[ProcessFilter]" << std::endl;
    file << L"ShowSystem=" << (m_showSystemProcesses ? L"true" : L"false") << std::endl;
    file << L"ShowOnlyAccessible=" << (m_showOnlyAccessible ? L"true" : L"false") << std::endl;
    
    file.close();
    LOG_DEBUG(L"Settings saved to " + path);
    return true;
}

void Settings::AddRecentDll(const std::wstring& path) {
     
    auto it = std::find(m_recentDlls.begin(), m_recentDlls.end(), path);
    if (it != m_recentDlls.end()) {
        m_recentDlls.erase(it);
    }
    
     
    m_recentDlls.insert(m_recentDlls.begin(), path);
    
     
    while (static_cast<int>(m_recentDlls.size()) > m_maxRecentDlls) {
        m_recentDlls.pop_back();
    }
}

std::wstring Settings::InjectionMethodToString(InjectionMethod method) const {
    switch (method) {
        case InjectionMethod::CreateRemoteThread: return L"CreateRemoteThread";
        case InjectionMethod::NtCreateThreadEx: return L"NtCreateThreadEx";
        case InjectionMethod::QueueUserAPC: return L"QueueUserAPC";
        case InjectionMethod::SetWindowsHookEx: return L"SetWindowsHookEx";
        case InjectionMethod::ManualMap: return L"ManualMapping";
        default: return L"CreateRemoteThread";
    }
}

InjectionMethod Settings::StringToInjectionMethod(const std::wstring& str) const {
    if (str == L"CreateRemoteThread") return InjectionMethod::CreateRemoteThread;
    if (str == L"NtCreateThreadEx") return InjectionMethod::NtCreateThreadEx;
    if (str == L"QueueUserAPC") return InjectionMethod::QueueUserAPC;
    if (str == L"SetWindowsHookEx") return InjectionMethod::SetWindowsHookEx;
    if (str == L"ManualMapping") return InjectionMethod::ManualMap;
    return InjectionMethod::CreateRemoteThread;
}

std::wstring Settings::LogLevelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::Debug: return L"Debug";
        case LogLevel::Info: return L"Info";
        case LogLevel::Warning: return L"Warning";
        case LogLevel::Error: return L"Error";
        default: return L"Info";
    }
}

LogLevel Settings::StringToLogLevel(const std::wstring& str) const {
    if (str == L"Debug") return LogLevel::Debug;
    if (str == L"Info") return LogLevel::Info;
    if (str == L"Warning") return LogLevel::Warning;
    if (str == L"Error") return LogLevel::Error;
    return LogLevel::Info;
}

}  
