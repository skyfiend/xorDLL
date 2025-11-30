#pragma once

#include "core/types.h"
#include <string>
#include <vector>
#include <map>
#include <functional>

namespace xordll {
namespace cli {

 
struct Argument {
    std::wstring name;
    std::wstring shortName;
    std::wstring description;
    bool required;
    bool hasValue;
    std::wstring defaultValue;
};

 
struct ParsedOptions {
    std::wstring command;
    std::map<std::wstring, std::wstring> options;
    std::vector<std::wstring> positionalArgs;
    
    bool HasOption(const std::wstring& name) const {
        return options.find(name) != options.end();
    }
    
    std::wstring GetOption(const std::wstring& name, const std::wstring& defaultVal = L"") const {
        auto it = options.find(name);
        return it != options.end() ? it->second : defaultVal;
    }
    
    int GetIntOption(const std::wstring& name, int defaultVal = 0) const {
        auto it = options.find(name);
        if (it != options.end()) {
            try {
                return std::stoi(it->second);
            } catch (...) {}
        }
        return defaultVal;
    }
};

 
using CommandHandler = std::function<int(const ParsedOptions&)>;

 
struct Command {
    std::wstring name;
    std::wstring description;
    std::vector<Argument> arguments;
    CommandHandler handler;
};

 
class CommandLine {
public:
    CommandLine();
    ~CommandLine();
    
     
    ParsedOptions Parse(int argc, wchar_t* argv[]);
    
     
    int Execute(const ParsedOptions& options);
    
     
    int Run(int argc, wchar_t* argv[]);
    
     
    void PrintHelp();
    
     
    void PrintCommandHelp(const std::wstring& command);
    
     
    void PrintVersion();

private:
    void RegisterCommands();
    
     
    int HandleInject(const ParsedOptions& options);
    int HandleEject(const ParsedOptions& options);
    int HandleList(const ParsedOptions& options);
    int HandleInfo(const ParsedOptions& options);
    int HandleProfile(const ParsedOptions& options);
    int HandleMonitor(const ParsedOptions& options);
    
    std::map<std::wstring, Command> m_commands;
    std::vector<Argument> m_globalArgs;
};

 
class Console {
public:
    enum class Color {
        Default,
        Red,
        Green,
        Yellow,
        Blue,
        Cyan,
        White
    };
    
     
    static void Print(const std::wstring& text, Color color = Color::Default);
    
     
    static void PrintLine(const std::wstring& text = L"", Color color = Color::Default);
    
     
    static void Error(const std::wstring& text);
    
     
    static void Success(const std::wstring& text);
    
     
    static void Warning(const std::wstring& text);
    
     
    static void Info(const std::wstring& text);
    
     
    static void PrintTable(
        const std::vector<std::wstring>& headers,
        const std::vector<std::vector<std::wstring>>& rows
    );
    
     
    static void PrintProgress(int current, int total, const std::wstring& label = L"");
    
     
    static void ClearLine();
    
     
    static void SetColorsEnabled(bool enabled);

private:
    static bool s_colorsEnabled;
    static void SetColor(Color color);
    static void ResetColor();
};

}  
}  
