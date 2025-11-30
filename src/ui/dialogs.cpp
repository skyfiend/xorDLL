 

#include "ui/dialogs.h"
#include "ui/controls.h"
#include "core/injection_core.h"
#include "utils/string_utils.h"
#include "version.h"
#include <commctrl.h>
#include <windowsx.h>
#include <shellapi.h>
#include <cmath>

namespace xordll {
namespace ui {

 
int AboutDialog::s_animFrame = 0;
float AboutDialog::s_logoRotation = 0.0f;

 
 
 

bool SettingsDialog::Show(HWND hwndParent) {
    HWND hwnd = CreateSettingsWindow(hwndParent);
    if (!hwnd) return false;
    
     
    EnableWindow(hwndParent, FALSE);
    ShowWindow(hwnd, SW_SHOW);
    
    MSG msg;
    bool result = false;
    
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!IsWindow(hwnd)) {
            result = (msg.wParam == IDOK);
            break;
        }
        
        if (!IsDialogMessage(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    EnableWindow(hwndParent, TRUE);
    SetForegroundWindow(hwndParent);
    
    return result;
}

HWND SettingsDialog::CreateSettingsWindow(HWND parent) {
    const int WIDTH = 450;
    const int HEIGHT = 400;
    
     
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT {
        return SettingsDialog::DialogProc(hwnd, msg, wParam, lParam);
    };
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(45, 45, 48));
    wc.lpszClassName = L"xorDLLSettingsDialog";
    RegisterClassExW(&wc);
    
     
    RECT rcParent;
    GetWindowRect(parent, &rcParent);
    int x = rcParent.left + (rcParent.right - rcParent.left - WIDTH) / 2;
    int y = rcParent.top + (rcParent.bottom - rcParent.top - HEIGHT) / 2;
    
    HWND hwnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        L"xorDLLSettingsDialog",
        L"Settings",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        x, y, WIDTH, HEIGHT,
        parent,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );
    
    if (hwnd) {
        helpers::ApplyDarkMode(hwnd, true);
        InitDialog(hwnd);
    }
    
