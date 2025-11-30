 

#include "ui/main_window.h"
#include "ui/controls.h"
#include "ui/dialogs.h"
#include "utils/logger.h"
#include "utils/string_utils.h"
#include "utils/file_utils.h"
#include "utils/settings.h"
#include "version.h"
#include <windowsx.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <shellapi.h>
#include <gdiplus.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "gdiplus.lib")

namespace xordll {

 
enum IMMERSIVE_HC_CACHE_MODE {
    IHCM_USE_CACHED_VALUE,
    IHCM_REFRESH
};

enum PreferredAppMode {
    Default,
    AllowDark,
    ForceDark,
    ForceLight,
    Max
};

using fnSetPreferredAppMode = PreferredAppMode(WINAPI*)(PreferredAppMode appMode);
using fnAllowDarkModeForWindow = bool(WINAPI*)(HWND hWnd, bool allow);
using fnRefreshImmersiveColorPolicyState = void(WINAPI*)();

 
 
 

MainWindow::MainWindow()
    : m_hwnd(nullptr)
    , m_hwndProcessList(nullptr)
    , m_hwndProcessSearch(nullptr)
    , m_hwndDllList(nullptr)
    , m_hwndBtnDllAdd(nullptr)
    , m_hwndBtnDllRemove(nullptr)
    , m_hwndMethodCombo(nullptr)
    , m_hwndBtnInject(nullptr)
    , m_hwndBtnEject(nullptr)
    , m_hwndBtnRefresh(nullptr)
    , m_hwndSelectedProc(nullptr)
    , m_hwndStatusBar(nullptr)
    , m_hMenu(nullptr)
    , m_hInstance(nullptr)
    , m_colorScheme(ColorScheme::Dark())
    , m_lastInjectedModule(nullptr)
    , m_lastInjectedPid(0)
    , m_selectedPid(0)
    , m_hBackgroundBrush(nullptr)
    , m_hFont(nullptr)
    , m_hFontBold(nullptr)
    , m_hImageList(nullptr)
    , m_hBgImage(nullptr)
    , m_bgWidth(0)
    , m_bgHeight(0)
    , m_alwaysOnTop(false)
    , m_minimizedToTray(false)
{
    m_processManager = std::make_unique<ProcessManager>();
    m_injectionCore = std::make_unique<InjectionCore>();
    m_hotkeyManager = std::make_unique<ui::HotkeyManager>();
    m_trayIcon = std::make_unique<ui::TrayIcon>();
    m_acceleratorManager = std::make_unique<ui::AcceleratorManager>();
}

MainWindow::~MainWindow()
{
    if (m_hBackgroundBrush) DeleteObject(m_hBackgroundBrush);
    if (m_hFont) DeleteObject(m_hFont);
    if (m_hFontBold) DeleteObject(m_hFontBold);
    if (m_hMenu) DestroyMenu(m_hMenu);
    if (m_hBgImage) DeleteObject(m_hBgImage);
}

bool MainWindow::Create(HINSTANCE hInstance, int nCmdShow)
{
    m_hInstance = hInstance;
    
    if (!RegisterWindowClass(hInstance)) {
        LOG_ERROR(L"Failed to register window class");
        return false;
    }
    
     
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenWidth - WINDOW_WIDTH) / 2;
    int y = (screenHeight - WINDOW_HEIGHT) / 2;
    
     
    m_hwnd = CreateWindowExW(
        WS_EX_ACCEPTFILES,
        WINDOW_CLASS_NAME,
        XORDLL_APP_NAME,
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
        x, y, WINDOW_WIDTH, WINDOW_HEIGHT,
        nullptr,
        nullptr,
        hInstance,
        this
    );
    
    if (!m_hwnd) {
        LOG_WIN_ERROR(L"CreateWindowEx");
        return false;
    }
    
    ShowWindow(m_hwnd, nCmdShow);
    UpdateWindow(m_hwnd);
    
    return true;
}

int MainWindow::Run()
{
    MSG msg = { 0 };
    
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return static_cast<int>(msg.wParam);
}

