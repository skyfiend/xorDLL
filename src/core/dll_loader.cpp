 

#include "core/dll_loader.h"
#include "utils/file_utils.h"
#include "utils/string_utils.h"
#include "utils/logger.h"
#include <softpub.h>
#include <wintrust.h>
#include <mscat.h>

#pragma comment(lib, "wintrust.lib")
#pragma comment(lib, "version.lib")

namespace xordll {

DllLoader& DllLoader::Instance() {
    static DllLoader instance;
    return instance;
}

bool DllLoader::LoadDll(const std::wstring& path, DllInfo& info) {
     
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_cache.find(path);
        if (it != m_cache.end()) {
            info = it->second;
            return true;
        }
    }
    
     
    if (!utils::FileExists(path)) {
        LOG_ERROR(L"DLL file not found: " + path);
        return false;
    }
    
     
    if (!ValidatePEHeaders(path)) {
        LOG_ERROR(L"Invalid PE headers: " + path);
        return false;
    }
    
     
    info.path = path;
    info.name = utils::GetFileName(path);
    info.fileSize = utils::GetFileSize(path);
    
     
    std::vector<BYTE> data;
    if (!utils::ReadFileToMemory(path, data)) {
        LOG_ERROR(L"Failed to read DLL file: " + path);
        return false;
    }
    
    auto dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(data.data());
    auto ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(data.data() + dosHeader->e_lfanew);
    
    info.is64Bit = (ntHeaders->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64);
    
     
    info.description = GetDescription(path);
    info.version = GetVersion(path);
    
     
    info.isSigned = VerifyDigitalSignature(path);
    
     
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cache[path] = info;
    }
    
    LOG_DEBUG(L"DLL loaded: " + info.name + L" (" + (info.is64Bit ? L"x64" : L"x86") + L")");
    return true;
}

std::optional<DllInfo> DllLoader::GetCachedInfo(const std::wstring& path) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_cache.find(path);
    if (it != m_cache.end()) {
        return it->second;
    }
    return std::nullopt;
}

void DllLoader::ClearCache() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache.clear();
    LOG_DEBUG(L"DLL cache cleared");
}

void DllLoader::RemoveFromCache(const std::wstring& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache.erase(path);
}

bool DllLoader::IsCompatible(const DllInfo& dllInfo, bool processIs64Bit) {
    return dllInfo.is64Bit == processIs64Bit;
}

bool DllLoader::ValidatePEHeaders(const std::wstring& path) {
    std::vector<BYTE> data;
    if (!utils::ReadFileToMemory(path, data)) {
        return false;
    }
    
    if (data.size() < sizeof(IMAGE_DOS_HEADER)) {
        return false;
    }
    
    auto dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(data.data());
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        return false;
    }
    
    if (data.size() < static_cast<size_t>(dosHeader->e_lfanew) + sizeof(IMAGE_NT_HEADERS)) {
        return false;
    }
    
    auto ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(data.data() + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
        return false;
    }
    
     
    if (!(ntHeaders->FileHeader.Characteristics & IMAGE_FILE_DLL)) {
        return false;
    }
    
     
    WORD machine = ntHeaders->FileHeader.Machine;
    if (machine != IMAGE_FILE_MACHINE_I386 && machine != IMAGE_FILE_MACHINE_AMD64) {
        return false;
    }
    
    return true;
}

bool DllLoader::VerifyDigitalSignature(const std::wstring& path) {
    WINTRUST_FILE_INFO fileInfo = { 0 };
    fileInfo.cbStruct = sizeof(fileInfo);
    fileInfo.pcwszFilePath = path.c_str();
    fileInfo.hFile = nullptr;
    fileInfo.pgKnownSubject = nullptr;
    
    GUID policyGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    
    WINTRUST_DATA winTrustData = { 0 };
    winTrustData.cbStruct = sizeof(winTrustData);
    winTrustData.pPolicyCallbackData = nullptr;
    winTrustData.pSIPClientData = nullptr;
    winTrustData.dwUIChoice = WTD_UI_NONE;
    winTrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    winTrustData.dwUnionChoice = WTD_CHOICE_FILE;
    winTrustData.dwStateAction = WTD_STATEACTION_VERIFY;
    winTrustData.hWVTStateData = nullptr;
    winTrustData.pwszURLReference = nullptr;
    winTrustData.dwProvFlags = WTD_SAFER_FLAG;
    winTrustData.dwUIContext = 0;
    winTrustData.pFile = &fileInfo;
    
    LONG status = WinVerifyTrust(nullptr, &policyGUID, &winTrustData);
    
     
    winTrustData.dwStateAction = WTD_STATEACTION_CLOSE;
    WinVerifyTrust(nullptr, &policyGUID, &winTrustData);
    
    return status == ERROR_SUCCESS;
}

