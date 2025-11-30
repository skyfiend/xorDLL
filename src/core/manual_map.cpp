 

#include "core/manual_map.h"
#include "utils/string_utils.h"
#include "utils/file_utils.h"
#include <tlhelp32.h>

namespace xordll {

 
 
 

ManualMapper::ManualMapper()
    : m_dosHeader(nullptr)
    , m_ntHeaders(nullptr)
    , m_remoteBase(nullptr)
    , m_imageSize(0)
{
}

ManualMapper::~ManualMapper() = default;

ManualMapResult ManualMapper::Map(
    HANDLE processHandle,
    const std::wstring& dllPath,
    ManualMapFlags flags
) {
    ManualMapResult result = { false, nullptr, 0, L"", 0 };
    
     
    std::vector<BYTE> dllData;
    if (!utils::ReadFileToMemory(dllPath, dllData)) {
        result.errorMessage = L"Failed to read DLL file: " + dllPath;
        result.errorCode = GetLastError();
        Log(LogLevel::Error, result.errorMessage);
        return result;
    }
    
    Log(LogLevel::Info, L"Read DLL file: " + std::to_wstring(dllData.size()) + L" bytes");
    
    return MapFromMemory(processHandle, dllData, flags);
}

ManualMapResult ManualMapper::MapFromMemory(
    HANDLE processHandle,
    const std::vector<BYTE>& dllData,
    ManualMapFlags flags
) {
    ManualMapResult result = { false, nullptr, 0, L"", 0 };
    
    m_dllData = dllData;
    
     
    if (!ParsePEHeaders(m_dllData)) {
        result.errorMessage = L"Failed to parse PE headers";
        result.errorCode = ERROR_BAD_FORMAT;
        Log(LogLevel::Error, result.errorMessage);
        return result;
    }
    
     
    if (!ValidatePEHeaders()) {
        result.errorMessage = L"Invalid PE file";
        result.errorCode = ERROR_BAD_FORMAT;
        Log(LogLevel::Error, result.errorMessage);
        return result;
    }
    
    Log(LogLevel::Info, L"PE headers validated, image size: " + 
        std::to_wstring(m_ntHeaders->OptionalHeader.SizeOfImage) + L" bytes");
    
     
    if (!AllocateMemory(processHandle, flags)) {
        result.errorMessage = L"Failed to allocate memory in target process";
        result.errorCode = GetLastError();
        Log(LogLevel::Error, result.errorMessage);
        return result;
    }
    
    Log(LogLevel::Info, L"Allocated memory at: 0x" + 
        utils::Utf8ToWide(std::to_string(reinterpret_cast<uintptr_t>(m_remoteBase))));
    
     
    if (!CopySections(processHandle)) {
        result.errorMessage = L"Failed to copy sections";
        result.errorCode = GetLastError();
        VirtualFreeEx(processHandle, m_remoteBase, 0, MEM_RELEASE);
        Log(LogLevel::Error, result.errorMessage);
        return result;
    }
    
    Log(LogLevel::Debug, L"Sections copied successfully");
    
     
    if (!ProcessRelocations(processHandle, m_remoteBase)) {
        result.errorMessage = L"Failed to process relocations";
        result.errorCode = GetLastError();
        VirtualFreeEx(processHandle, m_remoteBase, 0, MEM_RELEASE);
        Log(LogLevel::Error, result.errorMessage);
        return result;
    }
    
    Log(LogLevel::Debug, L"Relocations processed");
    
     
    if (!ResolveImports(processHandle)) {
        result.errorMessage = L"Failed to resolve imports";
        result.errorCode = GetLastError();
        VirtualFreeEx(processHandle, m_remoteBase, 0, MEM_RELEASE);
        Log(LogLevel::Error, result.errorMessage);
        return result;
    }
    
    Log(LogLevel::Debug, L"Imports resolved");
    
     
    if (flags & ManualMapFlags::HandleTLS) {
        if (!HandleTLSCallbacks(processHandle)) {
            Log(LogLevel::Warning, L"TLS callback handling failed (non-fatal)");
        }
    }
    
     
    if (flags & ManualMapFlags::AdjustProtections) {
        if (!SetSectionProtections(processHandle)) {
            Log(LogLevel::Warning, L"Failed to set section protections (non-fatal)");
        }
    }
    
     
    if (!ExecuteDllMain(processHandle, DLL_PROCESS_ATTACH)) {
        result.errorMessage = L"Failed to execute DllMain";
        result.errorCode = GetLastError();
        VirtualFreeEx(processHandle, m_remoteBase, 0, MEM_RELEASE);
        Log(LogLevel::Error, result.errorMessage);
        return result;
    }
    
    Log(LogLevel::Info, L"DllMain executed successfully");
    
     
    if (flags & ManualMapFlags::ClearHeader) {
        CleanupHeaders(processHandle, flags);
        Log(LogLevel::Debug, L"Headers cleared");
    }
    
    result.success = true;
    result.baseAddress = m_remoteBase;
    result.mappedSize = m_imageSize;
    
    Log(LogLevel::Info, L"Manual mapping completed successfully");
    
    return result;
}

bool ManualMapper::Unmap(HANDLE processHandle, LPVOID baseAddress) {
     
     
    
    return VirtualFreeEx(processHandle, baseAddress, 0, MEM_RELEASE) != FALSE;
}

 
 
 

bool ManualMapper::ParsePEHeaders(const std::vector<BYTE>& data) {
    if (data.size() < sizeof(IMAGE_DOS_HEADER)) {
        return false;
    }
    
    m_dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(const_cast<BYTE*>(data.data()));
    
    if (m_dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        return false;
    }
    
    if (data.size() < static_cast<size_t>(m_dosHeader->e_lfanew) + sizeof(IMAGE_NT_HEADERS)) {
        return false;
    }
    
    m_ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(
        const_cast<BYTE*>(data.data()) + m_dosHeader->e_lfanew);
    
    if (m_ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
        return false;
    }
    
     
    m_sections.clear();
    auto sectionHeader = IMAGE_FIRST_SECTION(m_ntHeaders);
    
    for (WORD i = 0; i < m_ntHeaders->FileHeader.NumberOfSections; i++) {
        SectionInfo section;
        section.name = std::string(reinterpret_cast<char*>(sectionHeader[i].Name), 8);
        section.virtualAddress = sectionHeader[i].VirtualAddress;
        section.virtualSize = sectionHeader[i].Misc.VirtualSize;
        section.rawDataOffset = sectionHeader[i].PointerToRawData;
        section.rawDataSize = sectionHeader[i].SizeOfRawData;
        section.characteristics = sectionHeader[i].Characteristics;
        m_sections.push_back(section);
    }
    
    m_imageSize = m_ntHeaders->OptionalHeader.SizeOfImage;
    
    return true;
}

bool ManualMapper::ValidatePEHeaders() {
    if (!m_dosHeader || !m_ntHeaders) return false;
    
     
#ifdef _WIN64
    if (m_ntHeaders->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64) {
        Log(LogLevel::Error, L"DLL architecture mismatch: expected x64");
        return false;
    }
#else
    if (m_ntHeaders->FileHeader.Machine != IMAGE_FILE_MACHINE_I386) {
        Log(LogLevel::Error, L"DLL architecture mismatch: expected x86");
        return false;
    }
#endif
    
     
    if (!(m_ntHeaders->FileHeader.Characteristics & IMAGE_FILE_DLL)) {
        Log(LogLevel::Error, L"File is not a DLL");
        return false;
    }
    
    return true;
}

bool ManualMapper::Is64BitPE() const {
    return m_ntHeaders && m_ntHeaders->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64;
}

 
 
 

bool ManualMapper::AllocateMemory(HANDLE hProcess, ManualMapFlags flags) {
    LPVOID preferredBase = reinterpret_cast<LPVOID>(
        m_ntHeaders->OptionalHeader.ImageBase);
    
     
    m_remoteBase = VirtualAllocEx(
        hProcess,
        preferredBase,
        m_imageSize,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    );
    
     
    if (!m_remoteBase) {
        m_remoteBase = VirtualAllocEx(
            hProcess,
            nullptr,
            m_imageSize,
            MEM_COMMIT | MEM_RESERVE,
            PAGE_EXECUTE_READWRITE
        );
    }
    
    return m_remoteBase != nullptr;
}

bool ManualMapper::CopySections(HANDLE hProcess) {
     
    SIZE_T written;
    if (!WriteProcessMemory(hProcess, m_remoteBase, m_dllData.data(),
        m_ntHeaders->OptionalHeader.SizeOfHeaders, &written)) {
        return false;
    }
    
     
    for (const auto& section : m_sections) {
        if (section.rawDataSize == 0) continue;
        
        LPVOID sectionDest = reinterpret_cast<BYTE*>(m_remoteBase) + section.virtualAddress;
        LPCVOID sectionSrc = m_dllData.data() + section.rawDataOffset;
        
        if (!WriteProcessMemory(hProcess, sectionDest, sectionSrc, section.rawDataSize, &written)) {
            Log(LogLevel::Error, L"Failed to copy section: " + utils::Utf8ToWide(section.name));
            return false;
        }
    }
    
    return true;
}

bool ManualMapper::ProcessRelocations(HANDLE hProcess, LPVOID newBase) {
    auto& dataDir = m_ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    
    if (dataDir.Size == 0) {
         
        return true;
    }
    
    ULONGLONG delta = reinterpret_cast<ULONGLONG>(newBase) - 
                      m_ntHeaders->OptionalHeader.ImageBase;
    
    if (delta == 0) {
         
        return true;
    }
    
    auto relocBase = reinterpret_cast<PIMAGE_BASE_RELOCATION>(
        m_dllData.data() + dataDir.VirtualAddress);
    
    while (relocBase->VirtualAddress) {
        DWORD numEntries = (relocBase->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
        auto relocEntry = reinterpret_cast<WORD*>(relocBase + 1);
        
        for (DWORD i = 0; i < numEntries; i++) {
            WORD type = relocEntry[i] >> 12;
            WORD offset = relocEntry[i] & 0xFFF;
            
            if (type == IMAGE_REL_BASED_ABSOLUTE) continue;
            
            LPVOID patchAddr = reinterpret_cast<BYTE*>(m_remoteBase) + 
                               relocBase->VirtualAddress + offset;
            
            if (type == IMAGE_REL_BASED_HIGHLOW || type == IMAGE_REL_BASED_DIR64) {
                 
                ULONGLONG value = 0;
                SIZE_T bytesRead;
                size_t valueSize = (type == IMAGE_REL_BASED_DIR64) ? 8 : 4;
                
                if (!ReadProcessMemory(hProcess, patchAddr, &value, valueSize, &bytesRead)) {
                    continue;
                }
                
                 
                value += delta;
                
                 
                SIZE_T bytesWritten;
                WriteProcessMemory(hProcess, patchAddr, &value, valueSize, &bytesWritten);
            }
        }
        
        relocBase = reinterpret_cast<PIMAGE_BASE_RELOCATION>(
            reinterpret_cast<BYTE*>(relocBase) + relocBase->SizeOfBlock);
    }
    
    return true;
}

bool ManualMapper::ResolveImports(HANDLE hProcess) {
    auto& dataDir = m_ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    
    if (dataDir.Size == 0) {
        return true;   
    }
    
    auto importDesc = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(
        m_dllData.data() + dataDir.VirtualAddress);
    
    while (importDesc->Name) {
        const char* moduleName = reinterpret_cast<const char*>(
            m_dllData.data() + importDesc->Name);
        
        Log(LogLevel::Debug, L"Resolving imports from: " + utils::Utf8ToWide(moduleName));
        
         
        HMODULE hRemoteModule = GetRemoteModuleHandle(hProcess, moduleName);
        
        if (!hRemoteModule) {
             
            if (!LoadRemoteModule(hProcess, moduleName)) {
                Log(LogLevel::Error, L"Failed to load module: " + utils::Utf8ToWide(moduleName));
                return false;
            }
            hRemoteModule = GetRemoteModuleHandle(hProcess, moduleName);
        }
        
        if (!hRemoteModule) {
            Log(LogLevel::Error, L"Module not found: " + utils::Utf8ToWide(moduleName));
            return false;
        }
        
         
        auto thunk = reinterpret_cast<PIMAGE_THUNK_DATA>(
            m_dllData.data() + importDesc->FirstThunk);
        auto origThunk = importDesc->OriginalFirstThunk ?
            reinterpret_cast<PIMAGE_THUNK_DATA>(m_dllData.data() + importDesc->OriginalFirstThunk) :
            thunk;
        
        LPVOID iatEntry = reinterpret_cast<BYTE*>(m_remoteBase) + importDesc->FirstThunk;
        
        while (origThunk->u1.AddressOfData) {
            FARPROC funcAddr = nullptr;
            
            if (IMAGE_SNAP_BY_ORDINAL(origThunk->u1.Ordinal)) {
                 
                WORD ordinal = IMAGE_ORDINAL(origThunk->u1.Ordinal);
                funcAddr = GetRemoteProcAddress(hProcess, hRemoteModule, 
                    reinterpret_cast<const char*>(ordinal));
            } else {
                 
                auto importByName = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(
                    m_dllData.data() + origThunk->u1.AddressOfData);
                funcAddr = GetRemoteProcAddress(hProcess, hRemoteModule, importByName->Name);
            }
            
            if (!funcAddr) {
                Log(LogLevel::Warning, L"Failed to resolve import function");
            }
            
             
            SIZE_T written;
            WriteProcessMemory(hProcess, iatEntry, &funcAddr, sizeof(funcAddr), &written);
            
            thunk++;
            origThunk++;
            iatEntry = reinterpret_cast<BYTE*>(iatEntry) + sizeof(LPVOID);
        }
        
        importDesc++;
    }
    
    return true;
}

bool ManualMapper::HandleTLSCallbacks(HANDLE hProcess) {
    auto& dataDir = m_ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];
    
    if (dataDir.Size == 0) {
        return true;   
    }
    
     
     
    Log(LogLevel::Debug, L"TLS directory found, callbacks may need handling");
    
    return true;
}

bool ManualMapper::SetSectionProtections(HANDLE hProcess) {
    for (const auto& section : m_sections) {
        DWORD protection = PAGE_READONLY;
        
        if (section.characteristics & IMAGE_SCN_MEM_EXECUTE) {
            if (section.characteristics & IMAGE_SCN_MEM_WRITE) {
                protection = PAGE_EXECUTE_READWRITE;
            } else {
                protection = PAGE_EXECUTE_READ;
            }
        } else if (section.characteristics & IMAGE_SCN_MEM_WRITE) {
            protection = PAGE_READWRITE;
        }
        
        LPVOID sectionAddr = reinterpret_cast<BYTE*>(m_remoteBase) + section.virtualAddress;
        DWORD oldProtect;
        
        VirtualProtectEx(hProcess, sectionAddr, section.virtualSize, protection, &oldProtect);
    }
    
    return true;
}

bool ManualMapper::ExecuteDllMain(HANDLE hProcess, DWORD reason) {
    LPVOID entryPoint = reinterpret_cast<BYTE*>(m_remoteBase) + 
                        m_ntHeaders->OptionalHeader.AddressOfEntryPoint;
    
    if (m_ntHeaders->OptionalHeader.AddressOfEntryPoint == 0) {
        Log(LogLevel::Debug, L"No entry point, skipping DllMain");
        return true;
    }
    
     
    std::vector<BYTE> shellcode;
    
#ifdef _WIN64
    shellcode = ShellcodeGenerator::GenerateDllMainCaller64(m_remoteBase, entryPoint, reason);
#else
    shellcode = ShellcodeGenerator::GenerateDllMainCaller32(m_remoteBase, entryPoint, reason);
#endif
    
     
    LPVOID shellcodeAddr = VirtualAllocEx(hProcess, nullptr, shellcode.size(),
        MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    
    if (!shellcodeAddr) {
        return false;
    }
    
     
    SIZE_T written;
    if (!WriteProcessMemory(hProcess, shellcodeAddr, shellcode.data(), shellcode.size(), &written)) {
        VirtualFreeEx(hProcess, shellcodeAddr, 0, MEM_RELEASE);
        return false;
    }
    
     
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(shellcodeAddr), nullptr, 0, nullptr);
    
    if (!hThread) {
        VirtualFreeEx(hProcess, shellcodeAddr, 0, MEM_RELEASE);
        return false;
    }
    
     
    WaitForSingleObject(hThread, 5000);
    
    DWORD exitCode;
    GetExitCodeThread(hThread, &exitCode);
    
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, shellcodeAddr, 0, MEM_RELEASE);
    
