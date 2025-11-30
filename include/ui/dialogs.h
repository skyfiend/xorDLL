#pragma once

#include "core/types.h"
#include "utils/settings.h"
#include <functional>

namespace xordll {
namespace ui {

 
class SettingsDialog {
public:
     
    static bool Show(HWND hwndParent);

private:
    static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static void InitDialog(HWND hwnd);
    static void SaveSettings(HWND hwnd);
    static void OnCommand(HWND hwnd, WPARAM wParam);
    
    static HWND CreateSettingsWindow(HWND parent);
};

 
class AboutDialog {
public:
     
    static void Show(HWND hwndParent);

private:
    static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static void OnPaint(HWND hwnd);
    static void OnTimer(HWND hwnd);
    
    static int s_animFrame;
    static float s_logoRotation;
};

 
class DllInfoDialog {
public:
     
    static void Show(HWND hwndParent, const DllInfo& dllInfo);

private:
    static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

 
class ProcessInfoDialog {
public:
     
    static void Show(HWND hwndParent, const ProcessInfo& processInfo);

private:
    static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

 
class ProgressDialog {
public:
    ProgressDialog();
    ~ProgressDialog();
    
     
    bool Create(HWND hwndParent, const std::wstring& title);
    
     
    void Close();
    
     
    void SetProgress(int percent);
    
     
    void SetStatus(const std::wstring& status);
    
     
    bool IsCancelled() const { return m_cancelled; }
    
     
    HWND GetHandle() const { return m_hwnd; }

private:
    static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    HWND m_hwnd;
    HWND m_hwndProgress;
    HWND m_hwndStatus;
    bool m_cancelled;
};

 
class CustomMessageBox {
public:
    enum class Type {
        Info,
        Warning,
        Error,
        Question
    };
    
    enum class Buttons {
        Ok,
        OkCancel,
        YesNo,
        YesNoCancel
    };
    
    enum class Result {
        Ok,
        Cancel,
        Yes,
        No
    };
    
     
    static Result Show(
        HWND hwndParent,
        const std::wstring& title,
        const std::wstring& message,
        Type type = Type::Info,
        Buttons buttons = Buttons::Ok
    );

private:
    static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

 
class WelcomeDialog {
public:
     
    static bool Show(HWND hwndParent);

private:
    static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

}  
}  
