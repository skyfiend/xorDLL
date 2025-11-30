#pragma once

#include <windows.h>
#include <shellapi.h>
#include <string>
#include <functional>

namespace xordll {
namespace ui {

 
constexpr UINT WM_TRAYICON = WM_USER + 100;

 
class TrayIcon {
public:
    TrayIcon();
    ~TrayIcon();
    
     
    bool Create(HWND hwnd, HICON icon = nullptr, const std::wstring& tooltip = L"xorDLL");
    
     
    void Remove();
    
     
    void SetTooltip(const std::wstring& tooltip);
    
     
    void SetIcon(HICON icon);
    
     
    void ShowBalloon(
        const std::wstring& title,
        const std::wstring& message,
        DWORD icon = NIIF_INFO,
        UINT timeout = 3000
    );
    
     
    bool HandleMessage(LPARAM lParam);
    
     
    void SetOnLeftClick(std::function<void()> callback) { m_onLeftClick = callback; }
    
     
    void SetOnRightClick(std::function<void()> callback) { m_onRightClick = callback; }
    
     
    void SetOnDoubleClick(std::function<void()> callback) { m_onDoubleClick = callback; }
    
     
    void SetOnBalloonClick(std::function<void()> callback) { m_onBalloonClick = callback; }
    
     
    bool IsVisible() const { return m_visible; }
    
     
    static void ShowContextMenu(HWND hwnd, HMENU menu);

private:
    NOTIFYICONDATAW m_nid;
    HWND m_hwnd;
    bool m_visible;
    
    std::function<void()> m_onLeftClick;
    std::function<void()> m_onRightClick;
    std::function<void()> m_onDoubleClick;
    std::function<void()> m_onBalloonClick;
};

 
class TrayMenuBuilder {
public:
    TrayMenuBuilder();
    ~TrayMenuBuilder();
    
     
    TrayMenuBuilder& AddItem(UINT id, const std::wstring& text, bool enabled = true);
    
     
    TrayMenuBuilder& AddSeparator();
    
     
    TrayMenuBuilder& AddCheckItem(UINT id, const std::wstring& text, bool checked);
    
     
    TrayMenuBuilder& AddSubmenu(const std::wstring& text, HMENU submenu);
    
     
    TrayMenuBuilder& SetDefault(UINT id);
    
     
    HMENU Build();

private:
    HMENU m_menu;
    UINT m_defaultId;
};

}  
}  
