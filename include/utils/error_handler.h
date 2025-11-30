#pragma once

#include "core/types.h"
#include <string>
#include <exception>
#include <functional>

namespace xordll {

 
enum class ErrorCode {
    Success = 0,
    
     
    Unknown = 1,
    InvalidArgument = 2,
    NotInitialized = 3,
    AlreadyInitialized = 4,
    
     
    FileNotFound = 100,
    FileAccessDenied = 101,
    InvalidFileFormat = 102,
    FileTooLarge = 103,
    FileReadError = 104,
    
     
    ProcessNotFound = 200,
    ProcessAccessDenied = 201,
    ProcessTerminated = 202,
    ProcessArchMismatch = 203,
    ProcessIsSystem = 204,
    ProcessIsProtected = 205,
    
     
    InjectionFailed = 300,
    MemoryAllocationFailed = 301,
    MemoryWriteFailed = 302,
    ThreadCreationFailed = 303,
    ModuleLoadFailed = 304,
    ModuleNotFound = 305,
    
     
    InvalidDll = 400,
    DllArchMismatch = 401,
    DllNotSigned = 402,
    DllCorrupted = 403,
    
     
    InsufficientPrivileges = 500,
    AdminRequired = 501,
    DebugPrivilegeRequired = 502
};

 
struct ErrorInfo {
    ErrorCode code;
    DWORD windowsError;
    std::wstring message;
    std::wstring details;
    std::wstring suggestion;
    
    ErrorInfo()
        : code(ErrorCode::Success)
        , windowsError(0) {}
    
    ErrorInfo(ErrorCode c, const std::wstring& msg)
        : code(c)
        , windowsError(0)
        , message(msg) {}
    
    ErrorInfo(ErrorCode c, DWORD winErr, const std::wstring& msg)
        : code(c)
        , windowsError(winErr)
        , message(msg) {}
    
    bool IsSuccess() const { return code == ErrorCode::Success; }
    bool IsError() const { return code != ErrorCode::Success; }
};

 
class XorDllException : public std::exception {
public:
    XorDllException(const ErrorInfo& info)
        : m_info(info) {}
    
    XorDllException(ErrorCode code, const std::wstring& message)
        : m_info(code, message) {}
    
    const char* what() const noexcept override {
        return "xorDLL Exception";
    }
    
    const ErrorInfo& GetErrorInfo() const { return m_info; }
    ErrorCode GetCode() const { return m_info.code; }
    const std::wstring& GetMessage() const { return m_info.message; }
    
private:
    ErrorInfo m_info;
};

 
class ErrorHandler {
public:
     
    static std::wstring GetErrorMessage(ErrorCode code);
    
     
    static std::wstring GetErrorSuggestion(ErrorCode code);
    
     
    static ErrorInfo FromWindowsError(DWORD winError, const std::wstring& operation);
    
     
    static ErrorInfo FromLastError(const std::wstring& operation);
    
     
    static ErrorCode MapWindowsError(DWORD winError);
    
     
    static std::wstring FormatError(const ErrorInfo& info);
    
     
    static void ShowErrorDialog(HWND hwnd, const ErrorInfo& info);
    
     
    static void LogError(const ErrorInfo& info);
    
     
    static void SetErrorCallback(std::function<void(const ErrorInfo&)> callback);

private:
    static std::function<void(const ErrorInfo&)> s_errorCallback;
};

 
#define XORDLL_CHECK(condition, code, message) \
    do { \
        if (!(condition)) { \
            throw XorDllException(code, message); \
        } \
    } while (0)

#define XORDLL_CHECK_WIN(result, operation) \
    do { \
        if (!(result)) { \
            throw XorDllException(ErrorHandler::FromLastError(operation)); \
        } \
    } while (0)

}  
