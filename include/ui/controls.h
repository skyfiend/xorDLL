#pragma once

#include "core/types.h"
#include <commctrl.h>
#include <dwmapi.h>
#include <functional>

namespace xordll {
namespace ui {

 
enum class ButtonStyle {
    Default,
    Primary,     
    Success,     
    Warning,     
    Danger,      
    Flat         
};

 
class CustomButton {
public:
    CustomButton();
    ~CustomButton();
    
     
    bool Create(
        HWND parent,
        int id,
        const std::wstring& text,
        int x, int y, int width, int height,
        ButtonStyle style = ButtonStyle::Default
    );
    
     
    void SetText(const std::wstring& text);
    
     
    void SetEnabled(bool enabled);
    
     
    void SetStyle(ButtonStyle style);
    
     
    HWND GetHandle() const { return m_hwnd; }
    
     
    static LRESULT CALLBACK SubclassProc(
        HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
        UINT_PTR uIdSubclass, DWORD_PTR dwRefData
    );

private:
    HWND m_hwnd;
    ButtonStyle m_style;
    bool m_isHovered;
    bool m_isPressed;
    ColorScheme m_colors;
};

 
class CustomProgressBar {
public:
    CustomProgressBar();
    ~CustomProgressBar();
    
    bool Create(HWND parent, int id, int x, int y, int width, int height);
    
    void SetProgress(int percent);
    int GetProgress() const { return m_progress; }
    
    void SetIndeterminate(bool indeterminate);
    
    void SetColors(COLORREF background, COLORREF foreground);
    
    HWND GetHandle() const { return m_hwnd; }
    
    static LRESULT CALLBACK SubclassProc(
        HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
        UINT_PTR uIdSubclass, DWORD_PTR dwRefData
    );

private:
    HWND m_hwnd;
    int m_progress;
    bool m_indeterminate;
    COLORREF m_bgColor;
    COLORREF m_fgColor;
    int m_animOffset;
};

 
class ProcessListView {
public:
    ProcessListView();
    ~ProcessListView();
    
    bool Create(HWND parent, int id, int x, int y, int width, int height);
    
    void SetProcesses(const std::vector<ProcessInfo>& processes);
    void Clear();
    
    ProcessInfo* GetSelectedProcess();
    int GetSelectedIndex() const;
    
    void SetFilter(const std::wstring& filter);
    
    void SetColorScheme(const ColorScheme& scheme);
    
    void Refresh();
    
    HWND GetHandle() const { return m_hwnd; }
    
     
    std::function<void(const ProcessInfo&)> OnProcessSelected;
    std::function<void(const ProcessInfo&)> OnProcessDoubleClick;
    
    static LRESULT CALLBACK SubclassProc(
        HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
        UINT_PTR uIdSubclass, DWORD_PTR dwRefData
    );

private:
    void PopulateList();
    void ApplyFilter();
    
    HWND m_hwnd;
    HIMAGELIST m_imageList;
    std::vector<ProcessInfo> m_allProcesses;
    std::vector<ProcessInfo> m_filteredProcesses;
    std::wstring m_filter;
    ColorScheme m_colors;
};

 
class LogView {
public:
    LogView();
    ~LogView();
    
    bool Create(HWND parent, int id, int x, int y, int width, int height);
    
    void AddEntry(LogLevel level, const std::wstring& message);
    void Clear();
    
    void SetMaxEntries(int max);
    void SetColorScheme(const ColorScheme& scheme);
    
    void ScrollToBottom();
    
    HWND GetHandle() const { return m_hwnd; }
    
    static LRESULT CALLBACK SubclassProc(
        HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
        UINT_PTR uIdSubclass, DWORD_PTR dwRefData
    );

private:
    struct LogItem {
        LogLevel level;
        std::wstring timestamp;
        std::wstring message;
    };
    
    HWND m_hwnd;
    std::vector<LogItem> m_entries;
    int m_maxEntries;
    ColorScheme m_colors;
};

 
class TooltipManager {
public:
    TooltipManager();
    ~TooltipManager();
    
    bool Create(HWND parent);
    
    void AddTooltip(HWND control, const std::wstring& text);
    void UpdateTooltip(HWND control, const std::wstring& text);
    void RemoveTooltip(HWND control);
    
    HWND GetHandle() const { return m_hwnd; }

private:
    HWND m_hwnd;
    HWND m_parent;
};

 
namespace helpers {

 
void ApplyDarkMode(HWND hwnd, bool dark);

 
void SetRoundedCorners(HWND hwnd, bool rounded);

 
HFONT CreateUIFont(int size, int weight = FW_NORMAL, bool italic = false);

 
float GetDpiScale(HWND hwnd);

 
int ScaleByDpi(int value, HWND hwnd);

 
void CenterWindow(HWND hwnd, HWND parent = nullptr);

 
void FlashWindow(HWND hwnd, int count = 3);

 
void SetWindowTransparency(HWND hwnd, BYTE alpha);

}  

}  
}  
