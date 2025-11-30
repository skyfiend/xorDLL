 

#include "ui/controls.h"
#include "utils/string_utils.h"
#include "utils/logger.h"
#include <windowsx.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <algorithm>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")

namespace xordll {
namespace ui {

 
 
 

CustomButton::CustomButton()
    : m_hwnd(nullptr)
    , m_style(ButtonStyle::Default)
    , m_isHovered(false)
    , m_isPressed(false)
    , m_colors(ColorScheme::Dark())
{
}

CustomButton::~CustomButton() {
    if (m_hwnd) {
        RemoveWindowSubclass(m_hwnd, SubclassProc, 0);
    }
}

bool CustomButton::Create(
    HWND parent, int id, const std::wstring& text,
    int x, int y, int width, int height, ButtonStyle style
) {
    m_style = style;
    
    m_hwnd = CreateWindowExW(
        0,
        L"BUTTON",
        text.c_str(),
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        x, y, width, height,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        GetModuleHandle(nullptr),
        nullptr
    );
    
    if (!m_hwnd) return false;
    
     
    SetWindowSubclass(m_hwnd, SubclassProc, 0, reinterpret_cast<DWORD_PTR>(this));
    
    return true;
}

void CustomButton::SetText(const std::wstring& text) {
    SetWindowTextW(m_hwnd, text.c_str());
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void CustomButton::SetEnabled(bool enabled) {
    EnableWindow(m_hwnd, enabled);
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void CustomButton::SetStyle(ButtonStyle style) {
    m_style = style;
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

LRESULT CALLBACK CustomButton::SubclassProc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
    UINT_PTR uIdSubclass, DWORD_PTR dwRefData
) {
    auto* btn = reinterpret_cast<CustomButton*>(dwRefData);
    
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            RECT rc;
            GetClientRect(hwnd, &rc);
            
            bool enabled = IsWindowEnabled(hwnd);
            
             
            COLORREF bgColor, textColor, borderColor;
            
            switch (btn->m_style) {
                case ButtonStyle::Primary:
                    bgColor = btn->m_isPressed ? RGB(0, 90, 158) :
                              btn->m_isHovered ? RGB(28, 151, 234) : RGB(0, 122, 204);
                    textColor = RGB(255, 255, 255);
                    borderColor = bgColor;
                    break;
                case ButtonStyle::Success:
                    bgColor = btn->m_isPressed ? RGB(50, 150, 130) :
                              btn->m_isHovered ? RGB(90, 220, 196) : RGB(78, 201, 176);
                    textColor = RGB(255, 255, 255);
                    borderColor = bgColor;
                    break;
                case ButtonStyle::Danger:
                    bgColor = btn->m_isPressed ? RGB(180, 50, 50) :
                              btn->m_isHovered ? RGB(255, 100, 100) : RGB(244, 71, 71);
                    textColor = RGB(255, 255, 255);
                    borderColor = bgColor;
                    break;
                case ButtonStyle::Flat:
                    bgColor = btn->m_isPressed ? RGB(60, 60, 63) :
                              btn->m_isHovered ? RGB(70, 70, 74) : RGB(45, 45, 48);
                    textColor = RGB(220, 220, 220);
                    borderColor = bgColor;
                    break;
                default:
                    bgColor = btn->m_isPressed ? RGB(60, 60, 63) :
                              btn->m_isHovered ? RGB(70, 70, 74) : RGB(51, 51, 55);
                    textColor = RGB(220, 220, 220);
                    borderColor = RGB(63, 63, 70);
                    break;
            }
            
            if (!enabled) {
                bgColor = RGB(45, 45, 48);
                textColor = RGB(100, 100, 100);
                borderColor = RGB(55, 55, 58);
            }
            
             
            HBRUSH hBrush = CreateSolidBrush(bgColor);
            FillRect(hdc, &rc, hBrush);
            DeleteObject(hBrush);
            
             
            if (btn->m_style != ButtonStyle::Flat) {
                HPEN hPen = CreatePen(PS_SOLID, 1, borderColor);
                HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
                HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
                RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 4, 4);
                SelectObject(hdc, hOldBrush);
                SelectObject(hdc, hOldPen);
                DeleteObject(hPen);
            }
            
             
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, textColor);
            
            wchar_t text[256];
            GetWindowTextW(hwnd, text, 256);
            
            HFONT hFont = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
            if (hFont) SelectObject(hdc, hFont);
            
            DrawTextW(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_MOUSEMOVE:
            if (!btn->m_isHovered) {
                btn->m_isHovered = true;
                InvalidateRect(hwnd, nullptr, TRUE);
                
                TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
                TrackMouseEvent(&tme);
            }
            break;
            
        case WM_MOUSELEAVE:
            btn->m_isHovered = false;
            btn->m_isPressed = false;
            InvalidateRect(hwnd, nullptr, TRUE);
            break;
            
        case WM_LBUTTONDOWN:
            btn->m_isPressed = true;
            InvalidateRect(hwnd, nullptr, TRUE);
            break;
            
        case WM_LBUTTONUP:
            btn->m_isPressed = false;
            InvalidateRect(hwnd, nullptr, TRUE);
            break;
            
        case WM_NCDESTROY:
            RemoveWindowSubclass(hwnd, SubclassProc, uIdSubclass);
            break;
    }
    
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

 
 
 

CustomProgressBar::CustomProgressBar()
    : m_hwnd(nullptr)
    , m_progress(0)
    , m_indeterminate(false)
    , m_bgColor(RGB(45, 45, 48))
    , m_fgColor(RGB(0, 122, 204))
    , m_animOffset(0)
{
}

CustomProgressBar::~CustomProgressBar() {
    if (m_hwnd) {
        RemoveWindowSubclass(m_hwnd, SubclassProc, 0);
    }
}

bool CustomProgressBar::Create(HWND parent, int id, int x, int y, int width, int height) {
    m_hwnd = CreateWindowExW(
        0,
        PROGRESS_CLASS,
        nullptr,
        WS_CHILD | WS_VISIBLE,
        x, y, width, height,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        GetModuleHandle(nullptr),
        nullptr
    );
    
    if (!m_hwnd) return false;
    
    SetWindowSubclass(m_hwnd, SubclassProc, 0, reinterpret_cast<DWORD_PTR>(this));
    
    return true;
}

void CustomProgressBar::SetProgress(int percent) {
    m_progress = std::max(0, std::min(100, percent));
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void CustomProgressBar::SetIndeterminate(bool indeterminate) {
    m_indeterminate = indeterminate;
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void CustomProgressBar::SetColors(COLORREF background, COLORREF foreground) {
    m_bgColor = background;
    m_fgColor = foreground;
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

LRESULT CALLBACK CustomProgressBar::SubclassProc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
    UINT_PTR uIdSubclass, DWORD_PTR dwRefData
) {
    auto* pb = reinterpret_cast<CustomProgressBar*>(dwRefData);
    
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            RECT rc;
            GetClientRect(hwnd, &rc);
            
             
            HBRUSH hBgBrush = CreateSolidBrush(pb->m_bgColor);
            FillRect(hdc, &rc, hBgBrush);
            DeleteObject(hBgBrush);
            
             
            if (pb->m_progress > 0 || pb->m_indeterminate) {
                RECT progressRect = rc;
                
                if (pb->m_indeterminate) {
                     
                    int barWidth = (rc.right - rc.left) / 4;
                    progressRect.left = pb->m_animOffset;
                    progressRect.right = progressRect.left + barWidth;
                } else {
                    progressRect.right = rc.left + 
                        (rc.right - rc.left) * pb->m_progress / 100;
                }
                
                HBRUSH hFgBrush = CreateSolidBrush(pb->m_fgColor);
                FillRect(hdc, &progressRect, hFgBrush);
                DeleteObject(hFgBrush);
            }
            
             
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(63, 63, 70));
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
            SelectObject(hdc, hOldBrush);
            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_NCDESTROY:
            RemoveWindowSubclass(hwnd, SubclassProc, uIdSubclass);
            break;
    }
    
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

 
 
 

ProcessListView::ProcessListView()
    : m_hwnd(nullptr)
    , m_imageList(nullptr)
    , m_colors(ColorScheme::Dark())
{
}

ProcessListView::~ProcessListView() {
    if (m_imageList) {
        ImageList_Destroy(m_imageList);
    }
    if (m_hwnd) {
        RemoveWindowSubclass(m_hwnd, SubclassProc, 0);
    }
}

bool ProcessListView::Create(HWND parent, int id, int x, int y, int width, int height) {
    m_hwnd = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        WC_LISTVIEW,
        nullptr,
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | 
        LVS_SHOWSELALWAYS | LVS_OWNERDATA,
        x, y, width, height,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        GetModuleHandle(nullptr),
        nullptr
    );
    
    if (!m_hwnd) return false;
    
     
    ListView_SetExtendedListViewStyle(m_hwnd,
        LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES);
    
     
    m_imageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 100, 50);
    ListView_SetImageList(m_hwnd, m_imageList, LVSIL_SMALL);
    
     
    LVCOLUMN lvc = { 0 };
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    
    lvc.pszText = const_cast<LPWSTR>(L"PID");
    lvc.cx = 60;
    lvc.iSubItem = 0;
    ListView_InsertColumn(m_hwnd, 0, &lvc);
    
    lvc.pszText = const_cast<LPWSTR>(L"Name");
    lvc.cx = 150;
    lvc.iSubItem = 1;
    ListView_InsertColumn(m_hwnd, 1, &lvc);
    
    lvc.pszText = const_cast<LPWSTR>(L"Arch");
    lvc.cx = 50;
    lvc.iSubItem = 2;
    ListView_InsertColumn(m_hwnd, 2, &lvc);
    
    lvc.pszText = const_cast<LPWSTR>(L"Path");
    lvc.cx = width - 280;
    lvc.iSubItem = 3;
    ListView_InsertColumn(m_hwnd, 3, &lvc);
    
     
    SetWindowSubclass(m_hwnd, SubclassProc, 0, reinterpret_cast<DWORD_PTR>(this));
    
    return true;
}

void ProcessListView::SetProcesses(const std::vector<ProcessInfo>& processes) {
    m_allProcesses = processes;
    ApplyFilter();
}

void ProcessListView::Clear() {
    m_allProcesses.clear();
    m_filteredProcesses.clear();
    ListView_SetItemCount(m_hwnd, 0);
}

ProcessInfo* ProcessListView::GetSelectedProcess() {
    int sel = GetSelectedIndex();
    if (sel >= 0 && sel < static_cast<int>(m_filteredProcesses.size())) {
        return &m_filteredProcesses[sel];
    }
    return nullptr;
}

int ProcessListView::GetSelectedIndex() const {
    return ListView_GetNextItem(m_hwnd, -1, LVNI_SELECTED);
}

void ProcessListView::SetFilter(const std::wstring& filter) {
    m_filter = filter;
    ApplyFilter();
}

void ProcessListView::SetColorScheme(const ColorScheme& scheme) {
    m_colors = scheme;
    
    ListView_SetBkColor(m_hwnd, scheme.backgroundAlt);
    ListView_SetTextBkColor(m_hwnd, scheme.backgroundAlt);
    ListView_SetTextColor(m_hwnd, scheme.foreground);
    
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void ProcessListView::Refresh() {
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void ProcessListView::ApplyFilter() {
    m_filteredProcesses.clear();
    
    if (m_filter.empty()) {
        m_filteredProcesses = m_allProcesses;
    } else {
        std::wstring lowerFilter = utils::ToLower(m_filter);
        for (const auto& proc : m_allProcesses) {
            if (utils::ContainsIgnoreCase(proc.name, m_filter) ||
                std::to_wstring(proc.pid).find(m_filter) != std::wstring::npos) {
                m_filteredProcesses.push_back(proc);
            }
        }
    }
    
    ListView_SetItemCount(m_hwnd, static_cast<int>(m_filteredProcesses.size()));
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

LRESULT CALLBACK ProcessListView::SubclassProc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
    UINT_PTR uIdSubclass, DWORD_PTR dwRefData
) {
    auto* lv = reinterpret_cast<ProcessListView*>(dwRefData);
    
    switch (msg) {
        case WM_NOTIFY: {
            auto* nmhdr = reinterpret_cast<NMHDR*>(lParam);
            
            if (nmhdr->code == LVN_GETDISPINFO) {
                auto* lvdi = reinterpret_cast<NMLVDISPINFO*>(lParam);
                int index = lvdi->item.iItem;
                
                if (index >= 0 && index < static_cast<int>(lv->m_filteredProcesses.size())) {
                    const auto& proc = lv->m_filteredProcesses[index];
                    
                    if (lvdi->item.mask & LVIF_TEXT) {
                        switch (lvdi->item.iSubItem) {
                            case 0:
                                swprintf_s(lvdi->item.pszText, lvdi->item.cchTextMax,
                                    L"%lu", proc.pid);
                                break;
                            case 1:
                                wcsncpy_s(lvdi->item.pszText, lvdi->item.cchTextMax,
                                    proc.name.c_str(), _TRUNCATE);
                                break;
                            case 2:
                                wcscpy_s(lvdi->item.pszText, lvdi->item.cchTextMax,
                                    proc.is64Bit ? L"x64" : L"x86");
                                break;
                            case 3:
                                wcsncpy_s(lvdi->item.pszText, lvdi->item.cchTextMax,
                                    proc.path.c_str(), _TRUNCATE);
                                break;
                        }
                    }
                }
            }
            break;
        }
        
        case WM_NCDESTROY:
            RemoveWindowSubclass(hwnd, SubclassProc, uIdSubclass);
            break;
    }
    
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

 
 
 

LogView::LogView()
    : m_hwnd(nullptr)
    , m_maxEntries(500)
    , m_colors(ColorScheme::Dark())
{
}

LogView::~LogView() {
    if (m_hwnd) {
        RemoveWindowSubclass(m_hwnd, SubclassProc, 0);
    }
}

bool LogView::Create(HWND parent, int id, int x, int y, int width, int height) {
    m_hwnd = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        WC_LISTBOX,
        nullptr,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOINTEGRALHEIGHT | 
        LBS_OWNERDRAWFIXED | LBS_HASSTRINGS,
        x, y, width, height,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        GetModuleHandle(nullptr),
        nullptr
    );
    
    if (!m_hwnd) return false;
    
     
    SendMessage(m_hwnd, LB_SETITEMHEIGHT, 0, 18);
    
    SetWindowSubclass(m_hwnd, SubclassProc, 0, reinterpret_cast<DWORD_PTR>(this));
    
    return true;
}

void LogView::AddEntry(LogLevel level, const std::wstring& message) {
     
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t timestamp[32];
    swprintf_s(timestamp, L"[%02d:%02d:%02d]", st.wHour, st.wMinute, st.wSecond);
    
    LogItem item;
    item.level = level;
    item.timestamp = timestamp;
    item.message = message;
    
    m_entries.push_back(item);
    
     
    while (static_cast<int>(m_entries.size()) > m_maxEntries) {
        m_entries.erase(m_entries.begin());
        ListBox_DeleteString(m_hwnd, 0);
    }
    
     
    std::wstring fullText = item.timestamp + L" " + item.message;
    ListBox_AddString(m_hwnd, fullText.c_str());
    
    ScrollToBottom();
}

void LogView::Clear() {
    m_entries.clear();
    ListBox_ResetContent(m_hwnd);
}

void LogView::SetMaxEntries(int max) {
    m_maxEntries = max;
}

void LogView::SetColorScheme(const ColorScheme& scheme) {
    m_colors = scheme;
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void LogView::ScrollToBottom() {
    int count = ListBox_GetCount(m_hwnd);
    if (count > 0) {
        ListBox_SetTopIndex(m_hwnd, count - 1);
    }
}

LRESULT CALLBACK LogView::SubclassProc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
    UINT_PTR uIdSubclass, DWORD_PTR dwRefData
) {
    auto* log = reinterpret_cast<LogView*>(dwRefData);
    
    switch (msg) {
        case WM_DRAWITEM: {
            auto* dis = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
            
            if (dis->itemID == static_cast<UINT>(-1)) break;
            
            int index = dis->itemID;
            if (index >= static_cast<int>(log->m_entries.size())) break;
            
            const auto& entry = log->m_entries[index];
            
             
            COLORREF bgColor = (dis->itemState & ODS_SELECTED) ?
                log->m_colors.accent : log->m_colors.backgroundAlt;
            HBRUSH hBrush = CreateSolidBrush(bgColor);
            FillRect(dis->hDC, &dis->rcItem, hBrush);
            DeleteObject(hBrush);
            
             
            COLORREF textColor;
            switch (entry.level) {
                case LogLevel::Debug: textColor = RGB(128, 128, 128); break;
                case LogLevel::Info: textColor = log->m_colors.foreground; break;
                case LogLevel::Warning: textColor = log->m_colors.warning; break;
                case LogLevel::Error: textColor = log->m_colors.error; break;
                default: textColor = log->m_colors.foreground; break;
            }
            
            SetBkMode(dis->hDC, TRANSPARENT);
            SetTextColor(dis->hDC, textColor);
            
             
            RECT textRect = dis->rcItem;
            textRect.left += 4;
            
            std::wstring fullText = entry.timestamp + L" " + entry.message;
            DrawTextW(dis->hDC, fullText.c_str(), -1, &textRect,
                DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
            
            return TRUE;
        }
        
        case WM_NCDESTROY:
            RemoveWindowSubclass(hwnd, SubclassProc, uIdSubclass);
            break;
    }
    
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

 
 
 

TooltipManager::TooltipManager()
    : m_hwnd(nullptr)
    , m_parent(nullptr)
{
}

TooltipManager::~TooltipManager() {
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
    }
}

bool TooltipManager::Create(HWND parent) {
    m_parent = parent;
    
    m_hwnd = CreateWindowExW(
        WS_EX_TOPMOST,
        TOOLTIPS_CLASS,
        nullptr,
        WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        parent,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );
    
    return m_hwnd != nullptr;
}

void TooltipManager::AddTooltip(HWND control, const std::wstring& text) {
    TOOLINFOW ti = { 0 };
    ti.cbSize = sizeof(ti);
    ti.hwnd = m_parent;
    ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    ti.uId = reinterpret_cast<UINT_PTR>(control);
    ti.lpszText = const_cast<LPWSTR>(text.c_str());
    
    SendMessage(m_hwnd, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti));
}

void TooltipManager::UpdateTooltip(HWND control, const std::wstring& text) {
    TOOLINFOW ti = { 0 };
    ti.cbSize = sizeof(ti);
    ti.hwnd = m_parent;
    ti.uFlags = TTF_IDISHWND;
    ti.uId = reinterpret_cast<UINT_PTR>(control);
    ti.lpszText = const_cast<LPWSTR>(text.c_str());
    
    SendMessage(m_hwnd, TTM_UPDATETIPTEXT, 0, reinterpret_cast<LPARAM>(&ti));
}

void TooltipManager::RemoveTooltip(HWND control) {
    TOOLINFOW ti = { 0 };
    ti.cbSize = sizeof(ti);
    ti.hwnd = m_parent;
    ti.uId = reinterpret_cast<UINT_PTR>(control);
    
    SendMessage(m_hwnd, TTM_DELTOOL, 0, reinterpret_cast<LPARAM>(&ti));
}

 
 
 

namespace helpers {

 
using fnSetPreferredAppMode = int(WINAPI*)(int);
using fnAllowDarkModeForWindow = bool(WINAPI*)(HWND, bool);
using fnRefreshImmersiveColorPolicyState = void(WINAPI*)();

void ApplyDarkMode(HWND hwnd, bool dark) {
    HMODULE hUxtheme = LoadLibraryW(L"uxtheme.dll");
    if (!hUxtheme) return;
    
    auto SetPreferredAppMode = reinterpret_cast<fnSetPreferredAppMode>(
        GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135)));
    auto AllowDarkModeForWindow = reinterpret_cast<fnAllowDarkModeForWindow>(
        GetProcAddress(hUxtheme, MAKEINTRESOURCEA(133)));
    auto RefreshImmersiveColorPolicyState = reinterpret_cast<fnRefreshImmersiveColorPolicyState>(
        GetProcAddress(hUxtheme, MAKEINTRESOURCEA(104)));
    
    if (SetPreferredAppMode) {
        SetPreferredAppMode(dark ? 2 : 1);   
    }
    
    if (AllowDarkModeForWindow) {
        AllowDarkModeForWindow(hwnd, dark);
    }
    
    if (RefreshImmersiveColorPolicyState) {
        RefreshImmersiveColorPolicyState();
    }
    
     
    BOOL darkMode = dark ? TRUE : FALSE;
    DwmSetWindowAttribute(hwnd, 20, &darkMode, sizeof(darkMode));
    
    FreeLibrary(hUxtheme);
}

void SetRoundedCorners(HWND hwnd, bool rounded) {
     
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#define DWMWCP_DEFAULT 0
#define DWMWCP_DONOTROUND 1
#define DWMWCP_ROUND 2
#define DWMWCP_ROUNDSMALL 3
#endif
    int preference = rounded ? DWMWCP_ROUND : DWMWCP_DONOTROUND;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, 
        &preference, sizeof(preference));
}

HFONT CreateUIFont(int size, int weight, bool italic) {
    return CreateFontW(
        -size, 0, 0, 0, weight, italic, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
    );
}

float GetDpiScale(HWND hwnd) {
    HDC hdc = GetDC(hwnd);
    int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(hwnd, hdc);
    return dpi / 96.0f;
}

int ScaleByDpi(int value, HWND hwnd) {
    return static_cast<int>(value * GetDpiScale(hwnd));
}

void CenterWindow(HWND hwnd, HWND parent) {
    RECT rc, rcParent;
    GetWindowRect(hwnd, &rc);
    
    if (parent) {
        GetWindowRect(parent, &rcParent);
    } else {
        rcParent.left = 0;
        rcParent.top = 0;
        rcParent.right = GetSystemMetrics(SM_CXSCREEN);
        rcParent.bottom = GetSystemMetrics(SM_CYSCREEN);
    }
    
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    int x = rcParent.left + (rcParent.right - rcParent.left - width) / 2;
    int y = rcParent.top + (rcParent.bottom - rcParent.top - height) / 2;
    
    SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void FlashWindow(HWND hwnd, int count) {
    FLASHWINFO fwi = { 0 };
    fwi.cbSize = sizeof(fwi);
    fwi.hwnd = hwnd;
    fwi.dwFlags = FLASHW_ALL;
    fwi.uCount = count;
    fwi.dwTimeout = 0;
    FlashWindowEx(&fwi);
}

void SetWindowTransparency(HWND hwnd, BYTE alpha) {
    SetWindowLongW(hwnd, GWL_EXSTYLE, 
        GetWindowLongW(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    SetLayeredWindowAttributes(hwnd, 0, alpha, LWA_ALPHA);
}

}  

}  
}  
