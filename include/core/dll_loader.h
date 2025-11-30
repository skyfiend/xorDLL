#pragma once

#include "core/types.h"
#include <map>
#include <mutex>

namespace xordll {

 
class DllLoader {
public:
     
    static DllLoader& Instance();
    
     
    DllLoader(const DllLoader&) = delete;
    DllLoader& operator=(const DllLoader&) = delete;
    
     
    bool LoadDll(const std::wstring& path, DllInfo& info);
    
     
    std::optional<DllInfo> GetCachedInfo(const std::wstring& path) const;
    
     
    void ClearCache();
    
     
    void RemoveFromCache(const std::wstring& path);
    
     
    static bool IsCompatible(const DllInfo& dllInfo, bool processIs64Bit);
    
     
    static bool ValidatePEHeaders(const std::wstring& path);
    
     
    static bool VerifyDigitalSignature(const std::wstring& path);
    
     
    static std::vector<std::string> GetExports(const std::wstring& path);
    
     
    static std::wstring GetDescription(const std::wstring& path);
    
     
    static std::wstring GetVersion(const std::wstring& path);
    
     
    static std::wstring GetCompanyName(const std::wstring& path);

private:
    DllLoader() = default;
    ~DllLoader() = default;
    
     
    static std::wstring GetVersionInfoString(const std::wstring& path, const std::wstring& key);
    
    std::map<std::wstring, DllInfo> m_cache;
    mutable std::mutex m_mutex;
};

}  