    return exitCode != 0;
}

bool ManualMapper::CleanupHeaders(HANDLE hProcess, ManualMapFlags flags) {
     
    std::vector<BYTE> zeros(m_ntHeaders->OptionalHeader.SizeOfHeaders, 0);
    SIZE_T written;
    
    return WriteProcessMemory(hProcess, m_remoteBase, zeros.data(), zeros.size(), &written) != FALSE;
}

 
 
 

HMODULE ManualMapper::GetRemoteModuleHandle(HANDLE hProcess, const std::string& moduleName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32,
        GetProcessId(hProcess));
    
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return nullptr;
    }
    
    MODULEENTRY32W me = { sizeof(me) };
    HMODULE result = nullptr;
    
    if (Module32FirstW(hSnapshot, &me)) {
        do {
            std::wstring modName = me.szModule;
            if (_wcsicmp(modName.c_str(), utils::Utf8ToWide(moduleName).c_str()) == 0) {
                result = me.hModule;
                break;
            }
        } while (Module32NextW(hSnapshot, &me));
    }
    
    CloseHandle(hSnapshot);
    return result;
}

FARPROC ManualMapper::GetRemoteProcAddress(HANDLE hProcess, HMODULE hModule, const std::string& funcName) {
     
     
    
     
    HMODULE hLocalModule = GetModuleHandleA(nullptr);
    
     
    FARPROC localFunc = GetProcAddress(hModule, funcName.c_str());
    
    return localFunc;
}