    return hwnd;
}

void SettingsDialog::InitDialog(HWND hwnd) {
    auto& settings = Settings::Instance();
    HFONT hFont = helpers::CreateUIFont(12);
    
    int y = 20;
    const int MARGIN = 20;
    const int LABEL_WIDTH = 150;
    const int CONTROL_WIDTH = 200;
    const int ROW_HEIGHT = 30;
    
     
    HWND hTitle = CreateWindowExW(0, L"STATIC", L"Application Settings",
        WS_CHILD | WS_VISIBLE,
        MARGIN, y, 400, 25, hwnd, nullptr, nullptr, nullptr);
    HFONT hTitleFont = helpers::CreateUIFont(16, FW_BOLD);
    SendMessage(hTitle, WM_SETFONT, (WPARAM)hTitleFont, TRUE);
    y += 40;
    
     
    HWND hDarkMode = CreateWindowExW(0, L"BUTTON", L"Dark Mode",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        MARGIN, y, 200, 20, hwnd, (HMENU)101, nullptr, nullptr);
    SendMessage(hDarkMode, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hDarkMode, BM_SETCHECK, settings.GetDarkMode() ? BST_CHECKED : BST_UNCHECKED, 0);
    y += ROW_HEIGHT;
    
     
    HWND hAutoRefresh = CreateWindowExW(0, L"BUTTON", L"Auto-refresh process list",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        MARGIN, y, 200, 20, hwnd, (HMENU)102, nullptr, nullptr);
    SendMessage(hAutoRefresh, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hAutoRefresh, BM_SETCHECK, settings.GetAutoRefresh() ? BST_CHECKED : BST_UNCHECKED, 0);
    y += ROW_HEIGHT;
    
     
    CreateWindowExW(0, L"STATIC", L"Refresh interval (ms):",
        WS_CHILD | WS_VISIBLE,
        MARGIN, y + 2, LABEL_WIDTH, 20, hwnd, nullptr, nullptr, nullptr);
    HWND hInterval = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", 
        std::to_wstring(settings.GetRefreshInterval()).c_str(),
        WS_CHILD | WS_VISIBLE | ES_NUMBER,
        MARGIN + LABEL_WIDTH, y, 80, 22, hwnd, (HMENU)103, nullptr, nullptr);
    SendMessage(hInterval, WM_SETFONT, (WPARAM)hFont, TRUE);
    y += ROW_HEIGHT;
    
     
    HWND hConfirm = CreateWindowExW(0, L"BUTTON", L"Confirm before injection",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        MARGIN, y, 200, 20, hwnd, (HMENU)104, nullptr, nullptr);
    SendMessage(hConfirm, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hConfirm, BM_SETCHECK, settings.GetConfirmInjection() ? BST_CHECKED : BST_UNCHECKED, 0);
    y += ROW_HEIGHT;
    
     
    CreateWindowExW(0, L"STATIC", L"Default method:",
        WS_CHILD | WS_VISIBLE,
        MARGIN, y + 2, LABEL_WIDTH, 20, hwnd, nullptr, nullptr, nullptr);
    HWND hMethod = CreateWindowExW(0, L"COMBOBOX", nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        MARGIN + LABEL_WIDTH, y, CONTROL_WIDTH, 200, hwnd, (HMENU)105, nullptr, nullptr);
    SendMessage(hMethod, WM_SETFONT, (WPARAM)hFont, TRUE);
    
     
    ComboBox_AddString(hMethod, L"CreateRemoteThread");
    ComboBox_AddString(hMethod, L"NtCreateThreadEx");
    ComboBox_AddString(hMethod, L"QueueUserAPC");
    ComboBox_SetCurSel(hMethod, static_cast<int>(settings.GetDefaultMethod()));
    y += ROW_HEIGHT;
    
     
    HWND hShowSystem = CreateWindowExW(0, L"BUTTON", L"Show system processes",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        MARGIN, y, 200, 20, hwnd, (HMENU)106, nullptr, nullptr);
    SendMessage(hShowSystem, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hShowSystem, BM_SETCHECK, settings.GetShowSystemProcesses() ? BST_CHECKED : BST_UNCHECKED, 0);
    y += ROW_HEIGHT;
    
     
    HWND hLogFile = CreateWindowExW(0, L"BUTTON", L"Save logs to file",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        MARGIN, y, 200, 20, hwnd, (HMENU)107, nullptr, nullptr);
    SendMessage(hLogFile, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hLogFile, BM_SETCHECK, settings.GetLogToFile() ? BST_CHECKED : BST_UNCHECKED, 0);
    y += ROW_HEIGHT + 20;
    
     
    CreateWindowExW(0, L"STATIC", nullptr,
        WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
        MARGIN, y, 400, 2, hwnd, nullptr, nullptr, nullptr);
    y += 15;
    
     
    HWND hOk = CreateWindowExW(0, L"BUTTON", L"OK",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        230, y, 90, 28, hwnd, (HMENU)IDOK, nullptr, nullptr);
    SendMessage(hOk, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    HWND hCancel = CreateWindowExW(0, L"BUTTON", L"Cancel",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        330, y, 90, 28, hwnd, (HMENU)IDCANCEL, nullptr, nullptr);
    SendMessage(hCancel, WM_SETFONT, (WPARAM)hFont, TRUE);
}

INT_PTR CALLBACK SettingsDialog::DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND:
            OnCommand(hwnd, wParam);
            return TRUE;
            
        case WM_CLOSE:
            DestroyWindow(hwnd);
            PostMessage(nullptr, WM_NULL, IDCANCEL, 0);
            return TRUE;
            
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORBTN: {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, RGB(220, 220, 220));
            SetBkColor(hdc, RGB(45, 45, 48));
            static HBRUSH hBrush = CreateSolidBrush(RGB(45, 45, 48));
            return (INT_PTR)hBrush;
        }
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void SettingsDialog::OnCommand(HWND hwnd, WPARAM wParam) {
    switch (LOWORD(wParam)) {
        case IDOK:
            SaveSettings(hwnd);
            DestroyWindow(hwnd);
            PostMessage(nullptr, WM_NULL, IDOK, 0);
            break;
            
        case IDCANCEL:
            DestroyWindow(hwnd);
            PostMessage(nullptr, WM_NULL, IDCANCEL, 0);
            break;
    }
}

void SettingsDialog::SaveSettings(HWND hwnd) {
    auto& settings = Settings::Instance();
    
    settings.SetDarkMode(IsDlgButtonChecked(hwnd, 101) == BST_CHECKED);
    settings.SetAutoRefresh(IsDlgButtonChecked(hwnd, 102) == BST_CHECKED);
    
    wchar_t buffer[32];
    GetDlgItemTextW(hwnd, 103, buffer, 32);
    settings.SetRefreshInterval(_wtoi(buffer));
    
    settings.SetConfirmInjection(IsDlgButtonChecked(hwnd, 104) == BST_CHECKED);
    
    HWND hMethod = GetDlgItem(hwnd, 105);
    settings.SetDefaultMethod(static_cast<InjectionMethod>(ComboBox_GetCurSel(hMethod)));
    
    settings.SetShowSystemProcesses(IsDlgButtonChecked(hwnd, 106) == BST_CHECKED);
    settings.SetLogToFile(IsDlgButtonChecked(hwnd, 107) == BST_CHECKED);
    
    settings.Save();
}

 
 
 