void MainWindow::SetColorScheme(const ColorScheme& scheme)
{
    m_colorScheme = scheme;
    
    if (m_hBackgroundBrush) {
        DeleteObject(m_hBackgroundBrush);
    }
    m_hBackgroundBrush = CreateSolidBrush(m_colorScheme.background);
    
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void MainWindow::AddLogEntry(LogLevel level, const std::wstring& message)
{
     
    SetStatusText(message, 0);
}

void MainWindow::SetStatusText(const std::wstring& text, int part)
{
     
    if (part == 0 && !text.empty()) {
        std::wstring title = std::wstring(XORDLL_APP_NAME) + L" - " + text;
        SetWindowTextW(m_hwnd, title.c_str());
    }
}

 
 
 

LRESULT CALLBACK MainWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    MainWindow* pThis = nullptr;
    
    if (msg == WM_NCCREATE) {
        auto pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = static_cast<MainWindow*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hwnd = hwnd;
    } else {
        pThis = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    
    if (pThis) {
        return pThis->HandleMessage(msg, wParam, lParam);
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT MainWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_CREATE:
            OnCreate();
            return 0;
            
        case WM_DESTROY:
            OnDestroy();
            return 0;
            
        case WM_PAINT:
            OnPaint();
            return 0;
            
        case WM_COMMAND:
            OnCommand(wParam, lParam);
            return 0;
            
        case WM_NOTIFY:
            OnNotify(wParam, lParam);
            return 0;
            
        case WM_TIMER:
            OnTimer(wParam);
            return 0;
            
        case WM_DROPFILES:
            OnDropFiles(reinterpret_cast<HDROP>(wParam));
            return 0;
            
        case WM_HOTKEY:
            OnHotkey(wParam);
            return 0;
            
        case WM_SIZE:
            if (wParam == SIZE_MINIMIZED) {
                OnMinimize();
            }
            OnSize(LOWORD(lParam), HIWORD(lParam));
            return 0;
            
        case ui::WM_TRAYICON:
            OnTrayIcon(lParam);
            return 0;
            
         
            
        case WM_ERASEBKGND:
            return 1;   
    }
    
    return DefWindowProc(m_hwnd, msg, wParam, lParam);
}

 
 
 

bool MainWindow::RegisterWindowClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;   
    wc.lpszClassName = WINDOW_CLASS_NAME;
    
     
    wc.hIcon = LoadIconW(hInstance, L"IDI_APPICON");
    wc.hIconSm = LoadIconW(hInstance, L"IDI_APPICON");
    if (!wc.hIcon) {
        wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
        wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
    }
    
    return RegisterClassExW(&wc) != 0;
}

void MainWindow::CreateControls()
{
     
    NONCLIENTMETRICS ncm = { sizeof(ncm) };
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
    m_hFont = CreateFontIndirect(&ncm.lfMessageFont);
    
     
    m_hBackgroundBrush = GetSysColorBrush(COLOR_BTNFACE);
    
    int y = MARGIN;
    int clientWidth = WINDOW_WIDTH - 2 * MARGIN - 16;
    int leftWidth = (int)(clientWidth * 0.48);
    int rightWidth = clientWidth - leftWidth - MARGIN;
    
     
    CreateWindowExW(0, L"STATIC", L"Processes",
        WS_CHILD | WS_VISIBLE,
        MARGIN, y, 100, 18,
        m_hwnd, nullptr, m_hInstance, nullptr);
    
     
    CreateWindowExW(0, L"STATIC", L"DLL Files",
        WS_CHILD | WS_VISIBLE,
        MARGIN + leftWidth + MARGIN, y, 100, 18,
        m_hwnd, nullptr, m_hInstance, nullptr);
    
    y += 20;

     
    m_hwndProcessSearch = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        MARGIN, y, leftWidth - 65, CONTROL_HEIGHT,
        m_hwnd, reinterpret_cast<HMENU>(ID_PROCESS_SEARCH), m_hInstance, nullptr);
    SendMessage(m_hwndProcessSearch, EM_SETCUEBANNER, 0, (LPARAM)L"Search...");

     
    m_hwndBtnRefresh = CreateWindowExW(0, L"BUTTON", L"Refresh",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        MARGIN + leftWidth - 60, y, 60, CONTROL_HEIGHT,
        m_hwnd, reinterpret_cast<HMENU>(ID_BTN_REFRESH), m_hInstance, nullptr);

     
    m_hwndBtnDllAdd = CreateWindowExW(0, L"BUTTON", L"Add...",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        MARGIN + leftWidth + MARGIN, y, 60, CONTROL_HEIGHT,
        m_hwnd, reinterpret_cast<HMENU>(ID_DLL_ADD), m_hInstance, nullptr);

    m_hwndBtnDllRemove = CreateWindowExW(0, L"BUTTON", L"Remove",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        MARGIN + leftWidth + MARGIN + 65, y, 60, CONTROL_HEIGHT,
        m_hwnd, reinterpret_cast<HMENU>(ID_DLL_REMOVE), m_hInstance, nullptr);

    
    y += CONTROL_HEIGHT + 6;
    
    int listHeight = 180;
    
     
    m_hwndProcessList = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEW, L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_NOCOLUMNHEADER,
        MARGIN, y, leftWidth, listHeight,
        m_hwnd, reinterpret_cast<HMENU>(ID_PROCESS_LIST), m_hInstance, nullptr);
    
    m_hImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 50, 20);
    ListView_SetImageList(m_hwndProcessList, m_hImageList, LVSIL_SMALL);
    ListView_SetExtendedListViewStyle(m_hwndProcessList, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    
    LVCOLUMN lvc = { 0 };
    lvc.mask = LVCF_WIDTH;
    lvc.cx = leftWidth - 20;
    ListView_InsertColumn(m_hwndProcessList, 0, &lvc);
    
     
    m_hwndDllList = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
        MARGIN + leftWidth + MARGIN, y, rightWidth, listHeight,
        m_hwnd, reinterpret_cast<HMENU>(ID_DLL_LIST), m_hInstance, nullptr);
    
    y += listHeight + MARGIN;
    
     
    
    CreateWindowExW(0, L"STATIC", L"Target:",
        WS_CHILD | WS_VISIBLE,
        MARGIN, y + 4, 45, CONTROL_HEIGHT,
        m_hwnd, nullptr, m_hInstance, nullptr);
    
    m_hwndSelectedProc = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"(no process selected)",
        WS_CHILD | WS_VISIBLE | ES_READONLY | ES_AUTOHSCROLL,
        MARGIN + 48, y, leftWidth - 48, CONTROL_HEIGHT,
        m_hwnd, nullptr, m_hInstance, nullptr);
    
    y += CONTROL_HEIGHT + 8;
    
    CreateWindowExW(0, L"STATIC", L"Method:",
        WS_CHILD | WS_VISIBLE,
        MARGIN, y + 4, 50, CONTROL_HEIGHT,
        m_hwnd, nullptr, m_hInstance, nullptr);
    
    m_hwndMethodCombo = CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS,
        MARGIN + 55, y, 160, 200,
        m_hwnd, reinterpret_cast<HMENU>(ID_METHOD_COMBO), m_hInstance, nullptr);
    
    auto methods = m_injectionCore->GetAvailableMethods();
    for (auto method : methods) {
        ComboBox_AddString(m_hwndMethodCombo, InjectionCore::GetMethodName(method).c_str());
    }
    ComboBox_SetCurSel(m_hwndMethodCombo, 0);
    
     
    m_hwndBtnInject = CreateWindowExW(0, L"BUTTON", L"Inject",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        MARGIN + clientWidth - 2 * BUTTON_WIDTH - 10, y, BUTTON_WIDTH, CONTROL_HEIGHT,
        m_hwnd, reinterpret_cast<HMENU>(ID_BTN_INJECT), m_hInstance, nullptr);
    
    m_hwndBtnEject = CreateWindowExW(0, L"BUTTON", L"Eject",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_DISABLED,
        MARGIN + clientWidth - BUTTON_WIDTH, y, BUTTON_WIDTH, CONTROL_HEIGHT,
        m_hwnd, reinterpret_cast<HMENU>(ID_BTN_EJECT), m_hInstance, nullptr);
    
     
    EnumChildWindows(m_hwnd, [](HWND hwnd, LPARAM lParam) -> BOOL {
        SendMessage(hwnd, WM_SETFONT, lParam, TRUE);
        return TRUE;
    }, reinterpret_cast<LPARAM>(m_hFont));
}

