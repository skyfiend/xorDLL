 

#include "ui/hotkeys.h"
#include "ui/main_window.h"
#include "utils/logger.h"

namespace xordll {
namespace ui {

 
 
 

std::wstring HotkeyDef::ToString() const {
    std::wstring result;
    
    if (modifiers & MOD_CONTROL) result += L"Ctrl+";
    if (modifiers & MOD_ALT) result += L"Alt+";
    if (modifiers & MOD_SHIFT) result += L"Shift+";
    
     
    switch (vk) {
        case VK_F1: result += L"F1"; break;
        case VK_F2: result += L"F2"; break;
        case VK_F3: result += L"F3"; break;
        case VK_F4: result += L"F4"; break;
        case VK_F5: result += L"F5"; break;
        case VK_F6: result += L"F6"; break;
        case VK_F7: result += L"F7"; break;
        case VK_F8: result += L"F8"; break;
        case VK_F9: result += L"F9"; break;
        case VK_F10: result += L"F10"; break;
        case VK_F11: result += L"F11"; break;
        case VK_F12: result += L"F12"; break;
        case VK_ESCAPE: result += L"Esc"; break;
        case VK_RETURN: result += L"Enter"; break;
        case VK_SPACE: result += L"Space"; break;
        case VK_DELETE: result += L"Del"; break;
        case VK_INSERT: result += L"Ins"; break;
        case VK_HOME: result += L"Home"; break;
        case VK_END: result += L"End"; break;
        default:
            if (vk >= 'A' && vk <= 'Z') {
                result += static_cast<wchar_t>(vk);
            } else if (vk >= '0' && vk <= '9') {
                result += static_cast<wchar_t>(vk);
            } else {
                result += L"?";
            }
            break;
    }
    
    return result;
}

 
 
 

HotkeyManager::HotkeyManager()
    : m_hwnd(nullptr)
    , m_initialized(false)
{
}

HotkeyManager::~HotkeyManager() {
    Shutdown();
}

bool HotkeyManager::Initialize(HWND hwnd) {
    if (m_initialized) return true;
    
    m_hwnd = hwnd;
    m_initialized = true;
    
    LOG_DEBUG(L"HotkeyManager initialized");
    return true;
}

void HotkeyManager::Shutdown() {
    if (!m_initialized) return;
    
     
    for (const auto& pair : m_hotkeys) {
        UnregisterHotKey(m_hwnd, static_cast<int>(pair.first));
    }
    
    m_hotkeys.clear();
    m_callbacks.clear();
    m_initialized = false;
    
    LOG_DEBUG(L"HotkeyManager shutdown");
}

bool HotkeyManager::RegisterHotkey(HotkeyId id, const HotkeyDef& def) {
    if (!m_initialized) return false;
    
     
    UnregisterHotkey(id);
    
    if (RegisterHotKey(m_hwnd, static_cast<int>(id), def.modifiers, def.vk)) {
        m_hotkeys[id] = def;
        LOG_DEBUG(L"Registered hotkey: " + def.name + L" (" + def.ToString() + L")");
        return true;
    }
    
    LOG_WARNING(L"Failed to register hotkey: " + def.name);
    return false;
}

void HotkeyManager::UnregisterHotkey(HotkeyId id) {
    if (!m_initialized) return;
    
    auto it = m_hotkeys.find(id);
    if (it != m_hotkeys.end()) {
        UnregisterHotKey(m_hwnd, static_cast<int>(id));
        m_hotkeys.erase(it);
    }
}

void HotkeyManager::SetCallback(HotkeyId id, std::function<void()> callback) {
    m_callbacks[id] = callback;
}

bool HotkeyManager::HandleHotkey(WPARAM wParam) {
    HotkeyId id = static_cast<HotkeyId>(wParam);
    
    auto it = m_callbacks.find(id);
    if (it != m_callbacks.end() && it->second) {
        it->second();
        return true;
    }
    
    return false;
}

HotkeyDef HotkeyManager::GetHotkey(HotkeyId id) const {
    auto it = m_hotkeys.find(id);
    if (it != m_hotkeys.end()) {
        return it->second;
    }
    return HotkeyDef();
}

void HotkeyManager::RegisterDefaults() {
     
    RegisterHotkey(HotkeyId::Inject, HotkeyDef(0, VK_F5, L"Inject"));
    
     
    RegisterHotkey(HotkeyId::Eject, HotkeyDef(0, VK_F6, L"Eject"));
    
     
    RegisterHotkey(HotkeyId::Refresh, HotkeyDef(0, VK_F9, L"Refresh"));
    
     
    RegisterHotkey(HotkeyId::OpenDll, HotkeyDef(MOD_CONTROL, 'O', L"Open DLL"));
    
     
    RegisterHotkey(HotkeyId::Settings, HotkeyDef(MOD_CONTROL, VK_OEM_COMMA, L"Settings"));
    
     
    RegisterHotkey(HotkeyId::FocusSearch, HotkeyDef(MOD_CONTROL, 'F', L"Focus Search"));
}

 
 
 

AcceleratorManager::AcceleratorManager()
    : m_hAccel(nullptr)
{
}

AcceleratorManager::~AcceleratorManager() {
    if (m_hAccel) {
        DestroyAcceleratorTable(m_hAccel);
    }
}

bool AcceleratorManager::Create() {
     
    ACCEL accels[] = {
        { FCONTROL | FVIRTKEY, 'O', ID_MENU_FILE_OPEN_DLL },
        { FVIRTKEY, VK_F5, ID_MENU_INJECT_START },
        { FVIRTKEY, VK_F6, ID_MENU_INJECT_EJECT },
        { FVIRTKEY, VK_F9, ID_BTN_REFRESH },
        { FCONTROL | FVIRTKEY, 'Q', ID_MENU_FILE_EXIT },
        { FVIRTKEY, VK_F1, ID_MENU_HELP_ABOUT },
    };
    
    m_hAccel = CreateAcceleratorTableW(accels, _countof(accels));
    return m_hAccel != nullptr;
}

bool AcceleratorManager::TranslateAccelerator(HWND hwnd, MSG* msg) {
    if (m_hAccel) {
        return ::TranslateAccelerator(hwnd, m_hAccel, msg) != 0;
    }
    return false;
}

}  
}  
