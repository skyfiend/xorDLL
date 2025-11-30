#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <windows.h>

namespace xordll {
namespace utils {

 
inline std::string WideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), 
        static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
    
    std::string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), 
        static_cast<int>(wstr.size()), &result[0], size, nullptr, nullptr);
    
    return result;
}

 
inline std::wstring Utf8ToWide(const std::string& str) {
    if (str.empty()) return std::wstring();
    
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), 
        static_cast<int>(str.size()), nullptr, 0);
    
    std::wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), 
        static_cast<int>(str.size()), &result[0], size);
    
    return result;
}

 
inline std::wstring ToLower(const std::wstring& str) {
    std::wstring result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::towlower);
    return result;
}

 
inline std::wstring ToUpper(const std::wstring& str) {
    std::wstring result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::towupper);
    return result;
}

 
inline bool ContainsIgnoreCase(const std::wstring& str, const std::wstring& substr) {
    return ToLower(str).find(ToLower(substr)) != std::wstring::npos;
}

 
inline std::wstring Trim(const std::wstring& str) {
    const wchar_t* whitespace = L" \t\n\r\f\v";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::wstring::npos) return L"";
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

 
inline std::vector<std::wstring> Split(const std::wstring& str, wchar_t delimiter) {
    std::vector<std::wstring> result;
    size_t start = 0;
    size_t end = str.find(delimiter);
    
    while (end != std::wstring::npos) {
        result.push_back(str.substr(start, end - start));
        start = end + 1;
        end = str.find(delimiter, start);
    }
    
    result.push_back(str.substr(start));
    return result;
}

 
inline std::wstring Join(const std::vector<std::wstring>& strings, const std::wstring& delimiter) {
    if (strings.empty()) return L"";
    
    std::wstring result = strings[0];
    for (size_t i = 1; i < strings.size(); ++i) {
        result += delimiter + strings[i];
    }
    return result;
}

 
inline std::wstring FormatWindowsError(DWORD errorCode) {
    if (errorCode == 0) return L"No error";
    
    LPWSTR messageBuffer = nullptr;
    DWORD size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&messageBuffer),
        0,
        nullptr
    );
    
    std::wstring message;
    if (size > 0 && messageBuffer) {
        message = std::wstring(messageBuffer, size);
        message = Trim(message);
        LocalFree(messageBuffer);
    } else {
        message = L"Unknown error";
    }
    
    return L"[" + std::to_wstring(errorCode) + L"] " + message;
}

 
inline std::wstring FormatFileSize(size_t bytes) {
    const wchar_t* units[] = { L"B", L"KB", L"MB", L"GB", L"TB" };
    int unitIndex = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        unitIndex++;
    }
    
    wchar_t buffer[64];
    if (unitIndex == 0) {
        swprintf_s(buffer, L"%.0f %s", size, units[unitIndex]);
    } else {
        swprintf_s(buffer, L"%.2f %s", size, units[unitIndex]);
    }
    
    return buffer;
}

 
inline std::wstring GetFileName(const std::wstring& path) {
    size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) return path;
    return path.substr(pos + 1);
}

 
inline std::wstring GetDirectory(const std::wstring& path) {
    size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) return L"";
    return path.substr(0, pos);
}

 
inline std::wstring GetExtension(const std::wstring& path) {
    size_t pos = path.find_last_of(L'.');
    if (pos == std::wstring::npos) return L"";
    return path.substr(pos);
}

}  
}  
