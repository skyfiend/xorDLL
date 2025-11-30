#pragma once

#include <string>
#include <vector>
#include <windows.h>
#include <shlobj.h>
#include <commdlg.h>

namespace xordll {
namespace utils {

 
inline bool FileExists(const std::wstring& path) {
    DWORD attrs = GetFileAttributesW(path.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY));
}

 
inline bool DirectoryExists(const std::wstring& path) {
    DWORD attrs = GetFileAttributesW(path.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY));
}

 
inline size_t GetFileSize(const std::wstring& path) {
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fileInfo)) {
        return 0;
    }
    
    LARGE_INTEGER size;
    size.HighPart = fileInfo.nFileSizeHigh;
    size.LowPart = fileInfo.nFileSizeLow;
    return static_cast<size_t>(size.QuadPart);
}

 
inline bool CreateDirectoryRecursive(const std::wstring& path) {
    if (DirectoryExists(path)) return true;
    
    size_t pos = path.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        std::wstring parent = path.substr(0, pos);
        if (!CreateDirectoryRecursive(parent)) {
            return false;
        }
    }
    
    return CreateDirectoryW(path.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS;
}

 
inline std::wstring GetAppDataPath(const std::wstring& appName = L"xorDLL") {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, path))) {
        std::wstring result = std::wstring(path) + L"\\" + appName;
        CreateDirectoryRecursive(result);
        return result;
    }
    return L"";
}

 
inline std::wstring GetExecutableDirectory() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring fullPath = path;
    size_t pos = fullPath.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        return fullPath.substr(0, pos);
    }
    return fullPath;
}

 
inline bool ReadFileToMemory(const std::wstring& path, std::vector<BYTE>& data) {
    HANDLE hFile = CreateFileW(
        path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize)) {
        CloseHandle(hFile);
        return false;
    }
    
    data.resize(static_cast<size_t>(fileSize.QuadPart));
    
    DWORD bytesRead;
    BOOL success = ReadFile(hFile, data.data(), static_cast<DWORD>(data.size()), &bytesRead, nullptr);
    CloseHandle(hFile);
    
    return success && bytesRead == data.size();
}

 
inline std::wstring ShowOpenFileDialog(
    HWND hwndOwner,
    const wchar_t* filter,
    const wchar_t* title = L"Open File"
) {
    wchar_t filename[MAX_PATH] = { 0 };
    
    OPENFILENAMEW ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwndOwner;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    
    if (GetOpenFileNameW(&ofn)) {
        return filename;
    }
    
    return L"";
}

 
inline std::wstring ShowSaveFileDialog(
    HWND hwndOwner,
    const wchar_t* filter,
    const wchar_t* defaultExt = nullptr,
    const wchar_t* title = L"Save File"
) {
    wchar_t filename[MAX_PATH] = { 0 };
    
    OPENFILENAMEW ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwndOwner;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title;
    ofn.lpstrDefExt = defaultExt;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    
    if (GetSaveFileNameW(&ofn)) {
        return filename;
    }
    
    return L"";
}

}  
}  
