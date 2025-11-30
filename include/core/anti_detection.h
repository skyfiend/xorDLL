#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <functional>

namespace xordll {

 
enum class AntiDetectTechnique {
    None = 0,
    UnlinkFromPEB = 1 << 0,          
    EraseHeaders = 1 << 1,            
    HideFromToolhelp = 1 << 2,        
    SpoofModuleName = 1 << 3,         
    RandomizeTimestamp = 1 << 4,      
    ClearDebugInfo = 1 << 5,          
    
     
    Basic = UnlinkFromPEB | EraseHeaders,
    Advanced = UnlinkFromPEB | EraseHeaders | HideFromToolhelp | ClearDebugInfo,
    Maximum = UnlinkFromPEB | EraseHeaders | HideFromToolhelp | 
              SpoofModuleName | RandomizeTimestamp | ClearDebugInfo
};

inline AntiDetectTechnique operator|(AntiDetectTechnique a, AntiDetectTechnique b) {
    return static_cast<AntiDetectTechnique>(static_cast<int>(a) | static_cast<int>(b));
}

inline bool operator&(AntiDetectTechnique a, AntiDetectTechnique b) {
    return (static_cast<int>(a) & static_cast<int>(b)) != 0;
}

 
#pragma pack(push, 1)

typedef struct _UNICODE_STRING_T {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} UNICODE_STRING_T;

typedef struct _LDR_DATA_TABLE_ENTRY_T {
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderLinks;
    LIST_ENTRY InInitializationOrderLinks;
    PVOID DllBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING_T FullDllName;
    UNICODE_STRING_T BaseDllName;
    ULONG Flags;
    USHORT LoadCount;
    USHORT TlsIndex;
    LIST_ENTRY HashLinks;
    ULONG TimeDateStamp;
} LDR_DATA_TABLE_ENTRY_T, *PLDR_DATA_TABLE_ENTRY_T;

typedef struct _PEB_LDR_DATA_T {
    ULONG Length;
    BOOLEAN Initialized;
    PVOID SsHandle;
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
} PEB_LDR_DATA_T, *PPEB_LDR_DATA_T;

#pragma pack(pop)

 
class AntiDetection {
public:
    using LogCallback = std::function<void(const std::wstring&)>;
    
    AntiDetection();
    ~AntiDetection();
    
     
    void SetLogCallback(LogCallback callback) { m_logCallback = callback; }
    
     
    bool Apply(
        HANDLE processHandle,
        LPVOID moduleBase,
        AntiDetectTechnique techniques
    );
    
     
    bool UnlinkFromPEB(HANDLE processHandle, LPVOID moduleBase);
    
     
    bool EraseHeaders(HANDLE processHandle, LPVOID moduleBase);
    
     
    bool SpoofModuleName(HANDLE processHandle, LPVOID moduleBase, const std::wstring& newName);
    
     
    bool ClearDebugDirectory(HANDLE processHandle, LPVOID moduleBase);
    
     
    bool IsModuleHidden(HANDLE processHandle, LPVOID moduleBase);

private:
     
    LPVOID GetRemotePEB(HANDLE processHandle);
    
     
    PLDR_DATA_TABLE_ENTRY_T FindModuleEntry(HANDLE processHandle, LPVOID peb, LPVOID moduleBase);
    
     
    bool UnlinkListEntry(HANDLE processHandle, PLIST_ENTRY entry);
    
    void Log(const std::wstring& message);
    
    LogCallback m_logCallback;
};

 
class MemoryProtection {
public:
     
    class ScopedProtection {
    public:
        ScopedProtection(HANDLE hProcess, LPVOID address, SIZE_T size, DWORD newProtect);
        ~ScopedProtection();
        
        bool IsValid() const { return m_valid; }
        
    private:
        HANDLE m_hProcess;
        LPVOID m_address;
        SIZE_T m_size;
        DWORD m_oldProtect;
        bool m_valid;
    };
    
     
    static bool MakeWritable(HANDLE hProcess, LPVOID address, SIZE_T size, DWORD& oldProtect);
    
     
    static bool RestoreProtection(HANDLE hProcess, LPVOID address, SIZE_T size, DWORD protect);
};

 
class ProcessEnvironment {
public:
     
    static LPVOID GetPEB(HANDLE hProcess);
    
     
    static LPVOID GetTEB(HANDLE hThread);
    
     
    static bool ReadLoaderData(HANDLE hProcess, LPVOID peb, PEB_LDR_DATA_T& ldrData);
};

}  
