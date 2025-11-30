 

#include "cli/command_line.h"
#include "utils/logger.h"
#include <windows.h>
#include <iostream>
#include <io.h>
#include <fcntl.h>

using namespace xordll;

 
bool EnableDebugPrivilege() {
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        return false;
    }
    
    TOKEN_PRIVILEGES tp;
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    
    if (!LookupPrivilegeValueW(nullptr, SE_DEBUG_NAME, &tp.Privileges[0].Luid)) {
        CloseHandle(hToken);
        return false;
    }
    
    bool result = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr) != FALSE;
    CloseHandle(hToken);
    
    return result && GetLastError() != ERROR_NOT_ALL_ASSIGNED;
}

 
void InitializeConsole() {
     
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stderr), _O_U16TEXT);
    
     
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}

 
int wmain(int argc, wchar_t* argv[]) {
    InitializeConsole();
    
     
    Logger::Instance().Initialize(L"xorDLL_cli.log");
    Logger::Instance().SetMinLevel(LogLevel::Warning);
    
     
    if (!EnableDebugPrivilege()) {
        cli::Console::Warning(L"Failed to enable debug privilege. Some operations may fail.");
    }
    
     
    cli::CommandLine cli;
    int result = cli.Run(argc, argv);
    
     
    Logger::Instance().Shutdown();
    
    return result;
}

 
int main(int argc, char* argv[]) {
     
    std::vector<std::wstring> wideArgs;
    std::vector<wchar_t*> wargv;
    
    for (int i = 0; i < argc; i++) {
        int len = MultiByteToWideChar(CP_UTF8, 0, argv[i], -1, nullptr, 0);
        std::wstring warg(len, 0);
        MultiByteToWideChar(CP_UTF8, 0, argv[i], -1, &warg[0], len);
        wideArgs.push_back(warg);
    }
    
    for (auto& arg : wideArgs) {
        wargv.push_back(&arg[0]);
    }
    
    return wmain(argc, wargv.data());
}
