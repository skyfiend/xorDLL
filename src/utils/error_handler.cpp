 

#include "utils/error_handler.h"
#include "utils/string_utils.h"
#include "utils/logger.h"

namespace xordll {

std::function<void(const ErrorInfo&)> ErrorHandler::s_errorCallback = nullptr;

std::wstring ErrorHandler::GetErrorMessage(ErrorCode code) {
    switch (code) {
        case ErrorCode::Success:
            return L"Operation completed successfully";
            
         
        case ErrorCode::Unknown:
            return L"An unknown error occurred";
        case ErrorCode::InvalidArgument:
            return L"Invalid argument provided";
        case ErrorCode::NotInitialized:
            return L"Component not initialized";
        case ErrorCode::AlreadyInitialized:
            return L"Component already initialized";
            
         
        case ErrorCode::FileNotFound:
            return L"File not found";
        case ErrorCode::FileAccessDenied:
            return L"Access to file denied";
        case ErrorCode::InvalidFileFormat:
            return L"Invalid file format";
        case ErrorCode::FileTooLarge:
            return L"File is too large";
        case ErrorCode::FileReadError:
            return L"Error reading file";
            
         
        case ErrorCode::ProcessNotFound:
            return L"Process not found";
        case ErrorCode::ProcessAccessDenied:
            return L"Access to process denied";
        case ErrorCode::ProcessTerminated:
            return L"Process has terminated";
        case ErrorCode::ProcessArchMismatch:
            return L"Process architecture mismatch";
        case ErrorCode::ProcessIsSystem:
            return L"Cannot inject into system process";
        case ErrorCode::ProcessIsProtected:
            return L"Process is protected";
            
         
        case ErrorCode::InjectionFailed:
            return L"Injection failed";
        case ErrorCode::MemoryAllocationFailed:
            return L"Failed to allocate memory in target process";
        case ErrorCode::MemoryWriteFailed:
            return L"Failed to write to target process memory";
        case ErrorCode::ThreadCreationFailed:
            return L"Failed to create remote thread";
        case ErrorCode::ModuleLoadFailed:
            return L"Failed to load module in target process";
        case ErrorCode::ModuleNotFound:
            return L"Module not found in target process";
            
         
        case ErrorCode::InvalidDll:
            return L"Invalid DLL file";
        case ErrorCode::DllArchMismatch:
            return L"DLL architecture does not match target process";
        case ErrorCode::DllNotSigned:
            return L"DLL is not digitally signed";
        case ErrorCode::DllCorrupted:
            return L"DLL file appears to be corrupted";
            
         
        case ErrorCode::InsufficientPrivileges:
            return L"Insufficient privileges";
        case ErrorCode::AdminRequired:
            return L"Administrator privileges required";
        case ErrorCode::DebugPrivilegeRequired:
            return L"Debug privilege required";
            
        default:
            return L"Unknown error";
    }
}

std::wstring ErrorHandler::GetErrorSuggestion(ErrorCode code) {
    switch (code) {
        case ErrorCode::FileNotFound:
            return L"Please verify the file path is correct.";
            
        case ErrorCode::FileAccessDenied:
            return L"Try running the application as administrator.";
            
        case ErrorCode::ProcessAccessDenied:
            return L"Run xorDLL as administrator to access this process.";
            
        case ErrorCode::ProcessArchMismatch:
            return L"Use the correct version of xorDLL (x86 or x64) for the target process.";
            
        case ErrorCode::ProcessIsSystem:
            return L"System processes cannot be injected into for security reasons.";
            
        case ErrorCode::ProcessIsProtected:
            return L"This process is protected and cannot be modified.";
            
        case ErrorCode::DllArchMismatch:
            return L"Use a DLL that matches the target process architecture (x86 or x64).";
            
        case ErrorCode::AdminRequired:
        case ErrorCode::InsufficientPrivileges:
            return L"Right-click xorDLL and select 'Run as administrator'.";
            
        case ErrorCode::DebugPrivilegeRequired:
            return L"Run as administrator to enable debug privileges.";
            
        case ErrorCode::MemoryAllocationFailed:
        case ErrorCode::MemoryWriteFailed:
            return L"The target process may have memory protection. Try a different injection method.";
            
        case ErrorCode::ThreadCreationFailed:
            return L"Try using NtCreateThreadEx method instead of CreateRemoteThread.";
            
        default:
            return L"";
    }
}

ErrorInfo ErrorHandler::FromWindowsError(DWORD winError, const std::wstring& operation) {
    ErrorInfo info;
    info.windowsError = winError;
    info.code = MapWindowsError(winError);
    info.message = operation + L": " + utils::FormatWindowsError(winError);
    info.suggestion = GetErrorSuggestion(info.code);
    return info;
}

ErrorInfo ErrorHandler::FromLastError(const std::wstring& operation) {
    return FromWindowsError(GetLastError(), operation);
}

ErrorCode ErrorHandler::MapWindowsError(DWORD winError) {
    switch (winError) {
        case ERROR_SUCCESS:
            return ErrorCode::Success;
            
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
            return ErrorCode::FileNotFound;
            
        case ERROR_ACCESS_DENIED:
            return ErrorCode::ProcessAccessDenied;
            
        case ERROR_INVALID_HANDLE:
            return ErrorCode::ProcessNotFound;
            
        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_OUTOFMEMORY:
            return ErrorCode::MemoryAllocationFailed;
            
        case ERROR_WRITE_FAULT:
            return ErrorCode::MemoryWriteFailed;
            
        case ERROR_BAD_EXE_FORMAT:
        case ERROR_EXE_MACHINE_TYPE_MISMATCH:
            return ErrorCode::DllArchMismatch;
            
        case ERROR_MOD_NOT_FOUND:
            return ErrorCode::ModuleNotFound;
            
        case ERROR_PRIVILEGE_NOT_HELD:
            return ErrorCode::InsufficientPrivileges;
            
        default:
            return ErrorCode::Unknown;
    }
}

std::wstring ErrorHandler::FormatError(const ErrorInfo& info) {
    std::wstring result;
    
     
    result = info.message;
    
     
    if (!info.details.empty()) {
        result += L"\n\nDetails: " + info.details;
    }
    
     
    if (info.windowsError != 0) {
        result += L"\n\nWindows Error: " + utils::FormatWindowsError(info.windowsError);
    }
    
     
    if (!info.suggestion.empty()) {
        result += L"\n\nSuggestion: " + info.suggestion;
    }
    
    return result;
}

void ErrorHandler::ShowErrorDialog(HWND hwnd, const ErrorInfo& info) {
    std::wstring title = L"xorDLL - Error";
    std::wstring message = FormatError(info);
    
    MessageBoxW(hwnd, message.c_str(), title.c_str(), MB_ICONERROR | MB_OK);
}

void ErrorHandler::LogError(const ErrorInfo& info) {
    std::wstring logMessage = L"Error [" + std::to_wstring(static_cast<int>(info.code)) + L"]: " + info.message;
    
    if (info.windowsError != 0) {
        logMessage += L" (Windows Error: " + std::to_wstring(info.windowsError) + L")";
    }
    
    LOG_ERROR(logMessage);
    
     
    if (s_errorCallback) {
        s_errorCallback(info);
    }
}

void ErrorHandler::SetErrorCallback(std::function<void(const ErrorInfo&)> callback) {
    s_errorCallback = callback;
}

}  