void AboutDialog::Show(HWND hwndParent) {
    const int WIDTH = 400;
    const int HEIGHT = 350;
    
     
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT {
        return AboutDialog::DialogProc(hwnd, msg, wParam, lParam);
    };
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
    wc.lpszClassName = L"xorDLLAboutDialog";
    RegisterClassExW(&wc);
    
     
    RECT rcParent;
    GetWindowRect(hwndParent, &rcParent);
    int x = rcParent.left + (rcParent.right - rcParent.left - WIDTH) / 2;
    int y = rcParent.top + (rcParent.bottom - rcParent.top - HEIGHT) / 2;
    
    HWND hwnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        L"xorDLLAboutDialog",
        L"About xorDLL",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        x, y, WIDTH, HEIGHT,
        hwndParent,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );
    
    if (!hwnd) return;
    
     
    s_animFrame = 0;
    s_logoRotation = 0.0f;
    SetTimer(hwnd, 1, 50, nullptr);
    
     
    EnableWindow(hwndParent, FALSE);
    ShowWindow(hwnd, SW_SHOW);
    
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!IsWindow(hwnd)) break;
        
        if (!IsDialogMessage(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    EnableWindow(hwndParent, TRUE);
    SetForegroundWindow(hwndParent);
}

INT_PTR CALLBACK AboutDialog::DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT:
            OnPaint(hwnd);
            return 0;
            
        case WM_TIMER:
            OnTimer(hwnd);
            return 0;
            
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                KillTimer(hwnd, 1);
                DestroyWindow(hwnd);
            } else if (LOWORD(wParam) == 1001) {
                 
                ShellExecuteW(hwnd, L"open", XORDLL_APP_WEBSITE, nullptr, nullptr, SW_SHOWNORMAL);
            }
            return TRUE;
            
        case WM_CLOSE:
            KillTimer(hwnd, 1);
            DestroyWindow(hwnd);
            return TRUE;
            
        case WM_CREATE: {
            HFONT hFont = helpers::CreateUIFont(12);
            
             
            HWND hGithub = CreateWindowExW(0, L"BUTTON", L"Visit GitHub",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                100, 260, 100, 28, hwnd, (HMENU)1001, nullptr, nullptr);
            SendMessage(hGithub, WM_SETFONT, (WPARAM)hFont, TRUE);
            
             
            HWND hOk = CreateWindowExW(0, L"BUTTON", L"OK",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | BS_PUSHBUTTON,
                210, 260, 90, 28, hwnd, (HMENU)IDOK, nullptr, nullptr);
            SendMessage(hOk, WM_SETFONT, (WPARAM)hFont, TRUE);
            return 0;
        }
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void AboutDialog::OnPaint(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    
    RECT rc;
    GetClientRect(hwnd, &rc);
    
     
    FillRect(hdc, &rc, GetSysColorBrush(COLOR_BTNFACE));
    
    SetBkMode(hdc, TRANSPARENT);
    
     
    int logoY = 30;
    int pulseSize = static_cast<int>(5 * sin(s_logoRotation));
    
     
    HFONT hLogoFont = helpers::CreateUIFont(36 + pulseSize, FW_BOLD);
    SelectObject(hdc, hLogoFont);
    
     
    SetTextColor(hdc, RGB(0, 120, 215));  
    
    RECT logoRect = { 0, logoY, rc.right, logoY + 50 };
    DrawTextW(hdc, L"xorDLL", -1, &logoRect, DT_CENTER | DT_SINGLELINE);
    DeleteObject(hLogoFont);
    
     
    HFONT hVersionFont = helpers::CreateUIFont(14);
    SelectObject(hdc, hVersionFont);
    SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
    
    RECT versionRect = { 0, logoY + 55, rc.right, logoY + 75 };
    std::wstring version = L"Version 1.0.0";
    DrawTextW(hdc, version.c_str(), -1, &versionRect, DT_CENTER | DT_SINGLELINE);
    DeleteObject(hVersionFont);
    
     
    HFONT hDescFont = helpers::CreateUIFont(12);
    SelectObject(hdc, hDescFont);
    SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
    
    RECT descRect = { 20, logoY + 100, rc.right - 20, logoY + 140 };
    std::wstring desc = std::wstring(XORDLL_APP_DESCRIPTION) + L"\n\nDeveloper: skyfiend";
    DrawTextW(hdc, desc.c_str(), -1, &descRect, DT_CENTER | DT_WORDBREAK);
    
     
    SetTextColor(hdc, GetSysColor(COLOR_GRAYTEXT));
    RECT copyRect = { 20, logoY + 160, rc.right - 20, logoY + 180 };
    DrawTextW(hdc, L"Copyright © 2025 skyfiend", -1, &copyRect, DT_CENTER | DT_SINGLELINE);
    
     
    RECT licRect = { 20, logoY + 185, rc.right - 20, logoY + 205 };
    DrawTextW(hdc, L"Licensed under MIT License", -1, &licRect, DT_CENTER | DT_SINGLELINE);
    
    DeleteObject(hDescFont);
    
    EndPaint(hwnd, &ps);
}