bool ManualMapper::LoadRemoteModule(HANDLE hProcess, const std::string& moduleName) {
     
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    FARPROC pLoadLibrary = GetProcAddress(hKernel32, "LoadLibraryA");
    
    if (!pLoadLibrary) return false;
    
     
    SIZE_T nameLen = moduleName.length() + 1;
    LPVOID remoteName = VirtualAllocEx(hProcess, nullptr, nameLen,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    
    if (!remoteName) return false;
    
     
    SIZE_T written;
    WriteProcessMemory(hProcess, remoteName, moduleName.c_str(), nameLen, &written);
    
     
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(pLoadLibrary), remoteName, 0, nullptr);
    
    if (!hThread) {
        VirtualFreeEx(hProcess, remoteName, 0, MEM_RELEASE);
        return false;
    }
    
    WaitForSingleObject(hThread, 5000);
    
    DWORD exitCode;
    GetExitCodeThread(hThread, &exitCode);
    
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, remoteName, 0, MEM_RELEASE);
    
    return exitCode != 0;
}

 
 
 

void ManualMapper::Log(LogLevel level, const std::wstring& message) {
    if (m_logCallback) {
        m_logCallback(level, L"[ManualMap] " + message);
    }
}

std::wstring ManualMapper::GetLastErrorMessage() {
    return utils::FormatWindowsError(GetLastError());
}

 
 
 

