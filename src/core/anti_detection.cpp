 

#include "core/anti_detection.h"
#include <winternl.h>

#pragma comment(lib, "ntdll.lib")

namespace xordll {

 
typedef NTSTATUS(NTAPI* NtQueryInformationProcessFn)(
    HANDLE ProcessHandle,
    PROCESSINFOCLASS ProcessInformationClass,
    PVOID ProcessInformation,
    ULONG ProcessInformationLength,
    PULONG ReturnLength
);

 
 
 

AntiDetection::AntiDetection() = default;
AntiDetection::~AntiDetection() = default;

bool AntiDetection::Apply(
    HANDLE processHandle,
    LPVOID moduleBase,
    AntiDetectTechnique techniques
) {
    bool success = true;
    
    if (techniques & AntiDetectTechnique::UnlinkFromPEB) {
        if (!UnlinkFromPEB(processHandle, moduleBase)) {
            Log(L"Failed to unlink from PEB");
            success = false;
        } else {
            Log(L"Module unlinked from PEB");
        }
    }
    
    if (techniques & AntiDetectTechnique::EraseHeaders) {
        if (!EraseHeaders(processHandle, moduleBase)) {
            Log(L"Failed to erase headers");
            success = false;
        } else {
            Log(L"PE headers erased");
        }
    }
    
    if (techniques & AntiDetectTechnique::ClearDebugInfo) {
        if (!ClearDebugDirectory(processHandle, moduleBase)) {
            Log(L"Failed to clear debug directory");
            success = false;
        } else {
            Log(L"Debug directory cleared");
        }
    }
    
    return success;
}

bool AntiDetection::UnlinkFromPEB(HANDLE processHandle, LPVOID moduleBase) {
     
    LPVOID peb = GetRemotePEB(processHandle);
    if (!peb) {
        return false;
    }
    
     
    LPVOID ldrDataPtr = nullptr;
    SIZE_T bytesRead;
    
#ifdef _WIN64
     
    if (!ReadProcessMemory(processHandle, 
        reinterpret_cast<BYTE*>(peb) + 0x18, 
        &ldrDataPtr, sizeof(ldrDataPtr), &bytesRead)) {
        return false;
    }
#else
     
    if (!ReadProcessMemory(processHandle, 
        reinterpret_cast<BYTE*>(peb) + 0x0C, 
        &ldrDataPtr, sizeof(ldrDataPtr), &bytesRead)) {
        return false;
    }
#endif
    
    if (!ldrDataPtr) {
        return false;
    }
    
     
    PEB_LDR_DATA_T ldrData;
    if (!ReadProcessMemory(processHandle, ldrDataPtr, &ldrData, sizeof(ldrData), &bytesRead)) {
        return false;
    }
    
     
    LIST_ENTRY* head = &reinterpret_cast<PPEB_LDR_DATA_T>(ldrDataPtr)->InLoadOrderModuleList;
    LIST_ENTRY currentEntry;
    PLIST_ENTRY current = ldrData.InLoadOrderModuleList.Flink;
    
    while (current != head) {
        if (!ReadProcessMemory(processHandle, current, &currentEntry, sizeof(currentEntry), &bytesRead)) {
            break;
        }
        
         
        PLDR_DATA_TABLE_ENTRY_T entry = CONTAINING_RECORD(current, LDR_DATA_TABLE_ENTRY_T, InLoadOrderLinks);
        
        LDR_DATA_TABLE_ENTRY_T entryData;
        if (!ReadProcessMemory(processHandle, entry, &entryData, sizeof(entryData), &bytesRead)) {
            current = currentEntry.Flink;
            continue;
        }
        
        if (entryData.DllBase == moduleBase) {
             
            
             
            UnlinkListEntry(processHandle, &entry->InLoadOrderLinks);
            
             
            UnlinkListEntry(processHandle, &entry->InMemoryOrderLinks);
            
             
            UnlinkListEntry(processHandle, &entry->InInitializationOrderLinks);
            
             
            UnlinkListEntry(processHandle, &entry->HashLinks);
            
            return true;
        }
        
        current = currentEntry.Flink;
    }
    
    return false;
}

bool AntiDetection::EraseHeaders(HANDLE processHandle, LPVOID moduleBase) {
     
    IMAGE_DOS_HEADER dosHeader;
    SIZE_T bytesRead;
    
    if (!ReadProcessMemory(processHandle, moduleBase, &dosHeader, sizeof(dosHeader), &bytesRead)) {
        return false;
    }
    
    if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
        return false;
    }
    
     
    IMAGE_NT_HEADERS ntHeaders;
    if (!ReadProcessMemory(processHandle, 
        reinterpret_cast<BYTE*>(moduleBase) + dosHeader.e_lfanew,
        &ntHeaders, sizeof(ntHeaders), &bytesRead)) {
        return false;
    }
    