void AboutDialog::OnTimer(HWND hwnd) {
    s_animFrame++;
    s_logoRotation += 0.1f;
    
     
    RECT logoRect = { 0, 20, 400, 100 };
    InvalidateRect(hwnd, &logoRect, FALSE);
}

 
 
 

ProgressDialog::ProgressDialog()
    : m_hwnd(nullptr)
    , m_hwndProgress(nullptr)
    , m_hwndStatus(nullptr)
    , m_cancelled(false)
{
}

ProgressDialog::~ProgressDialog() {
    Close();
}

bool ProgressDialog::Create(HWND hwndParent, const std::wstring& title) {
    const int WIDTH = 350;
    const int HEIGHT = 120;
    
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT {
        auto* dlg = reinterpret_cast<ProgressDialog*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        return ProgressDialog::DialogProc(hwnd, msg, wParam, lParam);
    };
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(45, 45, 48));
    wc.lpszClassName = L"xorDLLProgressDialog";
    RegisterClassExW(&wc);
    
    RECT rcParent;
    GetWindowRect(hwndParent, &rcParent);
    int x = rcParent.left + (rcParent.right - rcParent.left - WIDTH) / 2;
    int y = rcParent.top + (rcParent.bottom - rcParent.top - HEIGHT) / 2;
    
    m_hwnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        L"xorDLLProgressDialog",
        title.c_str(),
        WS_POPUP | WS_CAPTION,
        x, y, WIDTH, HEIGHT,
        hwndParent,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );
    
    if (!m_hwnd) return false;
    
    SetWindowLongPtr(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    helpers::ApplyDarkMode(m_hwnd, true);
    
    HFONT hFont = helpers::CreateUIFont(12);
    
     
    m_hwndStatus = CreateWindowExW(0, L"STATIC", L"Initializing...",
        WS_CHILD | WS_VISIBLE,
        20, 15, WIDTH - 40, 20, m_hwnd, nullptr, nullptr, nullptr);
    SendMessage(m_hwndStatus, WM_SETFONT, (WPARAM)hFont, TRUE);
    
     
    m_hwndProgress = CreateWindowExW(0, PROGRESS_CLASS, nullptr,
        WS_CHILD | WS_VISIBLE,
        20, 45, WIDTH - 40, 20, m_hwnd, nullptr, nullptr, nullptr);
    SendMessage(m_hwndProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    
    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);
    
    return true;
}

void ProgressDialog::Close() {
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

void ProgressDialog::SetProgress(int percent) {
    if (m_hwndProgress) {
        SendMessage(m_hwndProgress, PBM_SETPOS, percent, 0);
    }
}

void ProgressDialog::SetStatus(const std::wstring& status) {
    if (m_hwndStatus) {
        SetWindowTextW(m_hwndStatus, status.c_str());
    }
}

INT_PTR CALLBACK ProgressDialog::DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, RGB(220, 220, 220));
            SetBkColor(hdc, RGB(45, 45, 48));
            static HBRUSH hBrush = CreateSolidBrush(RGB(45, 45, 48));
            return (INT_PTR)hBrush;
        }
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

 
 
 

CustomMessageBox::Result CustomMessageBox::Show(
    HWND hwndParent,
    const std::wstring& title,
    const std::wstring& message,
    Type type,
    Buttons buttons
) {
    UINT mbType = MB_OK;
    
    switch (type) {
        case Type::Info: mbType |= MB_ICONINFORMATION; break;
        case Type::Warning: mbType |= MB_ICONWARNING; break;
        case Type::Error: mbType |= MB_ICONERROR; break;
        case Type::Question: mbType |= MB_ICONQUESTION; break;
    }
    
    switch (buttons) {
        case Buttons::Ok: mbType |= MB_OK; break;
        case Buttons::OkCancel: mbType |= MB_OKCANCEL; break;
        case Buttons::YesNo: mbType |= MB_YESNO; break;
        case Buttons::YesNoCancel: mbType |= MB_YESNOCANCEL; break;
    }
    
    int result = MessageBoxW(hwndParent, message.c_str(), title.c_str(), mbType);
    
    switch (result) {
        case IDOK: return Result::Ok;
        case IDCANCEL: return Result::Cancel;
        case IDYES: return Result::Yes;
        case IDNO: return Result::No;
        default: return Result::Cancel;
    }
}

}  
}  
