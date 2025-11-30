#pragma once

#include "core/types.h"
#include <string>
#include <vector>
#include <map>

namespace xordll {

 
class Settings {
public:
     
    static Settings& Instance();
    
     
    Settings(const Settings&) = delete;
    Settings& operator=(const Settings&) = delete;
    
     
    bool Load();
    
     
    bool Save();
    
     
    void ResetToDefaults();
    
     
    bool GetDarkMode() const { return m_darkMode; }
    void SetDarkMode(bool value) { m_darkMode = value; }
    
    bool GetAutoRefresh() const { return m_autoRefresh; }
    void SetAutoRefresh(bool value) { m_autoRefresh = value; }
    
    int GetRefreshInterval() const { return m_refreshInterval; }
    void SetRefreshInterval(int ms) { m_refreshInterval = ms; }
    
    bool GetMinimizeToTray() const { return m_minimizeToTray; }
    void SetMinimizeToTray(bool value) { m_minimizeToTray = value; }
    
    bool GetConfirmInjection() const { return m_confirmInjection; }
    void SetConfirmInjection(bool value) { m_confirmInjection = value; }
    
     
    InjectionMethod GetDefaultMethod() const { return m_defaultMethod; }
    void SetDefaultMethod(InjectionMethod method) { m_defaultMethod = method; }
    
    bool GetAutoSelectMethod() const { return m_autoSelectMethod; }
    void SetAutoSelectMethod(bool value) { m_autoSelectMethod = value; }
    
     
    std::vector<std::wstring> GetRecentDlls() const { return m_recentDlls; }
    void AddRecentDll(const std::wstring& path);
    void ClearRecentDlls() { m_recentDlls.clear(); }
    
    int GetMaxRecentDlls() const { return m_maxRecentDlls; }
    void SetMaxRecentDlls(int count) { m_maxRecentDlls = count; }
    
     
    int GetWindowX() const { return m_windowX; }
    void SetWindowX(int x) { m_windowX = x; }
    
    int GetWindowY() const { return m_windowY; }
    void SetWindowY(int y) { m_windowY = y; }
    
    int GetWindowWidth() const { return m_windowWidth; }
    void SetWindowWidth(int w) { m_windowWidth = w; }
    
    int GetWindowHeight() const { return m_windowHeight; }
    void SetWindowHeight(int h) { m_windowHeight = h; }
    
     
    std::wstring GetLanguage() const { return m_language; }
    void SetLanguage(const std::wstring& lang) { m_language = lang; }
    
     
    LogLevel GetLogLevel() const { return m_logLevel; }
    void SetLogLevel(LogLevel level) { m_logLevel = level; }
    
    bool GetLogToFile() const { return m_logToFile; }
    void SetLogToFile(bool value) { m_logToFile = value; }
    
     
    bool GetShowSystemProcesses() const { return m_showSystemProcesses; }
    void SetShowSystemProcesses(bool value) { m_showSystemProcesses = value; }
    
    bool GetShowOnlyAccessible() const { return m_showOnlyAccessible; }
    void SetShowOnlyAccessible(bool value) { m_showOnlyAccessible = value; }
    
     
    std::wstring GetSettingsPath() const;

private:
    Settings();
    ~Settings() = default;
    
     
    std::wstring InjectionMethodToString(InjectionMethod method) const;
    InjectionMethod StringToInjectionMethod(const std::wstring& str) const;
    std::wstring LogLevelToString(LogLevel level) const;
    LogLevel StringToLogLevel(const std::wstring& str) const;
    
     
    bool m_darkMode;
    bool m_autoRefresh;
    int m_refreshInterval;
    bool m_minimizeToTray;
    bool m_confirmInjection;
    
     
    InjectionMethod m_defaultMethod;
    bool m_autoSelectMethod;
    
     
    std::vector<std::wstring> m_recentDlls;
    int m_maxRecentDlls;
    
     
    int m_windowX;
    int m_windowY;
    int m_windowWidth;
    int m_windowHeight;
    
     
    std::wstring m_language;
    
     
    LogLevel m_logLevel;
    bool m_logToFile;
    
     
    bool m_showSystemProcesses;
    bool m_showOnlyAccessible;
};

}  