std::vector<BYTE> ShellcodeGenerator::GenerateDllMainCaller64(
    LPVOID dllBase,
    LPVOID entryPoint,
    DWORD reason
) {
    std::vector<BYTE> shellcode;
    
     
     
     
     
     
     
     
     
     
    
    shellcode = {
        0x48, 0x83, 0xEC, 0x28,                          
        0x48, 0xB9                                        
    };
    
     
    auto base = reinterpret_cast<ULONGLONG>(dllBase);
    for (int i = 0; i < 8; i++) {
        shellcode.push_back(static_cast<BYTE>(base >> (i * 8)));
    }
    
    shellcode.push_back(0xBA);   
    for (int i = 0; i < 4; i++) {
        shellcode.push_back(static_cast<BYTE>(reason >> (i * 8)));
    }
    
    shellcode.push_back(0x4D);
    shellcode.push_back(0x31);
    shellcode.push_back(0xC0);   
    
    shellcode.push_back(0x48);
    shellcode.push_back(0xB8);   
    
    auto entry = reinterpret_cast<ULONGLONG>(entryPoint);
    for (int i = 0; i < 8; i++) {
        shellcode.push_back(static_cast<BYTE>(entry >> (i * 8)));
    }
    
    shellcode.push_back(0xFF);
    shellcode.push_back(0xD0);   
    
    shellcode.push_back(0x48);
    shellcode.push_back(0x83);
    shellcode.push_back(0xC4);
    shellcode.push_back(0x28);   
    
    shellcode.push_back(0xC3);   
    
    return shellcode;
}

