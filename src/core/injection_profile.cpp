 

#include "core/injection_profile.h"
#include "utils/file_utils.h"
#include "utils/string_utils.h"
#include <fstream>
#include <sstream>
#include <random>
#include <iomanip>
#include <algorithm>

namespace xordll {

 
 
 

ProfileManager& ProfileManager::Instance() {
    static ProfileManager instance;
    return instance;
}

bool ProfileManager::Load(const std::wstring& path) {
    std::wstring filePath = path.empty() ? GetDefaultPath() : path;
    m_currentPath = filePath;
    
    if (!utils::FileExists(filePath)) {
        return true;   
    }
    
    std::ifstream file(filePath.c_str());
    if (!file.is_open()) {
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();
    
    return ProfileSerializer::FromJsonArray(json, m_profiles);
}

bool ProfileManager::Save(const std::wstring& path) {
    std::wstring filePath = path.empty() ? m_currentPath : path;
    
    if (filePath.empty()) {
        filePath = GetDefaultPath();
    }
    
     
    std::wstring dir = utils::GetDirectory(filePath);
    if (!dir.empty()) {
        utils::CreateDirectoryRecursive(dir);
    }
    
    std::string json = ProfileSerializer::ToJsonArray(m_profiles);
    
    std::ofstream file(filePath.c_str());
    if (!file.is_open()) {
        return false;
    }
    
    file << json;
    return true;
}

std::wstring ProfileManager::AddProfile(const InjectionProfile& profile) {
    std::wstring id = GenerateId();
    m_profiles[id] = profile;
    return id;
}

bool ProfileManager::RemoveProfile(const std::wstring& id) {
    auto it = m_profiles.find(id);
    if (it != m_profiles.end()) {
        m_profiles.erase(it);
        return true;
    }
    return false;
}

InjectionProfile* ProfileManager::GetProfile(const std::wstring& id) {
    auto it = m_profiles.find(id);
    if (it != m_profiles.end()) {
        return &it->second;
    }
    return nullptr;
}

InjectionProfile* ProfileManager::GetProfileByName(const std::wstring& name) {
    for (auto& pair : m_profiles) {
        if (pair.second.name == name) {
            return &pair.second;
        }
    }
    return nullptr;
}

std::vector<InjectionProfile*> ProfileManager::GetProfilesForProcess(const std::wstring& processName) {
    std::vector<InjectionProfile*> result;
    std::wstring lowerName = utils::ToLower(processName);
    
    for (auto& pair : m_profiles) {
        std::wstring targetLower = utils::ToLower(pair.second.targetProcess);
        
         
        if (targetLower == lowerName || 
            utils::ContainsIgnoreCase(lowerName, targetLower) ||
            utils::ContainsIgnoreCase(targetLower, L"*")) {
            result.push_back(&pair.second);
        }
    }
    
    return result;
}

std::vector<InjectionProfile*> ProfileManager::GetAutoInjectProfiles() {
    std::vector<InjectionProfile*> result;
    
    for (auto& pair : m_profiles) {
        if (pair.second.autoInject) {
            result.push_back(&pair.second);
        }
    }
    
    return result;
}

bool ProfileManager::UpdateProfile(const std::wstring& id, const InjectionProfile& profile) {
    auto it = m_profiles.find(id);
    if (it != m_profiles.end()) {
        it->second = profile;
        return true;
    }
    return false;
}

bool ProfileManager::ExportProfile(const std::wstring& id, const std::wstring& path) {
    auto profile = GetProfile(id);
    if (!profile) {
        return false;
    }
    
    std::string json = ProfileSerializer::ToJson(*profile);
    
    std::ofstream file(path.c_str());
    if (!file.is_open()) {
        return false;
    }
    
    file << json;
    return true;
}

std::wstring ProfileManager::ImportProfile(const std::wstring& path) {
    std::ifstream file(path.c_str());
    if (!file.is_open()) {
        return L"";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();
    
    InjectionProfile profile;
    if (!ProfileSerializer::FromJson(json, profile)) {
        return L"";
    }
    
    return AddProfile(profile);
}

std::wstring ProfileManager::GenerateId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::wstringstream ss;
    for (int i = 0; i < 8; i++) {
        ss << std::hex << dis(gen);
    }
    
    return ss.str();
}

std::wstring ProfileManager::GetDefaultPath() {
    return utils::GetAppDataPath() + L"\\xorDLL\\profiles.json";
}

 
 
 

 
std::string ProfileSerializer::ToJson(const InjectionProfile& profile) {
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"name\": \"" << utils::WideToUtf8(profile.name) << "\",\n";
    ss << "  \"description\": \"" << utils::WideToUtf8(profile.description) << "\",\n";
    ss << "  \"targetProcess\": \"" << utils::WideToUtf8(profile.targetProcess) << "\",\n";
    ss << "  \"dllPath\": \"" << utils::WideToUtf8(profile.dllPath) << "\",\n";
    ss << "  \"method\": " << static_cast<int>(profile.method) << ",\n";
    ss << "  \"waitForProcess\": " << (profile.waitForProcess ? "true" : "false") << ",\n";
    ss << "  \"waitTimeout\": " << profile.waitTimeout << ",\n";
    ss << "  \"injectionDelay\": " << profile.injectionDelay << ",\n";
    ss << "  \"antiDetect\": " << static_cast<int>(profile.antiDetect) << ",\n";
    ss << "  \"autoInject\": " << (profile.autoInject ? "true" : "false") << ",\n";
    ss << "  \"injectOnStartup\": " << (profile.injectOnStartup ? "true" : "false") << ",\n";
    ss << "  \"keepTrying\": " << (profile.keepTrying ? "true" : "false") << ",\n";
    ss << "  \"maxRetries\": " << profile.maxRetries << ",\n";
    ss << "  \"retryDelay\": " << profile.retryDelay << ",\n";
    ss << "  \"requireAdmin\": " << (profile.requireAdmin ? "true" : "false") << ",\n";
    ss << "  \"x64Only\": " << (profile.x64Only ? "true" : "false") << ",\n";
    ss << "  \"x86Only\": " << (profile.x86Only ? "true" : "false") << "\n";
    ss << "}";
    return ss.str();
}

 
static std::string GetJsonValue(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\":";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return "";
    
    pos += searchKey.length();
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    
    if (pos >= json.length()) return "";
    
    if (json[pos] == '"') {
         
        pos++;
        size_t end = json.find('"', pos);
        if (end == std::string::npos) return "";
        return json.substr(pos, end - pos);
    } else {
         
        size_t end = pos;
        while (end < json.length() && json[end] != ',' && json[end] != '}' && json[end] != '\n') {
            end++;
        }
        std::string value = json.substr(pos, end - pos);
         
        while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || value.back() == '\r')) {
            value.pop_back();
        }
        return value;
    }
}

