#pragma once

#include "core/types.h"
#include <windows.h>
#include <string>
#include <vector>
#include <functional>

namespace xordll {

 
enum class ManualMapFlags : DWORD {
    None = 0,
    ClearHeader = 1 << 0,            
    ClearNonNeeded = 1 << 1,         
    AdjustProtections = 1 << 2,      
    HandleTLS = 1 << 3,              
    HandleExceptions = 1 << 4,       
    RunUnderLdr = 1 << 5,            
    ShiftModule = 1 << 6,            
    CleanDataDirs = 1 << 7,          
    
     
    Default = ClearHeader | AdjustProtections | HandleTLS | HandleExceptions,
    Stealth = ClearHeader | ClearNonNeeded | AdjustProtections | HandleTLS | 
              HandleExceptions | CleanDataDirs,
    Maximum = ClearHeader | ClearNonNeeded | AdjustProtections | HandleTLS | 
              HandleExceptions | ShiftModule | CleanDataDirs
};

inline ManualMapFlags operator|(ManualMapFlags a, ManualMapFlags b) {
    return static_cast<ManualMapFlags>(static_cast<DWORD>(a) | static_cast<DWORD>(b));
}

inline bool operator&(ManualMapFlags a, ManualMapFlags b) {
    return (static_cast<DWORD>(a) & static_cast<DWORD>(b)) != 0;
}

 
struct ImportDescriptor {
    std::string moduleName;
    std::vector<std::pair<std::string, FARPROC>> functions;
};

 
struct RelocationEntry {
    DWORD rva;
    WORD type;
};

 
struct SectionInfo {
    std::string name;
    DWORD virtualAddress;
    DWORD virtualSize;
    DWORD rawDataOffset;
    DWORD rawDataSize;
    DWORD characteristics;
};

 
struct ManualMapResult {
    bool success;
    LPVOID baseAddress;
    SIZE_T mappedSize;
    std::wstring errorMessage;
    DWORD errorCode;
};

 
class ManualMapper {
public:
    using LogCallback = std::function<void(LogLevel, const std::wstring&)>;
    
    ManualMapper();
    ~ManualMapper();
    
     
    void SetLogCallback(LogCallback callback) { m_logCallback = callback; }
    
     
    ManualMapResult Map(
        HANDLE processHandle,
        const std::wstring& dllPath,
        ManualMapFlags flags = ManualMapFlags::Default
    );
    
     
    ManualMapResult MapFromMemory(
        HANDLE processHandle,
        const std::vector<BYTE>& dllData,
        ManualMapFlags flags = ManualMapFlags::Default
    );
    
     
    bool Unmap(HANDLE processHandle, LPVOID baseAddress);

private:
     
    bool ParsePEHeaders(const std::vector<BYTE>& data);
    bool ValidatePEHeaders();
    bool Is64BitPE() const;
    
     
    bool AllocateMemory(HANDLE hProcess, ManualMapFlags flags);
    bool CopySections(HANDLE hProcess);
    bool ProcessRelocations(HANDLE hProcess, LPVOID newBase);
    bool ResolveImports(HANDLE hProcess);
    bool HandleTLSCallbacks(HANDLE hProcess);
    bool SetSectionProtections(HANDLE hProcess);
    bool ExecuteDllMain(HANDLE hProcess, DWORD reason);
    bool CleanupHeaders(HANDLE hProcess, ManualMapFlags flags);
    
     
    HMODULE GetRemoteModuleHandle(HANDLE hProcess, const std::string& moduleName);
    FARPROC GetRemoteProcAddress(HANDLE hProcess, HMODULE hModule, const std::string& funcName);
    bool LoadRemoteModule(HANDLE hProcess, const std::string& moduleName);
    
     
    void Log(LogLevel level, const std::wstring& message);
    std::wstring GetLastErrorMessage();
    
     
    std::vector<BYTE> m_dllData;
    PIMAGE_DOS_HEADER m_dosHeader;
    PIMAGE_NT_HEADERS m_ntHeaders;
    std::vector<SectionInfo> m_sections;
    std::vector<ImportDescriptor> m_imports;
    std::vector<RelocationEntry> m_relocations;
    
     
    LPVOID m_remoteBase;
    SIZE_T m_imageSize;
    
     
    LogCallback m_logCallback;
};

 
class ShellcodeGenerator {
public:
     
    static std::vector<BYTE> GenerateDllMainCaller64(
        LPVOID dllBase,
        LPVOID entryPoint,
        DWORD reason
    );
    
     
    static std::vector<BYTE> GenerateDllMainCaller32(
        LPVOID dllBase,
        LPVOID entryPoint,
        DWORD reason
    );
    
     
    static std::vector<BYTE> GenerateTLSCaller(
        LPVOID dllBase,
        const std::vector<LPVOID>& callbacks,
        bool is64Bit
    );
};

}  