    if (ntHeaders.Signature != IMAGE_NT_SIGNATURE) {
        return false;
    }
    
     
    DWORD headerSize = ntHeaders.OptionalHeader.SizeOfHeaders;
    
     
    DWORD oldProtect;
    if (!VirtualProtectEx(processHandle, moduleBase, headerSize, PAGE_READWRITE, &oldProtect)) {
        return false;
    }
    
     
    std::vector<BYTE> zeros(headerSize, 0);
    SIZE_T bytesWritten;
    bool success = WriteProcessMemory(processHandle, moduleBase, zeros.data(), headerSize, &bytesWritten) != FALSE;
    
     
    VirtualProtectEx(processHandle, moduleBase, headerSize, oldProtect, &oldProtect);
    
    return success;
}

bool AntiDetection::SpoofModuleName(HANDLE processHandle, LPVOID moduleBase, const std::wstring& newName) {
    LPVOID peb = GetRemotePEB(processHandle);
    if (!peb) {
        return false;
    }
    
     
    PLDR_DATA_TABLE_ENTRY_T entry = FindModuleEntry(processHandle, peb, moduleBase);
    if (!entry) {
        return false;
    }
    
     
    LDR_DATA_TABLE_ENTRY_T entryData;
    SIZE_T bytesRead;
    if (!ReadProcessMemory(processHandle, entry, &entryData, sizeof(entryData), &bytesRead)) {
        return false;
    }
    
     
    size_t nameSize = (newName.length() + 1) * sizeof(wchar_t);
    LPVOID remoteName = VirtualAllocEx(processHandle, nullptr, nameSize,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    
    if (!remoteName) {
        return false;
    }
    
     
    SIZE_T bytesWritten;
    if (!WriteProcessMemory(processHandle, remoteName, newName.c_str(), nameSize, &bytesWritten)) {
        VirtualFreeEx(processHandle, remoteName, 0, MEM_RELEASE);
        return false;
    }
    
     
    UNICODE_STRING_T newNameStr;
    newNameStr.Length = static_cast<USHORT>(newName.length() * sizeof(wchar_t));
    newNameStr.MaximumLength = static_cast<USHORT>(nameSize);
    newNameStr.Buffer = reinterpret_cast<PWSTR>(remoteName);
    
     
    if (!WriteProcessMemory(processHandle, &entry->BaseDllName, &newNameStr, sizeof(newNameStr), &bytesWritten)) {
        VirtualFreeEx(processHandle, remoteName, 0, MEM_RELEASE);
        return false;
    }
    
    return true;
}

bool AntiDetection::ClearDebugDirectory(HANDLE processHandle, LPVOID moduleBase) {
     
    IMAGE_DOS_HEADER dosHeader;
    SIZE_T bytesRead;
    
    if (!ReadProcessMemory(processHandle, moduleBase, &dosHeader, sizeof(dosHeader), &bytesRead)) {
        return false;
    }
    
     
    IMAGE_NT_HEADERS ntHeaders;
    if (!ReadProcessMemory(processHandle,
        reinterpret_cast<BYTE*>(moduleBase) + dosHeader.e_lfanew,
        &ntHeaders, sizeof(ntHeaders), &bytesRead)) {
        return false;
    }
    
     
    auto& debugDir = ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
    
    if (debugDir.Size == 0) {
        return true;   
    }
    
     
    IMAGE_DATA_DIRECTORY emptyDir = { 0, 0 };
    LPVOID debugDirAddr = reinterpret_cast<BYTE*>(moduleBase) + dosHeader.e_lfanew +
        offsetof(IMAGE_NT_HEADERS, OptionalHeader) +
        offsetof(IMAGE_OPTIONAL_HEADER, DataDirectory) +
        IMAGE_DIRECTORY_ENTRY_DEBUG * sizeof(IMAGE_DATA_DIRECTORY);
    
    DWORD oldProtect;
    VirtualProtectEx(processHandle, debugDirAddr, sizeof(emptyDir), PAGE_READWRITE, &oldProtect);
    
    SIZE_T bytesWritten;
    bool success = WriteProcessMemory(processHandle, debugDirAddr, &emptyDir, sizeof(emptyDir), &bytesWritten) != FALSE;
    
    VirtualProtectEx(processHandle, debugDirAddr, sizeof(emptyDir), oldProtect, &oldProtect);
    
     
    if (success && debugDir.VirtualAddress) {
        LPVOID debugDataAddr = reinterpret_cast<BYTE*>(moduleBase) + debugDir.VirtualAddress;
        std::vector<BYTE> zeros(debugDir.Size, 0);
        
        VirtualProtectEx(processHandle, debugDataAddr, debugDir.Size, PAGE_READWRITE, &oldProtect);
        WriteProcessMemory(processHandle, debugDataAddr, zeros.data(), debugDir.Size, &bytesWritten);
        VirtualProtectEx(processHandle, debugDataAddr, debugDir.Size, oldProtect, &oldProtect);
    }
    
    return success;
}

bool AntiDetection::IsModuleHidden(HANDLE processHandle, LPVOID moduleBase) {
    LPVOID peb = GetRemotePEB(processHandle);
    if (!peb) {
        return false;
    }
    
    return FindModuleEntry(processHandle, peb, moduleBase) == nullptr;
}

LPVOID AntiDetection::GetRemotePEB(HANDLE processHandle) {
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtdll) {
        return nullptr;
    }
    
    auto NtQueryInformationProcess = reinterpret_cast<NtQueryInformationProcessFn>(
        GetProcAddress(hNtdll, "NtQueryInformationProcess"));
    
    if (!NtQueryInformationProcess) {
        return nullptr;
    }
    
    PROCESS_BASIC_INFORMATION pbi;
    ULONG returnLength;
    
    NTSTATUS status = NtQueryInformationProcess(
        processHandle,
        ProcessBasicInformation,
        &pbi,
        sizeof(pbi),
        &returnLength
    );
    
    if (status != 0) {
        return nullptr;
    }
    
    return pbi.PebBaseAddress;
}

