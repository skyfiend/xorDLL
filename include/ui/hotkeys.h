#pragma once

#include <windows.h>
#include <functional>
#include <map>
#include <string>

namespace xordll {
namespace ui {

 
enum class HotkeyId {
    Inject = 1,
    Eject,
    Refresh,
    OpenDll,
    Settings,
    About,
    ToggleTheme,
    FocusSearch,
    Exit
};

 
struct HotkeyDef {
    UINT modifiers;      
    UINT vk;             
    std::wstring name;   
    
    HotkeyDef() : modifiers(0), vk(0) {}
    HotkeyDef(UINT mod, UINT key, const std::wstring& n)
        : modifiers(mod), vk(key), name(n) {}
    
    std::wstring ToString() const;
};

 
class HotkeyManager {
public:
    HotkeyManager();
    ~HotkeyManager();
    
     
    bool Initialize(HWND hwnd);
    
     
    void Shutdown();
    
     
    bool RegisterHotkey(HotkeyId id, const HotkeyDef& def);
    
     
    void UnregisterHotkey(HotkeyId id);
    
     
    void SetCallback(HotkeyId id, std::function<void()> callback);
    
     
    bool HandleHotkey(WPARAM wParam);
    
     
    HotkeyDef GetHotkey(HotkeyId id) const;
    
     
    void RegisterDefaults();
    
     
    const std::map<HotkeyId, HotkeyDef>& GetAllHotkeys() const { return m_hotkeys; }

private:
    HWND m_hwnd;
    std::map<HotkeyId, HotkeyDef> m_hotkeys;
    std::map<HotkeyId, std::function<void()>> m_callbacks;
    bool m_initialized;
};

 
class AcceleratorManager {
public:
    AcceleratorManager();
    ~AcceleratorManager();
    
     
    bool Create();
    
     
    HACCEL GetHandle() const { return m_hAccel; }
    
     
    bool TranslateAccelerator(HWND hwnd, MSG* msg);

private:
    HACCEL m_hAccel;
};

}  
}  
