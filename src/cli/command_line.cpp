 

#include "cli/command_line.h"
#include "core/injection_core.h"
#include "core/process_manager.h"
#include "core/dll_loader.h"
#include "core/injection_profile.h"
#include "core/process_monitor.h"
#include "utils/string_utils.h"
#include "version.h"
#include <iostream>
#include <iomanip>

namespace xordll {
namespace cli {

bool Console::s_colorsEnabled = true;

 
 
 

CommandLine::CommandLine() {
    RegisterCommands();
}

CommandLine::~CommandLine() = default;

void CommandLine::RegisterCommands() {
     
    m_globalArgs = {
        { L"help", L"h", L"Show help message", false, false, L"" },
        { L"version", L"v", L"Show version information", false, false, L"" },
        { L"quiet", L"q", L"Suppress output", false, false, L"" },
        { L"no-color", L"", L"Disable colored output", false, false, L"" }
    };
    
     
    m_commands[L"inject"] = {
        L"inject",
        L"Inject a DLL into a process",
        {
            { L"pid", L"p", L"Target process ID", false, true, L"" },
            { L"name", L"n", L"Target process name", false, true, L"" },
            { L"dll", L"d", L"Path to DLL file", true, true, L"" },
            { L"method", L"m", L"Injection method (crt, ntcrt, apc, manual, hijack)", false, true, L"crt" },
            { L"wait", L"w", L"Wait for process to start", false, false, L"" },
            { L"delay", L"", L"Delay before injection (ms)", false, true, L"0" }
        },
        [this](const ParsedOptions& opts) { return HandleInject(opts); }
    };
    
     
    m_commands[L"eject"] = {
        L"eject",
        L"Eject a DLL from a process",
        {
            { L"pid", L"p", L"Target process ID", true, true, L"" },
            { L"dll", L"d", L"DLL name or handle", true, true, L"" }
        },
        [this](const ParsedOptions& opts) { return HandleEject(opts); }
    };
    
     
    m_commands[L"list"] = {
        L"list",
        L"List processes or modules",
        {
            { L"filter", L"f", L"Filter by name", false, true, L"" },
            { L"modules", L"m", L"List modules for process", false, true, L"" },
            { L"x64", L"", L"Show only x64 processes", false, false, L"" },
            { L"x86", L"", L"Show only x86 processes", false, false, L"" }
        },
        [this](const ParsedOptions& opts) { return HandleList(opts); }
    };
    
     
    m_commands[L"info"] = {
        L"info",
        L"Show information about a process or DLL",
        {
            { L"pid", L"p", L"Process ID", false, true, L"" },
            { L"dll", L"d", L"DLL path", false, true, L"" }
        },
        [this](const ParsedOptions& opts) { return HandleInfo(opts); }
    };
    
     
    m_commands[L"profile"] = {
        L"profile",
        L"Manage injection profiles",
        {
            { L"list", L"l", L"List all profiles", false, false, L"" },
            { L"run", L"r", L"Run a profile by name", false, true, L"" },
            { L"export", L"e", L"Export profile to file", false, true, L"" },
            { L"import", L"i", L"Import profile from file", false, true, L"" }
        },
        [this](const ParsedOptions& opts) { return HandleProfile(opts); }
    };
    
     
    m_commands[L"monitor"] = {
        L"monitor",
        L"Monitor processes for auto-injection",
        {
            { L"process", L"p", L"Process name to watch", true, true, L"" },
            { L"dll", L"d", L"DLL to inject", true, true, L"" },
            { L"method", L"m", L"Injection method", false, true, L"crt" },
            { L"delay", L"", L"Delay before injection (ms)", false, true, L"0" }
        },
        [this](const ParsedOptions& opts) { return HandleMonitor(opts); }
    };
}

ParsedOptions CommandLine::Parse(int argc, wchar_t* argv[]) {
    ParsedOptions options;
    
    if (argc < 2) {
        return options;
    }
    
    int i = 1;
    
     
    if (argv[1][0] != L'-') {
        options.command = argv[1];
        i = 2;
    }
    
     
    while (i < argc) {
        std::wstring arg = argv[i];
        
        if (arg.substr(0, 2) == L"--") {
             
            std::wstring name = arg.substr(2);
            size_t eqPos = name.find(L'=');
            
            if (eqPos != std::wstring::npos) {
                options.options[name.substr(0, eqPos)] = name.substr(eqPos + 1);
            } else if (i + 1 < argc && argv[i + 1][0] != L'-') {
                options.options[name] = argv[++i];
            } else {
                options.options[name] = L"true";
            }
        } else if (arg[0] == L'-') {
             
            for (size_t j = 1; j < arg.length(); j++) {
                std::wstring shortName(1, arg[j]);
                
                 
                std::wstring fullName = shortName;
                for (const auto& garg : m_globalArgs) {
                    if (garg.shortName == shortName) {
                        fullName = garg.name;
                        break;
                    }
                }
                
                if (!options.command.empty()) {
                    auto cmdIt = m_commands.find(options.command);
                    if (cmdIt != m_commands.end()) {
                        for (const auto& carg : cmdIt->second.arguments) {
                            if (carg.shortName == shortName) {
                                fullName = carg.name;
                                break;
                            }
                        }
                    }
                }
                
                 
                if (j == arg.length() - 1 && i + 1 < argc && argv[i + 1][0] != L'-') {
                    options.options[fullName] = argv[++i];
                } else {
                    options.options[fullName] = L"true";
                }
            }
        } else {
             
            options.positionalArgs.push_back(arg);
        }
        
        i++;
    }
    
    return options;
}

int CommandLine::Execute(const ParsedOptions& options) {
     
    if (options.HasOption(L"no-color")) {
        Console::SetColorsEnabled(false);
    }
    
    if (options.HasOption(L"help") || options.command.empty()) {
        if (!options.command.empty()) {
            PrintCommandHelp(options.command);
        } else {
            PrintHelp();
        }
        return 0;
    }
    
    if (options.HasOption(L"version")) {
        PrintVersion();
        return 0;
    }
    
     
    auto it = m_commands.find(options.command);
    if (it == m_commands.end()) {
        Console::Error(L"Unknown command: " + options.command);
        Console::PrintLine(L"Use --help to see available commands.");
        return 1;
    }
    
    return it->second.handler(options);
}

int CommandLine::Run(int argc, wchar_t* argv[]) {
    ParsedOptions options = Parse(argc, argv);
    return Execute(options);
}

void CommandLine::PrintHelp() {
    Console::PrintLine(L"xorDLL - Advanced DLL Injector", Console::Color::Cyan);
    Console::PrintLine(L"Usage: xorDLL <command> [options]");
    Console::PrintLine();
    
    Console::PrintLine(L"Commands:", Console::Color::Yellow);
    for (const auto& pair : m_commands) {
        Console::Print(L"  " + pair.first, Console::Color::Green);
        Console::PrintLine(L"\t" + pair.second.description);
    }
    
    Console::PrintLine();
    Console::PrintLine(L"Global Options:", Console::Color::Yellow);
    for (const auto& arg : m_globalArgs) {
        std::wstring optStr = L"  --" + arg.name;
        if (!arg.shortName.empty()) {
            optStr += L", -" + arg.shortName;
        }
        Console::Print(optStr, Console::Color::Green);
        Console::PrintLine(L"\t" + arg.description);
    }
    
    Console::PrintLine();
    Console::PrintLine(L"Use 'xorDLL <command> --help' for more information about a command.");
}

void CommandLine::PrintCommandHelp(const std::wstring& command) {
    auto it = m_commands.find(command);
    if (it == m_commands.end()) {
        Console::Error(L"Unknown command: " + command);
        return;
    }
    
    const Command& cmd = it->second;
    
    Console::PrintLine(L"Usage: xorDLL " + cmd.name + L" [options]", Console::Color::Cyan);
    Console::PrintLine(cmd.description);
    Console::PrintLine();
    
    Console::PrintLine(L"Options:", Console::Color::Yellow);
    for (const auto& arg : cmd.arguments) {
        std::wstring optStr = L"  --" + arg.name;
        if (!arg.shortName.empty()) {
            optStr += L", -" + arg.shortName;
        }
        if (arg.hasValue) {
            optStr += L" <value>";
        }
        if (arg.required) {
            optStr += L" (required)";
        }
        Console::Print(optStr, Console::Color::Green);
        Console::PrintLine(L"\t" + arg.description);
    }
}

void CommandLine::PrintVersion() {
    Console::PrintLine(std::wstring(XORDLL_APP_NAME) + L" " + 
        utils::Utf8ToWide(XORDLL_VERSION_FULL), Console::Color::Cyan);
    Console::PrintLine(L"Build: " + utils::Utf8ToWide(XORDLL_GIT_HASH));
    Console::PrintLine(XORDLL_APP_COPYRIGHT);
}

 
 
 

int CommandLine::HandleInject(const ParsedOptions& options) {
    std::wstring dllPath = options.GetOption(L"dll");
    if (dllPath.empty()) {
        Console::Error(L"DLL path is required (--dll)");
        return 1;
    }
    
    ProcessId pid = 0;
    
    if (options.HasOption(L"pid")) {
        pid = options.GetIntOption(L"pid");
    } else if (options.HasOption(L"name")) {
        std::wstring processName = options.GetOption(L"name");
        
        ProcessManager pm;
        pm.RefreshProcessList();
        
        auto processes = pm.FilterByName(processName);
        if (processes.empty()) {
            if (options.HasOption(L"wait")) {
                Console::Info(L"Waiting for process: " + processName);
                 
            } else {
                Console::Error(L"Process not found: " + processName);
                return 1;
            }
        } else {
            pid = processes[0].pid;
            Console::Info(L"Found process: " + processes[0].name + L" (PID: " + std::to_wstring(pid) + L")");
        }
    } else {
        Console::Error(L"Either --pid or --name is required");
        return 1;
    }
    
     
    InjectionMethod method = InjectionMethod::CreateRemoteThread;
    std::wstring methodStr = options.GetOption(L"method", L"crt");
    
    if (methodStr == L"crt") method = InjectionMethod::CreateRemoteThread;
    else if (methodStr == L"ntcrt") method = InjectionMethod::NtCreateThreadEx;
    else if (methodStr == L"apc") method = InjectionMethod::QueueUserAPC;
    else if (methodStr == L"manual") method = InjectionMethod::ManualMap;
    else if (methodStr == L"hijack") method = InjectionMethod::ThreadHijack;
    
     
    int delay = options.GetIntOption(L"delay", 0);
    if (delay > 0) {
        Console::Info(L"Waiting " + std::to_wstring(delay) + L"ms...");
        Sleep(delay);
    }
    
     
    Console::Info(L"Injecting DLL...");
    
    InjectionCore injector;
    injector.SetLogCallback([](LogLevel level, const std::wstring& msg) {
        if (level == LogLevel::Error) {
            Console::Error(msg);
        } else if (level == LogLevel::Warning) {
            Console::Warning(msg);
        } else {
            Console::Info(msg);
        }
    });
    
    InjectionResult result = injector.Inject(pid, dllPath, method);
    
    if (result.success) {
        Console::Success(L"Injection successful!");
        Console::Info(L"Module handle: 0x" + utils::Utf8ToWide(
            std::to_string(reinterpret_cast<uintptr_t>(result.moduleHandle))));
        return 0;
    } else {
        Console::Error(L"Injection failed: " + result.errorMessage);
        return 1;
    }
}

int CommandLine::HandleEject(const ParsedOptions& options) {
    ProcessId pid = options.GetIntOption(L"pid");
    std::wstring dllName = options.GetOption(L"dll");
    
    if (pid == 0 || dllName.empty()) {
        Console::Error(L"Both --pid and --dll are required");
        return 1;
    }
    
    Console::Info(L"Ejecting DLL from PID " + std::to_wstring(pid) + L"...");
    
    InjectionCore injector;
    InjectionResult result = injector.Eject(pid, nullptr);   
    
    if (result.success) {
        Console::Success(L"Ejection successful!");
        return 0;
    } else {
        Console::Error(L"Ejection failed: " + result.errorMessage);
        return 1;
    }
}

int CommandLine::HandleList(const ParsedOptions& options) {
    ProcessManager pm;
    pm.RefreshProcessList();
    
    std::wstring filter = options.GetOption(L"filter");
    auto processes = filter.empty() ? pm.GetProcessList() : pm.FilterByName(filter);
    
     
    if (options.HasOption(L"x64")) {
        processes.erase(
            std::remove_if(processes.begin(), processes.end(),
                [](const ProcessInfo& p) { return !p.is64Bit; }),
            processes.end()
        );
    } else if (options.HasOption(L"x86")) {
        processes.erase(
            std::remove_if(processes.begin(), processes.end(),
                [](const ProcessInfo& p) { return p.is64Bit; }),
            processes.end()
        );
    }
    
     
    std::vector<std::wstring> headers = { L"PID", L"Name", L"Arch", L"Path" };
    std::vector<std::vector<std::wstring>> rows;
    
    for (const auto& proc : processes) {
        std::vector<std::wstring> row;
        row.push_back(std::to_wstring(proc.pid));
        row.push_back(proc.name);
        row.push_back(proc.is64Bit ? L"x64" : L"x86");
        row.push_back(proc.path);
        rows.push_back(row);
    }
    
    Console::PrintTable(headers, rows);
    Console::PrintLine();
    Console::Info(L"Total: " + std::to_wstring(processes.size()) + L" processes");
    
    return 0;
}

int CommandLine::HandleInfo(const ParsedOptions& options) {
    if (options.HasOption(L"dll")) {
        std::wstring dllPath = options.GetOption(L"dll");
        
        DllLoader& loader = DllLoader::Instance();
        DllInfo info;
        
        if (!loader.LoadDll(dllPath, info)) {
            Console::Error(L"Failed to load DLL info");
            return 1;
        }
        
        Console::PrintLine(L"DLL Information:", Console::Color::Cyan);
        Console::PrintLine(L"  Path: " + info.path);
        Console::PrintLine(L"  Architecture: " + std::wstring(info.is64Bit ? L"x64" : L"x86"));
        Console::PrintLine(L"  Signed: " + std::wstring(info.isSigned ? L"Yes" : L"No"));
        Console::PrintLine(L"  Description: " + info.description);
        Console::PrintLine(L"  Version: " + info.version);
        
         
        
        return 0;
    }
    
    if (options.HasOption(L"pid")) {
        ProcessId pid = options.GetIntOption(L"pid");
        
        ProcessManager pm;
        pm.RefreshProcessList();
        
        auto proc = pm.FindByPid(pid);
        if (!proc) {
            Console::Error(L"Process not found");
            return 1;
        }
        
        Console::PrintLine(L"Process Information:", Console::Color::Cyan);
        Console::PrintLine(L"  PID: " + std::to_wstring(proc->pid));
        Console::PrintLine(L"  Name: " + proc->name);
        Console::PrintLine(L"  Path: " + proc->path);
        Console::PrintLine(L"  Architecture: " + std::wstring(proc->is64Bit ? L"x64" : L"x86"));
        
        return 0;
    }
    
    Console::Error(L"Either --pid or --dll is required");
    return 1;
}

int CommandLine::HandleProfile(const ParsedOptions& options) {
    ProfileManager& pm = ProfileManager::Instance();
    pm.Load();
    
    if (options.HasOption(L"list")) {
        auto profiles = pm.GetAllProfiles();
        
        if (profiles.empty()) {
            Console::Info(L"No profiles found");
            return 0;
        }
        
        Console::PrintLine(L"Injection Profiles:", Console::Color::Cyan);
        for (const auto& pair : profiles) {
            Console::Print(L"  " + pair.second.name, Console::Color::Green);
            Console::PrintLine(L" - " + pair.second.description);
            Console::PrintLine(L"    Target: " + pair.second.targetProcess);
            Console::PrintLine(L"    DLL: " + pair.second.dllPath);
        }
        
        return 0;
    }
    
    if (options.HasOption(L"run")) {
        std::wstring profileName = options.GetOption(L"run");
        auto profile = pm.GetProfileByName(profileName);
        
        if (!profile) {
            Console::Error(L"Profile not found: " + profileName);
            return 1;
        }
        
        Console::Info(L"Running profile: " + profile->name);
         
        
        return 0;
    }
    
    PrintCommandHelp(L"profile");
    return 1;
}

int CommandLine::HandleMonitor(const ParsedOptions& options) {
    std::wstring processName = options.GetOption(L"process");
    std::wstring dllPath = options.GetOption(L"dll");
    
    if (processName.empty() || dllPath.empty()) {
        Console::Error(L"Both --process and --dll are required");
        return 1;
    }
    
    Console::Info(L"Monitoring for process: " + processName);
    Console::Info(L"Press Ctrl+C to stop...");
    
    AutoInjector injector;
    injector.SetLogCallback([](LogLevel level, const std::wstring& msg) {
        if (level == LogLevel::Error) {
            Console::Error(msg);
        } else {
            Console::Info(msg);
        }
    });
    
    InjectionMethod method = InjectionMethod::CreateRemoteThread;
    std::wstring methodStr = options.GetOption(L"method", L"crt");
    if (methodStr == L"ntcrt") method = InjectionMethod::NtCreateThreadEx;
    else if (methodStr == L"apc") method = InjectionMethod::QueueUserAPC;
    
    int delay = options.GetIntOption(L"delay", 0);
    
    injector.AddRule(processName, dllPath, method, delay);
    injector.Start();
    
     
    while (true) {
        Sleep(1000);
    }
    
    return 0;
}

 
 
 

void Console::Print(const std::wstring& text, Color color) {
    if (s_colorsEnabled) {
        SetColor(color);
    }
    std::wcout << text;
    if (s_colorsEnabled) {
        ResetColor();
    }
}

void Console::PrintLine(const std::wstring& text, Color color) {
    Print(text, color);
    std::wcout << std::endl;
}

void Console::Error(const std::wstring& text) {
    Print(L"[ERROR] ", Color::Red);
    PrintLine(text);
}

void Console::Success(const std::wstring& text) {
    Print(L"[OK] ", Color::Green);
    PrintLine(text);
}

void Console::Warning(const std::wstring& text) {
    Print(L"[WARN] ", Color::Yellow);
    PrintLine(text);
}

void Console::Info(const std::wstring& text) {
    Print(L"[INFO] ", Color::Cyan);
    PrintLine(text);
}

void Console::PrintTable(
    const std::vector<std::wstring>& headers,
    const std::vector<std::vector<std::wstring>>& rows
) {
     
    std::vector<size_t> widths(headers.size(), 0);
    
    for (size_t i = 0; i < headers.size(); i++) {
        widths[i] = headers[i].length();
    }
    
    for (const auto& row : rows) {
        for (size_t i = 0; i < row.size() && i < widths.size(); i++) {
            widths[i] = std::max(widths[i], row[i].length());
        }
    }
    
     
    for (size_t i = 0; i < headers.size(); i++) {
        Print(headers[i], Color::Yellow);
        std::wcout << std::wstring(widths[i] - headers[i].length() + 2, L' ');
    }
    std::wcout << std::endl;
    
     
    for (size_t i = 0; i < headers.size(); i++) {
        std::wcout << std::wstring(widths[i], L'-') << L"  ";
    }
    std::wcout << std::endl;
    
     
    for (const auto& row : rows) {
        for (size_t i = 0; i < row.size() && i < widths.size(); i++) {
            std::wcout << row[i] << std::wstring(widths[i] - row[i].length() + 2, L' ');
        }
        std::wcout << std::endl;
    }
}

void Console::PrintProgress(int current, int total, const std::wstring& label) {
    int percent = (current * 100) / total;
    int barWidth = 40;
    int filled = (percent * barWidth) / 100;
    
    std::wcout << L"\r" << label << L" [";
    for (int i = 0; i < barWidth; i++) {
        std::wcout << (i < filled ? L"=" : L" ");
    }
    std::wcout << L"] " << percent << L"%" << std::flush;
}

void Console::ClearLine() {
    std::wcout << L"\r" << std::wstring(80, L' ') << L"\r" << std::flush;
}

void Console::SetColorsEnabled(bool enabled) {
    s_colorsEnabled = enabled;
}

void Console::SetColor(Color color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    WORD attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    
    switch (color) {
        case Color::Red:
            attr = FOREGROUND_RED | FOREGROUND_INTENSITY;
            break;
        case Color::Green:
            attr = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            break;
        case Color::Yellow:
            attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            break;
        case Color::Blue:
            attr = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
            break;
        case Color::Cyan:
            attr = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
            break;
        case Color::White:
            attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
            break;
        default:
            break;
    }
    
    SetConsoleTextAttribute(hConsole, attr);
}

void Console::ResetColor() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

}  
}  