void MainWindow::CreateMenu()
{
    m_hMenu = ::CreateMenu();
    
     
    HMENU hFileMenu = CreatePopupMenu();
    AppendMenuW(hFileMenu, MF_STRING, ID_MENU_FILE_OPEN_DLL, L"&Open DLL...\tCtrl+O");
    AppendMenuW(hFileMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hFileMenu, MF_STRING, ID_MENU_FILE_EXIT, L"E&xit\tAlt+F4");
    AppendMenuW(m_hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hFileMenu), L"&File");
    
     
    HMENU hInjectMenu = CreatePopupMenu();
    AppendMenuW(hInjectMenu, MF_STRING, ID_MENU_INJECT_START, L"&Inject\tF5");
    AppendMenuW(hInjectMenu, MF_STRING, ID_MENU_INJECT_EJECT, L"&Eject\tF6");
    AppendMenuW(m_hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hInjectMenu), L"&Injection");
    
     
    HMENU hOptionsMenu = CreatePopupMenu();
    AppendMenuW(hOptionsMenu, MF_STRING, ID_MENU_OPTIONS_ALWAYS_TOP, L"&Always on Top");
    AppendMenuW(hOptionsMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hOptionsMenu, MF_STRING, ID_MENU_OPTIONS_MINIMIZE_TRAY, L"&Minimize to Tray");
    AppendMenuW(hOptionsMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hOptionsMenu, MF_STRING, ID_MENU_OPTIONS_SETTINGS, L"&Settings...\tCtrl+,");
    AppendMenuW(m_hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hOptionsMenu), L"&Options");
    
     
    HMENU hHelpMenu = CreatePopupMenu();
    AppendMenuW(hHelpMenu, MF_STRING, ID_MENU_HELP_SHORTCUTS, L"&Keyboard Shortcuts\tF1");
    AppendMenuW(hHelpMenu, MF_STRING, ID_MENU_HELP_GITHUB, L"&GitHub Repository");
    AppendMenuW(hHelpMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hHelpMenu, MF_STRING, ID_MENU_HELP_ABOUT, L"&About xorDLL...");
    AppendMenuW(m_hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hHelpMenu), L"&Help");
    
    SetMenu(m_hwnd, m_hMenu);
}

void MainWindow::ApplyDarkMode()
{
     
    HMODULE hUxtheme = LoadLibraryW(L"uxtheme.dll");
    if (hUxtheme) {
        auto SetPreferredAppMode = reinterpret_cast<fnSetPreferredAppMode>(
            GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135)));
        auto AllowDarkModeForWindow = reinterpret_cast<fnAllowDarkModeForWindow>(
            GetProcAddress(hUxtheme, MAKEINTRESOURCEA(133)));
        auto RefreshImmersiveColorPolicyState = reinterpret_cast<fnRefreshImmersiveColorPolicyState>(
            GetProcAddress(hUxtheme, MAKEINTRESOURCEA(104)));
        
        if (SetPreferredAppMode) {
            SetPreferredAppMode(m_settings.darkMode ? ForceDark : ForceLight);
        }
        
        if (AllowDarkModeForWindow) {
            AllowDarkModeForWindow(m_hwnd, m_settings.darkMode);
        }
        
        if (RefreshImmersiveColorPolicyState) {
            RefreshImmersiveColorPolicyState();
        }
        
        FreeLibrary(hUxtheme);
    }
    
     
    BOOL darkMode = m_settings.darkMode ? TRUE : FALSE;
    DwmSetWindowAttribute(m_hwnd, 20, &darkMode, sizeof(darkMode));   
    
     
    SetColorScheme(m_settings.darkMode ? ColorScheme::Dark() : ColorScheme::Light());
}

