#pragma once

#include "core/types.h"
#include "core/anti_detection.h"
#include <string>
#include <vector>
#include <map>

namespace xordll {

 
struct InjectionProfile {
    std::wstring name;
    std::wstring description;
    
     
    std::wstring targetProcess;          
    std::wstring dllPath;                
    
     
    InjectionMethod method;              
    bool waitForProcess;                 
    DWORD waitTimeout;                   
    DWORD injectionDelay;                
    
     
    AntiDetectTechnique antiDetect;      
    
     
    bool autoInject;                     
    bool injectOnStartup;                
    bool keepTrying;                     
    int maxRetries;                      
    DWORD retryDelay;                    
    
     
    bool requireAdmin;                   
    bool x64Only;                        
    bool x86Only;                        
    
    InjectionProfile()
        : method(InjectionMethod::CreateRemoteThread)
        , waitForProcess(false)
        , waitTimeout(30000)
        , injectionDelay(0)
        , antiDetect(AntiDetectTechnique::None)
        , autoInject(false)
        , injectOnStartup(false)
        , keepTrying(false)
        , maxRetries(3)
        , retryDelay(1000)
        , requireAdmin(false)
        , x64Only(false)
        , x86Only(false)
    {}
};

 
class ProfileManager {
public:
    static ProfileManager& Instance();
    
     
    bool Load(const std::wstring& path = L"");
    
     
    bool Save(const std::wstring& path = L"");
    
     
    std::wstring AddProfile(const InjectionProfile& profile);
    
     
    bool RemoveProfile(const std::wstring& id);
    
     
    InjectionProfile* GetProfile(const std::wstring& id);
    
     
    InjectionProfile* GetProfileByName(const std::wstring& name);
    
     
    const std::map<std::wstring, InjectionProfile>& GetAllProfiles() const { return m_profiles; }
    
     
    std::vector<InjectionProfile*> GetProfilesForProcess(const std::wstring& processName);
    
     
    std::vector<InjectionProfile*> GetAutoInjectProfiles();
    
     
    bool UpdateProfile(const std::wstring& id, const InjectionProfile& profile);
    
     
    bool ExportProfile(const std::wstring& id, const std::wstring& path);
    
     
    std::wstring ImportProfile(const std::wstring& path);

private:
    ProfileManager() = default;
    ~ProfileManager() = default;
    ProfileManager(const ProfileManager&) = delete;
    ProfileManager& operator=(const ProfileManager&) = delete;
    
    std::wstring GenerateId();
    std::wstring GetDefaultPath();
    
    std::map<std::wstring, InjectionProfile> m_profiles;
    std::wstring m_currentPath;
};

 
class ProfileSerializer {
public:
     
    static std::string ToJson(const InjectionProfile& profile);
    
     
    static bool FromJson(const std::string& json, InjectionProfile& profile);
    
     
    static std::string ToJsonArray(const std::map<std::wstring, InjectionProfile>& profiles);
    
     
    static bool FromJsonArray(const std::string& json, std::map<std::wstring, InjectionProfile>& profiles);
};

}  
