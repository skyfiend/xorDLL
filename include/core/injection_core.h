#pragma once

#include "core/types.h"
#include <memory>

namespace xordll {

 
class IInjectionMethod {
public:
    virtual ~IInjectionMethod() = default;
    
     
    virtual std::wstring GetName() const = 0;
    
     
    virtual std::wstring GetDescription() const = 0;
    
     
    virtual bool RequiresAdmin() const = 0;
    
     
    virtual bool Supports32Bit() const = 0;
    
     
    virtual bool Supports64Bit() const = 0;
    
     
    virtual InjectionResult Inject(
        HANDLE processHandle,
        const std::wstring& dllPath,
        ProgressCallback progressCallback = nullptr
    ) = 0;
    
     
    virtual InjectionResult Eject(
        HANDLE processHandle,
        ModuleHandle moduleHandle
    ) = 0;
};

 
class InjectionCore {
public:
    InjectionCore();
    ~InjectionCore();
    
     
    InjectionCore(const InjectionCore&) = delete;
    InjectionCore& operator=(const InjectionCore&) = delete;
    
     
    InjectionResult Inject(
        ProcessId pid,
        const std::wstring& dllPath,
        InjectionMethod method = InjectionMethod::CreateRemoteThread,
        ProgressCallback progressCallback = nullptr
    );
    
     
    InjectionResult Eject(
        ProcessId pid,
        ModuleHandle moduleHandle,
        InjectionMethod method = InjectionMethod::CreateRemoteThread
    );
    
     
    std::vector<InjectionMethod> GetAvailableMethods() const;
    
     
    static std::wstring GetMethodName(InjectionMethod method);
    
     
    static std::wstring GetMethodDescription(InjectionMethod method);
    
     
    static bool MethodRequiresAdmin(InjectionMethod method);
    
     
    static bool ValidateDll(const std::wstring& dllPath, DllInfo& info);
    
     
    void SetLogCallback(LogCallback callback);

private:
    std::unique_ptr<IInjectionMethod> CreateMethod(InjectionMethod method);
    LogCallback m_logCallback;
    
    void Log(LogLevel level, const std::wstring& message);
};

 

 
class CreateRemoteThreadInjection : public IInjectionMethod {
public:
    std::wstring GetName() const override;
    std::wstring GetDescription() const override;
    bool RequiresAdmin() const override;
    bool Supports32Bit() const override;
    bool Supports64Bit() const override;
    
    InjectionResult Inject(
        HANDLE processHandle,
        const std::wstring& dllPath,
        ProgressCallback progressCallback = nullptr
    ) override;
    
    InjectionResult Eject(
        HANDLE processHandle,
        ModuleHandle moduleHandle
    ) override;
};

 
class NtCreateThreadExInjection : public IInjectionMethod {
public:
    std::wstring GetName() const override;
    std::wstring GetDescription() const override;
    bool RequiresAdmin() const override;
    bool Supports32Bit() const override;
    bool Supports64Bit() const override;
    
    InjectionResult Inject(
        HANDLE processHandle,
        const std::wstring& dllPath,
        ProgressCallback progressCallback = nullptr
    ) override;
    
    InjectionResult Eject(
        HANDLE processHandle,
        ModuleHandle moduleHandle
    ) override;
};

 
class QueueUserAPCInjection : public IInjectionMethod {
public:
    std::wstring GetName() const override;
    std::wstring GetDescription() const override;
    bool RequiresAdmin() const override;
    bool Supports32Bit() const override;
    bool Supports64Bit() const override;
    
    InjectionResult Inject(
        HANDLE processHandle,
        const std::wstring& dllPath,
        ProgressCallback progressCallback = nullptr
    ) override;
    
    InjectionResult Eject(
        HANDLE processHandle,
        ModuleHandle moduleHandle
    ) override;
};

 
class ManualMapInjection : public IInjectionMethod {
public:
    std::wstring GetName() const override;
    std::wstring GetDescription() const override;
    bool RequiresAdmin() const override;
    bool Supports32Bit() const override;
    bool Supports64Bit() const override;
    
    InjectionResult Inject(
        HANDLE processHandle,
        const std::wstring& dllPath,
        ProgressCallback progressCallback = nullptr
    ) override;
    
    InjectionResult Eject(
        HANDLE processHandle,
        ModuleHandle moduleHandle
    ) override;
};

 
class ThreadHijackInjection : public IInjectionMethod {
public:
    std::wstring GetName() const override;
    std::wstring GetDescription() const override;
    bool RequiresAdmin() const override;
    bool Supports32Bit() const override;
    bool Supports64Bit() const override;
    
    InjectionResult Inject(
        HANDLE processHandle,
        const std::wstring& dllPath,
        ProgressCallback progressCallback = nullptr
    ) override;
    
    InjectionResult Eject(
        HANDLE processHandle,
        ModuleHandle moduleHandle
    ) override;
    
private:
    ProcessId m_processId = 0;   
};

}  