void MainWindow::LoadSettings()
{
    Settings::Instance().Load();
    m_settings.darkMode = Settings::Instance().GetDarkMode();
    m_settings.autoRefresh = Settings::Instance().GetAutoRefresh();
    m_settings.refreshInterval = Settings::Instance().GetRefreshInterval();
}

void MainWindow::LoadBackgroundImage()
{
     
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
    
     
    HRSRC hRes = FindResourceW(m_hInstance, L"IDB_BACKGROUND", RT_RCDATA);
    if (!hRes) return;
    
    HGLOBAL hData = LoadResource(m_hInstance, hRes);
    if (!hData) return;
    
    void* pData = LockResource(hData);
    DWORD dataSize = SizeofResource(m_hInstance, hRes);
    if (!pData || !dataSize) return;
    
     
    IStream* pStream = nullptr;
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, dataSize);
    if (hMem) {
        void* pMem = GlobalLock(hMem);
        memcpy(pMem, pData, dataSize);
        GlobalUnlock(hMem);
        CreateStreamOnHGlobal(hMem, TRUE, &pStream);
    }
    
    if (pStream) {
        Gdiplus::Bitmap* pBitmap = Gdiplus::Bitmap::FromStream(pStream);
        if (pBitmap && pBitmap->GetLastStatus() == Gdiplus::Ok) {
            m_bgWidth = pBitmap->GetWidth();
            m_bgHeight = pBitmap->GetHeight();
            pBitmap->GetHBITMAP(Gdiplus::Color(0, 0, 0, 0), &m_hBgImage);
            delete pBitmap;
        }
        pStream->Release();
    }
}

void MainWindow::DrawCustomButton(DRAWITEMSTRUCT* dis)
{
     
    HDC hdcDest = dis->hDC;
    int w = dis->rcItem.right - dis->rcItem.left;
    int h = dis->rcItem.bottom - dis->rcItem.top;
    
    HDC hdc = CreateCompatibleDC(hdcDest);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcDest, w, h);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdc, hBitmap);
    
     
    Gdiplus::Graphics g(hdc);
    g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);
    
    bool isPressed = (dis->itemState & ODS_SELECTED);
    bool isDisabled = (dis->itemState & ODS_DISABLED);
    bool isInject = (dis->CtlID == ID_BTN_INJECT);
    
     
    g.Clear(Gdiplus::Color(20, 30, 25));
    
     
    Gdiplus::Color normalColor, hoverColor, pressColor, borderColor, textColor;
    
    if (isInject) {
         
        normalColor = Gdiplus::Color(40, 100, 50);
        pressColor = Gdiplus::Color(30, 80, 40);
        borderColor = Gdiplus::Color(60, 180, 80);
        textColor = Gdiplus::Color(255, 255, 255);
    } else {
         
        normalColor = Gdiplus::Color(35, 45, 40);
        pressColor = Gdiplus::Color(25, 35, 30);
        borderColor = Gdiplus::Color(60, 80, 70);
        textColor = Gdiplus::Color(200, 220, 200);
    }
    
    if (isDisabled) {
        normalColor = Gdiplus::Color(30, 35, 32);
        borderColor = Gdiplus::Color(50, 55, 52);
        textColor = Gdiplus::Color(100, 110, 100);
    }
    
     
    Gdiplus::Rect rect(0, 0, w-1, h-1);
    Gdiplus::SolidBrush brush(isPressed ? pressColor : normalColor);
    g.FillRectangle(&brush, rect);
    
     
    Gdiplus::Pen pen(borderColor, 1.0f);
    g.DrawRectangle(&pen, rect);
    
     
    wchar_t text[256];
    GetWindowTextW(dis->hwndItem, text, 256);
    
    Gdiplus::Font font(L"Segoe UI", 9, Gdiplus::FontStyleRegular);
    Gdiplus::SolidBrush textBrush(textColor);
    Gdiplus::StringFormat format;
    format.SetAlignment(Gdiplus::StringAlignmentCenter);
    format.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    
    Gdiplus::RectF rectF(0, 0, (float)w, (float)h);
    g.DrawString(text, -1, &font, rectF, &format, &textBrush);
    
     
    BitBlt(hdcDest, 0, 0, w, h, hdc, 0, 0, SRCCOPY);
    
     
    SelectObject(hdc, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdc);
}

void MainWindow::SaveSettings()
{
    Settings::Instance().SetDarkMode(m_settings.darkMode);
    Settings::Instance().SetAutoRefresh(m_settings.autoRefresh);
    Settings::Instance().SetRefreshInterval(m_settings.refreshInterval);
    Settings::Instance().Save();
}