bool ProfileSerializer::FromJson(const std::string& json, InjectionProfile& profile) {
    try {
        profile.name = utils::Utf8ToWide(GetJsonValue(json, "name"));
        profile.description = utils::Utf8ToWide(GetJsonValue(json, "description"));
        profile.targetProcess = utils::Utf8ToWide(GetJsonValue(json, "targetProcess"));
        profile.dllPath = utils::Utf8ToWide(GetJsonValue(json, "dllPath"));
        
        std::string methodStr = GetJsonValue(json, "method");
        if (!methodStr.empty()) {
            profile.method = static_cast<InjectionMethod>(std::stoi(methodStr));
        }
        
        profile.waitForProcess = GetJsonValue(json, "waitForProcess") == "true";
        
        std::string timeoutStr = GetJsonValue(json, "waitTimeout");
        if (!timeoutStr.empty()) {
            profile.waitTimeout = std::stoul(timeoutStr);
        }
        
        std::string delayStr = GetJsonValue(json, "injectionDelay");
        if (!delayStr.empty()) {
            profile.injectionDelay = std::stoul(delayStr);
        }
        
        std::string antiDetectStr = GetJsonValue(json, "antiDetect");
        if (!antiDetectStr.empty()) {
            profile.antiDetect = static_cast<AntiDetectTechnique>(std::stoi(antiDetectStr));
        }
        
        profile.autoInject = GetJsonValue(json, "autoInject") == "true";
        profile.injectOnStartup = GetJsonValue(json, "injectOnStartup") == "true";
        profile.keepTrying = GetJsonValue(json, "keepTrying") == "true";
        
        std::string retriesStr = GetJsonValue(json, "maxRetries");
        if (!retriesStr.empty()) {
            profile.maxRetries = std::stoi(retriesStr);
        }
        
        std::string retryDelayStr = GetJsonValue(json, "retryDelay");
        if (!retryDelayStr.empty()) {
            profile.retryDelay = std::stoul(retryDelayStr);
        }
        
        profile.requireAdmin = GetJsonValue(json, "requireAdmin") == "true";
        profile.x64Only = GetJsonValue(json, "x64Only") == "true";
        profile.x86Only = GetJsonValue(json, "x86Only") == "true";
        
        return true;
    } catch (...) {
        return false;
    }
}

std::string ProfileSerializer::ToJsonArray(const std::map<std::wstring, InjectionProfile>& profiles) {
    std::stringstream ss;
    ss << "{\n";
    
    bool first = true;
    for (const auto& pair : profiles) {
        if (!first) ss << ",\n";
        first = false;
        
        ss << "  \"" << utils::WideToUtf8(pair.first) << "\": " << ToJson(pair.second);
    }
    
    ss << "\n}";
    return ss.str();
}

bool ProfileSerializer::FromJsonArray(const std::string& json, std::map<std::wstring, InjectionProfile>& profiles) {
    profiles.clear();
    
     
    size_t pos = 0;
    while ((pos = json.find("\"", pos)) != std::string::npos) {
        pos++;
        size_t keyEnd = json.find("\"", pos);
        if (keyEnd == std::string::npos) break;
        
        std::string key = json.substr(pos, keyEnd - pos);
        pos = keyEnd + 1;
        
         
        size_t objStart = json.find("{", pos);
        if (objStart == std::string::npos) break;
        
         
        int braceCount = 1;
        size_t objEnd = objStart + 1;
        while (objEnd < json.length() && braceCount > 0) {
            if (json[objEnd] == '{') braceCount++;
            else if (json[objEnd] == '}') braceCount--;
            objEnd++;
        }
        
        std::string profileJson = json.substr(objStart, objEnd - objStart);
        
        InjectionProfile profile;
        if (FromJson(profileJson, profile)) {
            profiles[utils::Utf8ToWide(key)] = profile;
        }
        
        pos = objEnd;
    }
    
    return true;
}

}  