PLDR_DATA_TABLE_ENTRY_T AntiDetection::FindModuleEntry(HANDLE processHandle, LPVOID peb, LPVOID moduleBase) {
     
    LPVOID ldrDataPtr = nullptr;
    SIZE_T bytesRead;
    
#ifdef _WIN64
    if (!ReadProcessMemory(processHandle,
        reinterpret_cast<BYTE*>(peb) + 0x18,
        &ldrDataPtr, sizeof(ldrDataPtr), &bytesRead)) {
        return nullptr;
    }
#else
    if (!ReadProcessMemory(processHandle,
        reinterpret_cast<BYTE*>(peb) + 0x0C,
        &ldrDataPtr, sizeof(ldrDataPtr), &bytesRead)) {
        return nullptr;
    }
#endif
    
    if (!ldrDataPtr) {
        return nullptr;
    }
    
     
    PEB_LDR_DATA_T ldrData;
    if (!ReadProcessMemory(processHandle, ldrDataPtr, &ldrData, sizeof(ldrData), &bytesRead)) {
        return nullptr;
    }
    
     
    LIST_ENTRY* head = &reinterpret_cast<PPEB_LDR_DATA_T>(ldrDataPtr)->InLoadOrderModuleList;
    LIST_ENTRY currentEntry;
    PLIST_ENTRY current = ldrData.InLoadOrderModuleList.Flink;
    
    while (current != head) {
        if (!ReadProcessMemory(processHandle, current, &currentEntry, sizeof(currentEntry), &bytesRead)) {
            break;
        }
        
        PLDR_DATA_TABLE_ENTRY_T entry = CONTAINING_RECORD(current, LDR_DATA_TABLE_ENTRY_T, InLoadOrderLinks);
        
        LDR_DATA_TABLE_ENTRY_T entryData;
        if (!ReadProcessMemory(processHandle, entry, &entryData, sizeof(entryData), &bytesRead)) {
            current = currentEntry.Flink;
            continue;
        }
        
        if (entryData.DllBase == moduleBase) {
            return entry;
        }
        
        current = currentEntry.Flink;
    }
    
    return nullptr;
}