std::vector<BYTE> ShellcodeGenerator::GenerateDllMainCaller32(
    LPVOID dllBase,
    LPVOID entryPoint,
    DWORD reason
) {
    std::vector<BYTE> shellcode;
    
     
     
     
     
     
     
     
    
    shellcode = {
        0x6A, 0x00,                                      
        0x68                                             
    };
    
     
    for (int i = 0; i < 4; i++) {
        shellcode.push_back(static_cast<BYTE>(reason >> (i * 8)));
    }
    
    shellcode.push_back(0x68);   
    auto base = static_cast<DWORD>(reinterpret_cast<uintptr_t>(dllBase));
    for (int i = 0; i < 4; i++) {
        shellcode.push_back(static_cast<BYTE>(base >> (i * 8)));
    }
    
    shellcode.push_back(0xB8);   
    auto entry = static_cast<DWORD>(reinterpret_cast<uintptr_t>(entryPoint));
    for (int i = 0; i < 4; i++) {
        shellcode.push_back(static_cast<BYTE>(entry >> (i * 8)));
    }
    
    shellcode.push_back(0xFF);
    shellcode.push_back(0xD0);   
    
    shellcode.push_back(0xC3);   
    
    return shellcode;
}

std::vector<BYTE> ShellcodeGenerator::GenerateTLSCaller(
    LPVOID dllBase,
    const std::vector<LPVOID>& callbacks,
    bool is64Bit
) {
     
    return std::vector<BYTE>();
}

}  
