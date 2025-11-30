#pragma once

#include "core/types.h"
#include "core/process_manager.h"
#include "core/injection_core.h"
#include "ui/hotkeys.h"
#include "ui/tray_icon.h"
#include <commctrl.h>
#include <dwmapi.h>

namespace xordll {

 
namespace ui {
    class TooltipManager;
}

 
constexpr wchar_t WINDOW_CLASS_NAME[] = L"xorDLLMainWindow";

 
enum ControlId {
    ID_PROCESS_LIST = 1001,
    ID_PROCESS_SEARCH = 1002,
    ID_DLL_LIST = 1003,
    ID_DLL_ADD = 1004,
    ID_DLL_REMOVE = 1005,
    ID_METHOD_COMBO = 1006,
    ID_BTN_INJECT = 1007,
    ID_BTN_EJECT = 1008,
    ID_BTN_REFRESH = 1009,
    ID_STATUS_BAR = 1010,
    
     
    ID_MENU_FILE_EXIT = 2001,
    ID_MENU_FILE_OPEN_DLL = 2002,
    ID_MENU_FILE_EXPORT_LOG = 2003,
    ID_MENU_INJECT_START = 2101,
    ID_MENU_INJECT_EJECT = 2102,
    ID_MENU_OPTIONS_SETTINGS = 2201,
    ID_MENU_OPTIONS_DARK_MODE = 2202,
    ID_MENU_OPTIONS_ALWAYS_TOP = 2203,
    ID_MENU_OPTIONS_MINIMIZE_TRAY = 2204,
    ID_MENU_HELP_ABOUT = 2301,
    ID_MENU_HELP_GITHUB = 2302,
    ID_MENU_HELP_SHORTCUTS = 2303,
    
     
    ID_TRAY_SHOW = 4001,
    ID_TRAY_INJECT = 4002,
    ID_TRAY_EXIT = 4003,
    
     
    ID_TIMER_REFRESH = 3001,
    ID_TIMER_ANIMATION = 3002
};

 
class MainWindow {
public:
    MainWindow();
    ~MainWindow();
    
     
    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;
    
     
    bool Create(HINSTANCE hInstance, int nCmdShow);
    
     
    int Run();
    
     
    HWND GetHandle() const { return m_hwnd; }
    
     
    void SetColorScheme(const ColorScheme& scheme);
    
     
    void AddLogEntry(LogLevel level, const std::wstring& message);
    
     
    void SetStatusText(const std::wstring& text, int part = 0);

private:
     
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    
     
    bool RegisterWindowClass(HINSTANCE hInstance);
    void CreateControls();
    void CreateMenu();
    void ApplyDarkMode();
    void LoadSettings();
    void SaveSettings();
    
     
    void OnCreate();
    void OnDestroy();
    void OnSize(int width, int height);
    void OnPaint();
    void OnCommand(WPARAM wParam, LPARAM lParam);
    void OnNotify(WPARAM wParam, LPARAM lParam);
    void OnTimer(WPARAM timerId);
    void OnDropFiles(HDROP hDrop);
    void OnHotkey(WPARAM wParam);
    void OnTrayIcon(LPARAM lParam);
    void OnMinimize();
    
     
    void RefreshProcessList();
    void AddDllToList();
    void RemoveDllFromList();
    void PerformInjection();
    void PerformEjection();
    void ShowAboutDialog();
    void ShowSettingsDialog();
    void ShowShortcutsDialog();
    void OpenGitHub();
    void ExportLog();
    void ToggleAlwaysOnTop();
    void MinimizeToTray();
    void RestoreFromTray();
    void ShowTrayMenu();
    
     
    void PopulateProcessList();
    void FilterProcessList(const std::wstring& filter);
    ProcessInfo* GetSelectedProcess();
    InjectionMethod GetSelectedMethod();
    std::wstring GetDllPath();
    void EnableControls(bool enable);
    void LoadBackgroundImage();
    void DrawCustomButton(DRAWITEMSTRUCT* dis);
    
     
    HWND m_hwnd;
    HWND m_hwndProcessList;
    HWND m_hwndProcessSearch;
    HWND m_hwndDllList;
    HWND m_hwndBtnDllAdd;
    HWND m_hwndBtnDllRemove;
    HWND m_hwndMethodCombo;
    HWND m_hwndBtnInject;
    HWND m_hwndBtnEject;
    HWND m_hwndBtnRefresh;
    HWND m_hwndSelectedProc;
    HWND m_hwndStatusBar;
    HMENU m_hMenu;
    
     
    std::vector<std::wstring> m_dllList;
    
     
    std::unique_ptr<ProcessManager> m_processManager;
    std::unique_ptr<InjectionCore> m_injectionCore;
    
     
    HINSTANCE m_hInstance;
    ColorScheme m_colorScheme;
    AppSettings m_settings;
    std::vector<ProcessInfo> m_filteredProcesses;
    ModuleHandle m_lastInjectedModule;
    ProcessId m_lastInjectedPid;
    ProcessId m_selectedPid;   
    
     
    HBRUSH m_hBackgroundBrush;
    HFONT m_hFont;
    HFONT m_hFontBold;
    HIMAGELIST m_hImageList;   
    HBITMAP m_hBgImage;        
    int m_bgWidth;
    int m_bgHeight;
    
     
    std::unique_ptr<ui::HotkeyManager> m_hotkeyManager;
    std::unique_ptr<ui::TrayIcon> m_trayIcon;
    std::unique_ptr<ui::TooltipManager> m_tooltipManager;
    std::unique_ptr<ui::AcceleratorManager> m_acceleratorManager;
    
     
    bool m_alwaysOnTop;
    bool m_minimizedToTray;
    
     
    static constexpr int WINDOW_WIDTH = 560;
    static constexpr int WINDOW_HEIGHT = 380;
    static constexpr int MARGIN = 10;
    static constexpr int CONTROL_HEIGHT = 22;
    static constexpr int BUTTON_WIDTH = 75;
};

 
class AboutDialog {
public:
    static INT_PTR Show(HWND hwndParent);
    
private:
    static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

}  