bool AntiDetection::UnlinkListEntry(HANDLE processHandle, PLIST_ENTRY entry) {
    LIST_ENTRY currentEntry;
    SIZE_T bytesRead;
    
    if (!ReadProcessMemory(processHandle, entry, &currentEntry, sizeof(currentEntry), &bytesRead)) {
        return false;
    }
    
     
    SIZE_T bytesWritten;
    if (!WriteProcessMemory(processHandle,
        &currentEntry.Blink->Flink,
        &currentEntry.Flink,
        sizeof(PLIST_ENTRY),
        &bytesWritten)) {
        return false;
    }
    
     
    if (!WriteProcessMemory(processHandle,
        &currentEntry.Flink->Blink,
        &currentEntry.Blink,
        sizeof(PLIST_ENTRY),
        &bytesWritten)) {
        return false;
    }
    
    return true;
}

void AntiDetection::Log(const std::wstring& message) {
    if (m_logCallback) {
        m_logCallback(L"[AntiDetect] " + message);
    }
}

 
 
 

MemoryProtection::ScopedProtection::ScopedProtection(
    HANDLE hProcess, LPVOID address, SIZE_T size, DWORD newProtect
)
    : m_hProcess(hProcess)
    , m_address(address)
    , m_size(size)
    , m_oldProtect(0)
    , m_valid(false)
{
    m_valid = VirtualProtectEx(hProcess, address, size, newProtect, &m_oldProtect) != FALSE;
}

MemoryProtection::ScopedProtection::~ScopedProtection() {
    if (m_valid) {
        DWORD temp;
        VirtualProtectEx(m_hProcess, m_address, m_size, m_oldProtect, &temp);
    }
}

bool MemoryProtection::MakeWritable(HANDLE hProcess, LPVOID address, SIZE_T size, DWORD& oldProtect) {
    return VirtualProtectEx(hProcess, address, size, PAGE_READWRITE, &oldProtect) != FALSE;
}

bool MemoryProtection::RestoreProtection(HANDLE hProcess, LPVOID address, SIZE_T size, DWORD protect) {
    DWORD temp;
    return VirtualProtectEx(hProcess, address, size, protect, &temp) != FALSE;
}

 
 
 

LPVOID ProcessEnvironment::GetPEB(HANDLE hProcess) {
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtdll) return nullptr;
    
    auto NtQueryInformationProcess = reinterpret_cast<NtQueryInformationProcessFn>(
        GetProcAddress(hNtdll, "NtQueryInformationProcess"));
    
    if (!NtQueryInformationProcess) return nullptr;
    
    PROCESS_BASIC_INFORMATION pbi;
    ULONG returnLength;
    
    if (NtQueryInformationProcess(hProcess, ProcessBasicInformation, &pbi, sizeof(pbi), &returnLength) != 0) {
        return nullptr;
    }
    
    return pbi.PebBaseAddress;
}

LPVOID ProcessEnvironment::GetTEB(HANDLE hThread) {
     
    return nullptr;
}

bool ProcessEnvironment::ReadLoaderData(HANDLE hProcess, LPVOID peb, PEB_LDR_DATA_T& ldrData) {
    LPVOID ldrDataPtr = nullptr;
    SIZE_T bytesRead;
    
#ifdef _WIN64
    if (!ReadProcessMemory(hProcess, reinterpret_cast<BYTE*>(peb) + 0x18,
        &ldrDataPtr, sizeof(ldrDataPtr), &bytesRead)) {
        return false;
    }
#else
    if (!ReadProcessMemory(hProcess, reinterpret_cast<BYTE*>(peb) + 0x0C,
        &ldrDataPtr, sizeof(ldrDataPtr), &bytesRead)) {
        return false;
    }
#endif
    
    return ReadProcessMemory(hProcess, ldrDataPtr, &ldrData, sizeof(ldrData), &bytesRead) != FALSE;
}

}  
