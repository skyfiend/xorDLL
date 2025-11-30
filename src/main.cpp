 

#include "ui/main_window.h"
#include "utils/logger.h"
#include "core/process_manager.h"
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' \
    name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
    processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

 
int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow
) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    
     
    INITCOMMONCONTROLSEX icc = { 0 };
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_WIN95_CLASSES | ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES;
    if (!InitCommonControlsEx(&icc)) {
        MessageBoxW(nullptr, L"Failed to initialize common controls", 
            L"xorDLL Error", MB_ICONERROR | MB_OK);
        return 1;
    }
    
     
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        MessageBoxW(nullptr, L"Failed to initialize COM", 
            L"xorDLL Error", MB_ICONERROR | MB_OK);
        return 1;
    }
    
     
    xordll::Logger::Instance().Initialize();
    LOG_INFO(L"xorDLL starting...");
    
     
    if (xordll::ProcessManager::EnableDebugPrivilege()) {
        LOG_INFO(L"Debug privilege enabled");
    } else {
        LOG_WARNING(L"Failed to enable debug privilege - some processes may not be accessible");
    }
    
     
    if (xordll::ProcessManager::IsRunningAsAdmin()) {
        LOG_INFO(L"Running with administrator privileges");
    } else {
        LOG_WARNING(L"Not running as administrator - some features may be limited");
    }
    
     
    int exitCode = 0;
    {
        xordll::MainWindow mainWindow;
        
        if (!mainWindow.Create(hInstance, nCmdShow)) {
            LOG_ERROR(L"Failed to create main window");
            MessageBoxW(nullptr, L"Failed to create main window", 
                L"xorDLL Error", MB_ICONERROR | MB_OK);
            exitCode = 1;
        } else {
            LOG_INFO(L"Main window created successfully");
            exitCode = mainWindow.Run();
        }
    }
    
     
    LOG_INFO(L"xorDLL shutting down...");
    xordll::Logger::Instance().Shutdown();
    CoUninitialize();
    
    return exitCode;
}

#ifdef __MINGW32__
 
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    return wWinMain(hInstance, hPrevInstance, GetCommandLineW(), nCmdShow);
}
#endif
