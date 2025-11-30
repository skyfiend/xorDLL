 

#include "ui/tray_icon.h"
#include "utils/logger.h"

namespace xordll {
namespace ui {

 
 
 

TrayIcon::TrayIcon()
    : m_hwnd(nullptr)
    , m_visible(false)
{
    ZeroMemory(&m_nid, sizeof(m_nid));
}

TrayIcon::~TrayIcon() {
    Remove();
}

bool TrayIcon::Create(HWND hwnd, HICON icon, const std::wstring& tooltip) {
    if (m_visible) {
        Remove();
    }
    
    m_hwnd = hwnd;
    
    m_nid.cbSize = sizeof(m_nid);
    m_nid.hWnd = hwnd;
    m_nid.uID = 1;
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
    m_nid.uCallbackMessage = WM_TRAYICON;
    m_nid.hIcon = icon ? icon : LoadIcon(nullptr, IDI_APPLICATION);
    m_nid.uVersion = NOTIFYICON_VERSION_4;
    
    wcsncpy_s(m_nid.szTip, tooltip.c_str(), _TRUNCATE);
    
    if (Shell_NotifyIconW(NIM_ADD, &m_nid)) {
        Shell_NotifyIconW(NIM_SETVERSION, &m_nid);
        m_visible = true;
        LOG_DEBUG(L"Tray icon created");
        return true;
    }
    
    LOG_WARNING(L"Failed to create tray icon");
    return false;
}

void TrayIcon::Remove() {
    if (m_visible) {
        Shell_NotifyIconW(NIM_DELETE, &m_nid);
        m_visible = false;
        LOG_DEBUG(L"Tray icon removed");
    }
}

void TrayIcon::SetTooltip(const std::wstring& tooltip) {
    if (!m_visible) return;
    
    wcsncpy_s(m_nid.szTip, tooltip.c_str(), _TRUNCATE);
    m_nid.uFlags = NIF_TIP;
    Shell_NotifyIconW(NIM_MODIFY, &m_nid);
}

void TrayIcon::SetIcon(HICON icon) {
    if (!m_visible) return;
    
    m_nid.hIcon = icon;
    m_nid.uFlags = NIF_ICON;
    Shell_NotifyIconW(NIM_MODIFY, &m_nid);
}

void TrayIcon::ShowBalloon(
    const std::wstring& title,
    const std::wstring& message,
    DWORD icon,
    UINT timeout
) {
    if (!m_visible) return;
    
    m_nid.uFlags = NIF_INFO;
    m_nid.dwInfoFlags = icon;
    m_nid.uTimeout = timeout;
    
    wcsncpy_s(m_nid.szInfoTitle, title.c_str(), _TRUNCATE);
    wcsncpy_s(m_nid.szInfo, message.c_str(), _TRUNCATE);
    
    Shell_NotifyIconW(NIM_MODIFY, &m_nid);
}

bool TrayIcon::HandleMessage(LPARAM lParam) {
    UINT msg = LOWORD(lParam);
    
    switch (msg) {
        case WM_LBUTTONUP:
            if (m_onLeftClick) {
                m_onLeftClick();
                return true;
            }
            break;
            
        case WM_RBUTTONUP:
            if (m_onRightClick) {
                m_onRightClick();
                return true;
            }
            break;
            
        case WM_LBUTTONDBLCLK:
            if (m_onDoubleClick) {
                m_onDoubleClick();
                return true;
            }
            break;
            
        case NIN_BALLOONUSERCLICK:
            if (m_onBalloonClick) {
                m_onBalloonClick();
                return true;
            }
            break;
    }
    
    return false;
}

void TrayIcon::ShowContextMenu(HWND hwnd, HMENU menu) {
     
    POINT pt;
    GetCursorPos(&pt);
    
     
    SetForegroundWindow(hwnd);
    
     
    TrackPopupMenu(
        menu,
        TPM_RIGHTBUTTON | TPM_BOTTOMALIGN,
        pt.x, pt.y,
        0,
        hwnd,
        nullptr
    );
    
     
    PostMessage(hwnd, WM_NULL, 0, 0);
}

 
 
 

TrayMenuBuilder::TrayMenuBuilder()
    : m_menu(nullptr)
    , m_defaultId(0)
{
    m_menu = CreatePopupMenu();
}

TrayMenuBuilder::~TrayMenuBuilder() {
     
}

TrayMenuBuilder& TrayMenuBuilder::AddItem(UINT id, const std::wstring& text, bool enabled) {
    UINT flags = MF_STRING;
    if (!enabled) flags |= MF_GRAYED;
    
    AppendMenuW(m_menu, flags, id, text.c_str());
    return *this;
}

TrayMenuBuilder& TrayMenuBuilder::AddSeparator() {
    AppendMenuW(m_menu, MF_SEPARATOR, 0, nullptr);
    return *this;
}

TrayMenuBuilder& TrayMenuBuilder::AddCheckItem(UINT id, const std::wstring& text, bool checked) {
    UINT flags = MF_STRING;
    if (checked) flags |= MF_CHECKED;
    
    AppendMenuW(m_menu, flags, id, text.c_str());
    return *this;
}

TrayMenuBuilder& TrayMenuBuilder::AddSubmenu(const std::wstring& text, HMENU submenu) {
    AppendMenuW(m_menu, MF_POPUP, reinterpret_cast<UINT_PTR>(submenu), text.c_str());
    return *this;
}

TrayMenuBuilder& TrayMenuBuilder::SetDefault(UINT id) {
    m_defaultId = id;
    return *this;
}

HMENU TrayMenuBuilder::Build() {
    if (m_defaultId != 0) {
        SetMenuDefaultItem(m_menu, m_defaultId, FALSE);
    }
    
    HMENU result = m_menu;
    m_menu = nullptr;   
    return result;
}

}  
}  