std::vector<std::string> DllLoader::GetExports(const std::wstring& path) {
    std::vector<std::string> exports;
    
    std::vector<BYTE> data;
    if (!utils::ReadFileToMemory(path, data)) {
        return exports;
    }
    
    auto dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(data.data());
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        return exports;
    }
    
    auto ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(data.data() + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
        return exports;
    }
    
     
    DWORD exportDirRVA = 0;
    DWORD exportDirSize = 0;
    
    if (ntHeaders->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64) {
        auto ntHeaders64 = reinterpret_cast<IMAGE_NT_HEADERS64*>(ntHeaders);
        exportDirRVA = ntHeaders64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
        exportDirSize = ntHeaders64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
    } else {
        auto ntHeaders32 = reinterpret_cast<IMAGE_NT_HEADERS32*>(ntHeaders);
        exportDirRVA = ntHeaders32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
        exportDirSize = ntHeaders32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
    }
    
    if (exportDirRVA == 0 || exportDirSize == 0) {
        return exports;   
    }
    
     
    auto sectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
    DWORD exportDirOffset = 0;
    
    for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++) {
        if (exportDirRVA >= sectionHeader[i].VirtualAddress &&
            exportDirRVA < sectionHeader[i].VirtualAddress + sectionHeader[i].Misc.VirtualSize) {
            exportDirOffset = exportDirRVA - sectionHeader[i].VirtualAddress + sectionHeader[i].PointerToRawData;
            break;
        }
    }
    
    if (exportDirOffset == 0 || exportDirOffset >= data.size()) {
        return exports;
    }
    
    auto exportDir = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(data.data() + exportDirOffset);
    
     
    DWORD namesRVA = exportDir->AddressOfNames;
    DWORD namesOffset = 0;
    
    for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++) {
        if (namesRVA >= sectionHeader[i].VirtualAddress &&
            namesRVA < sectionHeader[i].VirtualAddress + sectionHeader[i].Misc.VirtualSize) {
            namesOffset = namesRVA - sectionHeader[i].VirtualAddress + sectionHeader[i].PointerToRawData;
            break;
        }
    }
    
    if (namesOffset == 0) {
        return exports;
    }
    
    auto nameRVAs = reinterpret_cast<DWORD*>(data.data() + namesOffset);
    
    for (DWORD i = 0; i < exportDir->NumberOfNames && i < 1000; i++) {   
        DWORD nameRVA = nameRVAs[i];
        DWORD nameOffset = 0;
        
        for (int j = 0; j < ntHeaders->FileHeader.NumberOfSections; j++) {
            if (nameRVA >= sectionHeader[j].VirtualAddress &&
                nameRVA < sectionHeader[j].VirtualAddress + sectionHeader[j].Misc.VirtualSize) {
                nameOffset = nameRVA - sectionHeader[j].VirtualAddress + sectionHeader[j].PointerToRawData;
                break;
            }
        }
        
        if (nameOffset > 0 && nameOffset < data.size()) {
            const char* name = reinterpret_cast<const char*>(data.data() + nameOffset);
            exports.push_back(name);
        }
    }
    
    return exports;
}

std::wstring DllLoader::GetDescription(const std::wstring& path) {
    return GetVersionInfoString(path, L"FileDescription");
}

std::wstring DllLoader::GetVersion(const std::wstring& path) {
    return GetVersionInfoString(path, L"FileVersion");
}

std::wstring DllLoader::GetCompanyName(const std::wstring& path) {
    return GetVersionInfoString(path, L"CompanyName");
}

std::wstring DllLoader::GetVersionInfoString(const std::wstring& path, const std::wstring& key) {
    DWORD handle = 0;
    DWORD size = GetFileVersionInfoSizeW(path.c_str(), &handle);
    
    if (size == 0) {
        return L"";
    }
    
    std::vector<BYTE> buffer(size);
    if (!GetFileVersionInfoW(path.c_str(), handle, size, buffer.data())) {
        return L"";
    }
    
     
    struct LANGANDCODEPAGE {
        WORD wLanguage;
        WORD wCodePage;
    } *lpTranslate;
    
    UINT translateLen = 0;
    if (!VerQueryValueW(buffer.data(), L"\\VarFileInfo\\Translation",
        reinterpret_cast<LPVOID*>(&lpTranslate), &translateLen)) {
        return L"";
    }
    
    if (translateLen < sizeof(LANGANDCODEPAGE)) {
        return L"";
    }
    
     
    wchar_t subBlock[256];
    swprintf_s(subBlock, L"\\StringFileInfo\\%04x%04x\\%s",
        lpTranslate[0].wLanguage, lpTranslate[0].wCodePage, key.c_str());
    
    LPWSTR value = nullptr;
    UINT valueLen = 0;
    
    if (!VerQueryValueW(buffer.data(), subBlock, reinterpret_cast<LPVOID*>(&value), &valueLen)) {
        return L"";
    }
    
    if (value && valueLen > 0) {
        return std::wstring(value);
    }
    
    return L"";
}

}  