void MainWindow::ShowSettingsDialog()
{
    if (ui::SettingsDialog::Show(m_hwnd)) {
         
        LoadSettings();
        ApplyDarkMode();
        
         
        KillTimer(m_hwnd, ID_TIMER_REFRESH);
        if (m_settings.autoRefresh) {
            SetTimer(m_hwnd, ID_TIMER_REFRESH, m_settings.refreshInterval, nullptr);
        }
    }
}

 
 
 

void MainWindow::OnCreate()
{
    LoadSettings();
    CreateControls();
    CreateMenu();
    SetMenu(m_hwnd, m_hMenu);
    
     
    m_hotkeyManager->Initialize(m_hwnd);
    m_hotkeyManager->RegisterDefaults();
    
     
    m_hotkeyManager->SetCallback(ui::HotkeyId::Inject, [this]() { PerformInjection(); });
    m_hotkeyManager->SetCallback(ui::HotkeyId::Eject, [this]() { PerformEjection(); });
    m_hotkeyManager->SetCallback(ui::HotkeyId::Refresh, [this]() { RefreshProcessList(); });
    m_hotkeyManager->SetCallback(ui::HotkeyId::OpenDll, [this]() { AddDllToList(); });
    m_hotkeyManager->SetCallback(ui::HotkeyId::Settings, [this]() { ShowSettingsDialog(); });
    m_hotkeyManager->SetCallback(ui::HotkeyId::FocusSearch, [this]() { 
        SetFocus(m_hwndProcessSearch); 
    });
    
     
    m_acceleratorManager->Create();
    
     
    Logger::Instance().SetUICallback([this](LogLevel level, const std::wstring& message) {
        AddLogEntry(level, message);
    });
    
     
    m_injectionCore->SetLogCallback([this](LogLevel level, const std::wstring& message) {
        AddLogEntry(level, message);
    });
    
     
    RefreshProcessList();
    
     
    if (m_settings.autoRefresh) {
        SetTimer(m_hwnd, ID_TIMER_REFRESH, m_settings.refreshInterval, nullptr);
    }
    
    SetStatusText(L"Ready", 0);
    SetStatusText(ProcessManager::IsRunningAsAdmin() ? L"Administrator" : L"User", 1);
}

void MainWindow::OnDestroy()
{
    KillTimer(m_hwnd, ID_TIMER_REFRESH);
    m_hotkeyManager->Shutdown();
    m_trayIcon->Remove();
    if (m_hImageList) {
        ImageList_Destroy(m_hImageList);
        m_hImageList = nullptr;
    }
    SaveSettings();
    PostQuitMessage(0);
}

void MainWindow::OnSize(int width, int height)
{
     
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void MainWindow::OnPaint()
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hwnd, &ps);
    
     
     
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    FillRect(hdc, &rc, GetSysColorBrush(COLOR_BTNFACE));
    
    EndPaint(m_hwnd, &ps);
}

void MainWindow::OnCommand(WPARAM wParam, LPARAM lParam)
{
    int id = LOWORD(wParam);
    int code = HIWORD(wParam);
    
    switch (id) {
        case ID_PROCESS_SEARCH:
            if (code == EN_CHANGE) {
                wchar_t buffer[256];
                GetWindowTextW(m_hwndProcessSearch, buffer, 256);
                FilterProcessList(buffer);
            }
            break;
            
        case ID_BTN_REFRESH:
            RefreshProcessList();
            break;
            
        case ID_DLL_ADD:
        case ID_MENU_FILE_OPEN_DLL:
            AddDllToList();
            break;
            
        case ID_DLL_REMOVE:
            RemoveDllFromList();
            break;
            
        case ID_BTN_INJECT:
        case ID_MENU_INJECT_START:
            PerformInjection();
            break;
            
        case ID_BTN_EJECT:
        case ID_MENU_INJECT_EJECT:
            PerformEjection();
            break;
            
        case ID_MENU_FILE_EXIT:
            DestroyWindow(m_hwnd);
            break;
            
        case ID_MENU_OPTIONS_DARK_MODE:
            m_settings.darkMode = !m_settings.darkMode;
            CheckMenuItem(m_hMenu, ID_MENU_OPTIONS_DARK_MODE,
                m_settings.darkMode ? MF_CHECKED : MF_UNCHECKED);
            ApplyDarkMode();
            break;
            
        case ID_MENU_OPTIONS_SETTINGS:
        case 4101:  
            ShowSettingsDialog();
            break;
            
        case ID_MENU_OPTIONS_ALWAYS_TOP:
            ToggleAlwaysOnTop();
            break;
            
        case ID_MENU_OPTIONS_MINIMIZE_TRAY:
            MinimizeToTray();
            break;
            
        case ID_MENU_FILE_EXPORT_LOG:
            ExportLog();
            break;
            
        case ID_MENU_HELP_ABOUT:
            ShowAboutDialog();
            break;
            
        case ID_MENU_HELP_GITHUB:
            OpenGitHub();
            break;
            
        case ID_MENU_HELP_SHORTCUTS:
            ShowShortcutsDialog();
            break;
            
         
        case ID_TRAY_SHOW:
            RestoreFromTray();
            break;
            
        case ID_TRAY_INJECT:
            RestoreFromTray();
            PerformInjection();
            break;
            
        case ID_TRAY_EXIT:
            m_trayIcon->Remove();
            DestroyWindow(m_hwnd);
            break;
    }
}

void MainWindow::OnNotify(WPARAM wParam, LPARAM lParam)
{
    auto nmhdr = reinterpret_cast<NMHDR*>(lParam);
    
    if (nmhdr->idFrom == ID_PROCESS_LIST) {
        switch (nmhdr->code) {
            case LVN_ITEMCHANGED: {
                auto nmlistview = reinterpret_cast<NMLISTVIEW*>(lParam);
                if ((nmlistview->uNewState & LVIS_SELECTED) && 
                    !(nmlistview->uOldState & LVIS_SELECTED)) {
                     
                    ProcessInfo* proc = GetSelectedProcess();
                    if (proc) {
                        m_selectedPid = proc->pid;
                        std::wstring info = proc->name + L" (PID: " + std::to_wstring(proc->pid) + L")";
                        if (proc->is64Bit) info += L" [x64]";
                        SetWindowTextW(m_hwndSelectedProc, info.c_str());
                    }
                }
                break;
            }
            case NM_CLICK: {
                NMITEMACTIVATE* nmia = reinterpret_cast<NMITEMACTIVATE*>(lParam);
                if (nmia->iItem >= 0) {
                    ListView_SetItemState(m_hwndProcessList, nmia->iItem, 
                        LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
                }
                break;
            }
        }
    }
}

void MainWindow::OnTimer(WPARAM timerId)
{
    if (timerId == ID_TIMER_REFRESH) {
        RefreshProcessList();
    }
}

void MainWindow::OnDropFiles(HDROP hDrop)
{
    UINT count = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
    for (UINT i = 0; i < count; i++) {
        wchar_t filePath[MAX_PATH];
        if (DragQueryFileW(hDrop, i, filePath, MAX_PATH)) {
            std::wstring path = filePath;
            if (utils::ToLower(utils::GetExtension(path)) == L".dll") {
                 
                bool exists = false;
                for (const auto& dll : m_dllList) {
                    if (_wcsicmp(dll.c_str(), path.c_str()) == 0) {
                        exists = true;
                        break;
                    }
                }
                if (!exists) {
                    m_dllList.push_back(path);
                    std::wstring filename = path.substr(path.find_last_of(L"\\/") + 1);
                    ListBox_AddString(m_hwndDllList, filename.c_str());
                }
            }
        }
    }
     
    if (ListBox_GetCount(m_hwndDllList) > 0) {
        ListBox_SetCurSel(m_hwndDllList, ListBox_GetCount(m_hwndDllList) - 1);
    }
    DragFinish(hDrop);
}

 
 
 

void MainWindow::RefreshProcessList()
{
    SetStatusText(L"Refreshing process list...", 0);
    
    m_processManager->RefreshProcessList();
    
    wchar_t filter[256];
    GetWindowTextW(m_hwndProcessSearch, filter, 256);
    FilterProcessList(filter);
    
    SetStatusText(L"Ready - " + std::to_wstring(m_filteredProcesses.size()) + L" processes", 0);
}


void MainWindow::PerformInjection()
{
    ProcessInfo* proc = GetSelectedProcess();
    if (!proc) {
        MessageBoxW(m_hwnd, L"Please select a target process.", L"No Process Selected", MB_ICONWARNING);
        return;
    }
    
    std::wstring dllPath = GetDllPath();
    if (dllPath.empty()) {
        MessageBoxW(m_hwnd, L"Please select a DLL file.", L"No DLL Selected", MB_ICONWARNING);
        return;
    }
    
    if (!utils::FileExists(dllPath)) {
        MessageBoxW(m_hwnd, L"The selected DLL file does not exist.", L"File Not Found", MB_ICONERROR);
        return;
    }
    
    InjectionMethod method = GetSelectedMethod();
    
    EnableControls(false);
    SetStatusText(L"Injecting...", 0);
    
    auto result = m_injectionCore->Inject(proc->pid, dllPath, method,
        [this](int percent, const std::wstring& status) {
            SetStatusText(status, 0);
        });
    
    EnableControls(true);
    
    if (result.success) {
        m_lastInjectedModule = result.remoteModule;
        m_lastInjectedPid = proc->pid;
        EnableWindow(m_hwndBtnEject, TRUE);
        
        SetStatusText(L"Injection successful!", 0);
        MessageBoxW(m_hwnd, L"DLL injected successfully!", L"Success", MB_ICONINFORMATION);
    } else {
        SetStatusText(L"Injection failed: " + result.errorMessage, 0);
        MessageBoxW(m_hwnd, result.errorMessage.c_str(), L"Injection Failed", MB_ICONERROR);
    }
}

void MainWindow::PerformEjection()
{
    if (!m_lastInjectedModule || m_lastInjectedPid == 0) {
        MessageBoxW(m_hwnd, L"No module to eject.", L"Error", MB_ICONWARNING);
        return;
    }
    
    InjectionMethod method = GetSelectedMethod();
    
    EnableControls(false);
    SetStatusText(L"Ejecting...", 0);
    
    auto result = m_injectionCore->Eject(m_lastInjectedPid, m_lastInjectedModule, method);
    
    EnableControls(true);
    
    if (result.success) {
        m_lastInjectedModule = nullptr;
        m_lastInjectedPid = 0;
        EnableWindow(m_hwndBtnEject, FALSE);
        
        SetStatusText(L"Ejection successful!", 0);
        MessageBoxW(m_hwnd, L"DLL ejected successfully!", L"Success", MB_ICONINFORMATION);
    } else {
        SetStatusText(L"Ejection failed: " + result.errorMessage, 0);
        MessageBoxW(m_hwnd, result.errorMessage.c_str(), L"Ejection Failed", MB_ICONERROR);
    }
}

void MainWindow::ShowAboutDialog()
{
    AboutDialog::Show(m_hwnd);
}

void MainWindow::OpenGitHub()
{
    ShellExecuteW(m_hwnd, L"open", XORDLL_APP_WEBSITE, nullptr, nullptr, SW_SHOWNORMAL);
}

 
 
 

void MainWindow::PopulateProcessList()
{
    ListView_DeleteAllItems(m_hwndProcessList);
    ImageList_RemoveAll(m_hImageList);
    
    HICON hDefaultIcon = LoadIcon(nullptr, IDI_APPLICATION);
    int defaultIconIndex = ImageList_AddIcon(m_hImageList, hDefaultIcon);
    
    LVITEM lvi = { 0 };
    lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
    
    int selectIndex = -1;
    
    for (size_t i = 0; i < m_filteredProcesses.size(); ++i) {
        auto& proc = m_filteredProcesses[i];
        
         
        if (proc.pid == m_selectedPid) {
            selectIndex = static_cast<int>(i);
        }
        
        int iconIndex = defaultIconIndex;
        if (!proc.path.empty()) {
            HICON hIcon = ProcessManager::GetProcessIcon(proc.path);
            if (hIcon) {
                iconIndex = ImageList_AddIcon(m_hImageList, hIcon);
                DestroyIcon(hIcon);
            }
        }
        
        std::wstring displayText = proc.name + L" (" + std::to_wstring(proc.pid) + L")";
        if (proc.is64Bit) displayText += L" [64]";
        
        lvi.iItem = static_cast<int>(i);
        lvi.iSubItem = 0;
        lvi.iImage = iconIndex;
        lvi.lParam = static_cast<LPARAM>(i);
        lvi.pszText = const_cast<LPWSTR>(displayText.c_str());
        ListView_InsertItem(m_hwndProcessList, &lvi);
    }
    
     
    if (selectIndex >= 0) {
        ListView_SetItemState(m_hwndProcessList, selectIndex, 
            LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(m_hwndProcessList, selectIndex, FALSE);
    }
}

void MainWindow::FilterProcessList(const std::wstring& filter)
{
    m_filteredProcesses = m_processManager->FilterByName(filter);
    PopulateProcessList();
}

ProcessInfo* MainWindow::GetSelectedProcess()
{
    int sel = ListView_GetNextItem(m_hwndProcessList, -1, LVNI_SELECTED);
    if (sel < 0 || sel >= static_cast<int>(m_filteredProcesses.size())) {
        return nullptr;
    }
    return &m_filteredProcesses[sel];
}

InjectionMethod MainWindow::GetSelectedMethod()
{
    int sel = ComboBox_GetCurSel(m_hwndMethodCombo);
    auto methods = m_injectionCore->GetAvailableMethods();
    if (sel >= 0 && sel < static_cast<int>(methods.size())) {
        return methods[sel];
    }
    return InjectionMethod::CreateRemoteThread;
}

std::wstring MainWindow::GetDllPath()
{
    int sel = ListBox_GetCurSel(m_hwndDllList);
    if (sel >= 0 && sel < static_cast<int>(m_dllList.size())) {
        return m_dllList[sel];
    }
    return L"";
}

void MainWindow::EnableControls(bool enable)
{
    EnableWindow(m_hwndProcessList, enable);
    EnableWindow(m_hwndProcessSearch, enable);
    EnableWindow(m_hwndDllList, enable);
    EnableWindow(m_hwndBtnDllAdd, enable);
    EnableWindow(m_hwndBtnDllRemove, enable);
    EnableWindow(m_hwndMethodCombo, enable);
    EnableWindow(m_hwndBtnInject, enable);
    EnableWindow(m_hwndBtnRefresh, enable);
}

void MainWindow::AddDllToList()
{
    std::wstring path = utils::ShowOpenFileDialog(
        m_hwnd,
        L"DLL Files (*.dll)\0*.dll\0All Files (*.*)\0*.*\0",
        L"Select DLL to Add"
    );
    
    if (!path.empty()) {
         
        for (const auto& dll : m_dllList) {
            if (_wcsicmp(dll.c_str(), path.c_str()) == 0) {
                SetStatusText(L"DLL already in list", 0);
                return;
            }
        }
        
        m_dllList.push_back(path);
        
         
        std::wstring filename = path.substr(path.find_last_of(L"\\/") + 1);
        ListBox_AddString(m_hwndDllList, filename.c_str());
        ListBox_SetCurSel(m_hwndDllList, ListBox_GetCount(m_hwndDllList) - 1);
        
        SetStatusText(L"Added: " + filename, 0);
    }
}

void MainWindow::RemoveDllFromList()
{
    int sel = ListBox_GetCurSel(m_hwndDllList);
    if (sel >= 0 && sel < static_cast<int>(m_dllList.size())) {
        m_dllList.erase(m_dllList.begin() + sel);
        ListBox_DeleteString(m_hwndDllList, sel);
        
         
        if (sel >= ListBox_GetCount(m_hwndDllList)) {
            sel = ListBox_GetCount(m_hwndDllList) - 1;
        }
        if (sel >= 0) {
            ListBox_SetCurSel(m_hwndDllList, sel);
        }
        
        SetStatusText(L"DLL removed", 0);
    }
}

 
 
 

void MainWindow::OnHotkey(WPARAM wParam)
{
    if (m_hotkeyManager) {
        m_hotkeyManager->HandleHotkey(wParam);
    }
}

void MainWindow::OnTrayIcon(LPARAM lParam)
{
    if (m_trayIcon) {
        m_trayIcon->HandleMessage(lParam);
    }
}

void MainWindow::OnMinimize()
{
    if (Settings::Instance().GetMinimizeToTray()) {
        MinimizeToTray();
    }
}

 
 
 

void MainWindow::ShowShortcutsDialog()
{
    std::wstring shortcuts = 
        L"Keyboard Shortcuts:\n\n"
        L"F5\t\tInject DLL\n"
        L"F6\t\tEject DLL\n"
        L"F9\t\tRefresh process list\n"
        L"Ctrl+O\t\tOpen DLL file\n"
        L"Ctrl+F\t\tFocus search box\n"
        L"Ctrl+,\t\tOpen settings\n"
        L"F1\t\tShow this help\n"
        L"Alt+F4\t\tExit application";
    
    MessageBoxW(m_hwnd, shortcuts.c_str(), L"Keyboard Shortcuts", MB_ICONINFORMATION | MB_OK);
}

void MainWindow::ExportLog()
{
    std::wstring path = utils::ShowSaveFileDialog(
        m_hwnd,
        L"Log Files (*.log)\0*.log\0Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0",
        L"log",
        L"Export Log"
    );
    
    if (!path.empty()) {
        if (Logger::Instance().ExportToFile(path)) {
            LOG_INFO(L"Log exported to: " + path);
            MessageBoxW(m_hwnd, L"Log exported successfully!", L"Export Complete", MB_ICONINFORMATION);
        } else {
            LOG_ERROR(L"Failed to export log to: " + path);
            MessageBoxW(m_hwnd, L"Failed to export log.", L"Export Error", MB_ICONERROR);
        }
    }
}

void MainWindow::ToggleAlwaysOnTop()
{
    m_alwaysOnTop = !m_alwaysOnTop;
    
    SetWindowPos(m_hwnd, 
        m_alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST,
        0, 0, 0, 0, 
        SWP_NOMOVE | SWP_NOSIZE);
    
     
    CheckMenuItem(m_hMenu, ID_MENU_OPTIONS_ALWAYS_TOP,
        m_alwaysOnTop ? MF_CHECKED : MF_UNCHECKED);
    
    LOG_DEBUG(m_alwaysOnTop ? L"Always on top enabled" : L"Always on top disabled");
}

void MainWindow::MinimizeToTray()
{
    if (!m_trayIcon->IsVisible()) {
        m_trayIcon->Create(m_hwnd, nullptr, L"xorDLL - DLL Injector");
        
         
        m_trayIcon->SetOnLeftClick([this]() { RestoreFromTray(); });
        m_trayIcon->SetOnDoubleClick([this]() { RestoreFromTray(); });
        m_trayIcon->SetOnRightClick([this]() { ShowTrayMenu(); });
    }
    
    ShowWindow(m_hwnd, SW_HIDE);
    m_minimizedToTray = true;
    
    m_trayIcon->ShowBalloon(L"xorDLL", L"Application minimized to tray", NIIF_INFO);
}

void MainWindow::RestoreFromTray()
{
    ShowWindow(m_hwnd, SW_SHOW);
    SetForegroundWindow(m_hwnd);
    m_minimizedToTray = false;
}

void MainWindow::ShowTrayMenu()
{
    ui::TrayMenuBuilder builder;
    
    HMENU menu = builder
        .AddItem(ID_TRAY_SHOW, L"Show xorDLL")
        .SetDefault(ID_TRAY_SHOW)
        .AddSeparator()
        .AddItem(ID_TRAY_INJECT, L"Quick Inject", !GetDllPath().empty())
        .AddSeparator()
        .AddItem(ID_TRAY_EXIT, L"Exit")
        .Build();
    
    ui::TrayIcon::ShowContextMenu(m_hwnd, menu);
    DestroyMenu(menu);
}

 
 
 

INT_PTR AboutDialog::Show(HWND hwndParent)
{
    ui::AboutDialog::Show(hwndParent);
    return 0;
}

}  
