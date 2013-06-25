
#pragma region Includes and Imports
#include "stdafx.h"
#include <windows.h>
#include <metahost.h>
#include "HostedDotNetRuntime.h"
#include <iostream>
#include <fstream>
#pragma comment(lib, "mscoree.lib")

// Import mscorlib.tlb (Microsoft Common Language Runtime Class Library).
#import "mscorlib.tlb" raw_interfaces_only				\
    high_property_prefixes("_get","_put","_putref")		\
    rename("ReportEvent", "InteropServices_ReportEvent")
using namespace mscorlib;
#pragma endregion



//
//   FUNCTION: RuntimeHostV4Demo1(PCWSTR, PCWSTR)
//
//   PURPOSE: The function demonstrates using .NET Framework 4.0 Hosting 
//   Interfaces to host a .NET runtime, and use the ICorRuntimeHost interface
//   that was provided in .NET v1.x to load a .NET assembly and invoke its 
//   type. 
//   
//   If the .NET runtime specified by the pszVersion parameter cannot be 
//   loaded into the current process, the function prints ".NET runtime <the 
//   runtime version> cannot be loaded", and return.
//   
//   If the .NET runtime is successfully loaded, the function loads the 
//   assembly identified by the pszAssemblyName parameter. Next, the function 
//   instantiates the class (pszClassName) in the assembly, calls its 
//   ToString() member method, and print the result. Last, the demo invokes 
//   the public static function 'int GetStringLength(string str)' of the class 
//   and print the result too.
//
//   PARAMETERS:
//   * pszVersion - The desired DOTNETFX version, in the format ��vX.X.XXXXX��. 
//     The parameter must not be NULL. It��s important to note that this 
//     parameter should match exactly the directory names for each version of
//     the framework, under C:\Windows\Microsoft.NET\Framework[64]. The 
//     current possible values are "v1.0.3705", "v1.1.4322", "v2.0.50727" and 
//     "v4.0.30319". Also, note that the ��v�� prefix is mandatory.
//   * pszAssemblyName - The display name of the assembly to be loaded, such 
//     as "CSClassLibrary". The ".DLL" file extension is not appended.
//   * pszClassName - The name of the Type that defines the method to invoke.
//
//   RETURN VALUE: HRESULT of the demo.
//

HRESULT slimhook::HostedDotNetRuntime::RuntimeHostV4(PCWSTR pszVersion, PCWSTR pszAssemblyBase, PCWSTR pszAssemblyName, PCWSTR pszClassName, PCWSTR channel)
{
    HRESULT hr;
       
    // ICorRuntimeHost and ICLRRuntimeHost are the two CLR hosting interfaces
    // supported by CLR 4.0. Here we demo the ICorRuntimeHost interface that 
    // was provided in .NET v1.x, and is compatible with all .NET Frameworks. 
    
    IUnknownPtr spAppDomainThunk = NULL;    
	IUnknownPtr spAppDomainSetupThunk = NULL;
	IUnknownPtr spEvidenceThunk = NULL;
	
	// The .NET assembly base
	bstr_t bstrAssemblyBase(pszAssemblyBase);

    // The .NET assembly to load.
    bstr_t bstrAssemblyName(pszAssemblyName);
    
    // The .NET class to instantiate.
    bstr_t bstrClassName(pszClassName);
    
    variant_t vtEmpty;

    
    // The instance method in the .NET class to invoke.
    bstr_t bstrMethodName(L"AddHook");
    SAFEARRAY *psaMethodArgs = NULL;
    variant_t vtStringRet;
	variant_t vtStringArg(channel);
	_ObjectPtr pObject; 
	_ObjectHandlePtr pObjectHandle; 

    // 
    // Load and start the .NET runtime.
    // 
	char buf[1024];
	FILE_LOG(logDEBUG) << "Load and start the .NET runtime";

    wprintf(L"Load and start the .NET runtime %s \n", pszVersion);

    hr = CLRCreateInstance(CLSID_CLRMetaHost, IID_PPV_ARGS(&_pMetaHost));
    if (FAILED(hr)){
		FILE_LOG(logERROR) << "CLRCreateInstance failed w/hr: 0x" << std::hex << hr;
		CleanUp();
        goto Cleanup;
    }

    // Get the ICLRRuntimeInfo corresponding to a particular CLR version. It 
    // supersedes CorBindToRuntimeEx with STARTUP_LOADER_SAFEMODE.
    hr = _pMetaHost->GetRuntime(pszVersion, IID_PPV_ARGS(&_pRuntimeInfo));
    if (FAILED(hr)){
		FILE_LOG(logERROR) << "ICLRMetaHost::GetRuntime failed w/hr 0x" << std::hex <<  hr;
		CleanUp();
        goto Cleanup;
    }

    // Check if the specified runtime can be loaded into the process. This 
    // method will take into account other runtimes that may already be 
    // loaded into the process and set pbLoadable to TRUE if this runtime can 
    // be loaded in an in-process side-by-side fashion. 
    BOOL fLoadable;
    hr = _pRuntimeInfo->IsLoadable(&fLoadable);
    if (FAILED(hr)){
		FILE_LOG(logERROR) << "ICLRRuntimeInfo::IsLoadable failed w/hr 0x" << std::hex <<  hr;
		CleanUp();
        goto Cleanup;
    }

    if (!fLoadable){
		FILE_LOG(logERROR) << ".NET runtime "<< pszVersion <<" cannot be loaded";
		CleanUp();
        goto Cleanup;
    }

    // Load the CLR into the current process and return a runtime interface 
    // pointer. ICorRuntimeHost and ICLRRuntimeHost are the two CLR hosting  
    // interfaces supported by CLR 4.0. Here we demo the ICorRuntimeHost 
    // interface that was provided in .NET v1.x, and is compatible with all 
    // .NET Frameworks. 
    hr = _pRuntimeInfo->GetInterface(CLSID_CorRuntimeHost, IID_PPV_ARGS(&_pCorRuntimeHost));
    if (FAILED(hr)){
		FILE_LOG(logERROR) << "ICLRRuntimeInfo::GetInterface failed w/hr 0x" << std::hex <<  hr;
		CleanUp();
        goto Cleanup;
    }

    // Start the CLR.
    hr = _pCorRuntimeHost->Start();
    if (FAILED(hr)){
		FILE_LOG(logERROR) << "CLR failed to start w/hr 0x" << std::hex <<  hr;
		CleanUp();
        goto Cleanup;
    }

    // 
    // Load the NET assembly. Call the static method GetStringLength of the 
    // class CSSimpleObject. Instantiate the class CSSimpleObject and call 
    // its instance method ToString.
    // 

    // The following C++ code does the same thing as this C# code:
    // 
    //   Assembly assembly = AppDomain.CurrentDomain.Load(pszAssemblyName);
    //   object length = type.InvokeMember("GetStringLength", 
    //       BindingFlags.InvokeMethod | BindingFlags.Static | 
    //       BindingFlags.Public, null, null, new object[] { "HelloWorld" });
    //   object obj = assembly.CreateInstance("CSClassLibrary.CSSimpleObject");
    //   object str = type.InvokeMember("ToString", 
    //       BindingFlags.InvokeMethod | BindingFlags.Instance | 
    //       BindingFlags.Public, null, obj, new object[] { });

    // Get a pointer to the default AppDomain in the CLR.
    //hr = pCorRuntimeHost->GetDefaultDomain(&spAppDomainThunk);		

	hr = _pCorRuntimeHost->CreateDomainSetup( &spAppDomainSetupThunk );
	if (FAILED(hr)){
		FILE_LOG(logERROR) << "ICorRuntimeHost::CreateDomainSetup failed w/hr 0x" << std::hex <<  hr;
		CleanUp();
        goto Cleanup;
    }
	hr = spAppDomainSetupThunk->QueryInterface(IID_PPV_ARGS(&_spAppDomainSetup));
	if (FAILED(hr)){
		FILE_LOG(logERROR) << "Failed to get AppDomainSetup w/hr 0x" << std::hex <<  hr;
		CleanUp();
        goto Cleanup;
    }
	FILE_LOG(logINFO) << "Put Application Base: " << bstrAssemblyBase;
	_spAppDomainSetup->put_ApplicationBase(bstrAssemblyBase);

	hr = _pCorRuntimeHost->CreateDomainEx(L"SlimHookAppDomain", _spAppDomainSetup, NULL, &spAppDomainThunk);
	if (FAILED(hr)){
		// 0x8007000e == E_OUTOFMEMORY 
		// http://theether.net/download/Microsoft/kb/158508.html (E_OUTOFMEMORY is returned, instead of E_ACCESSDENIED)
		FILE_LOG(logERROR) << "ICorRuntimeHost::CreateDomainEx failed w/hr 0x" << std::hex <<  hr;
		CleanUp();
        goto Cleanup;
    }

    hr = spAppDomainThunk->QueryInterface(IID_PPV_ARGS(&_spDefaultAppDomain));
    if (FAILED(hr)){
		FILE_LOG(logERROR) << "Failed to get default AppDomain w/hr 0x" << std::hex <<  hr;
		CleanUp();
        goto Cleanup;
    }

    // Load the .NET assembly.
    FILE_LOG(logINFO) << "Load the assembly " << bstr_t( pszAssemblyName );
	
    hr = _spDefaultAppDomain->Load_2(bstrAssemblyName, &_spAssembly);
    if (FAILED(hr)){
		FILE_LOG(logERROR) << "Failed to load the assembly w/hr 0x" << std::hex <<  hr;
		CleanUp();
        goto Cleanup;
    }

    // Get the Type of CSSimpleObject.
	FILE_LOG(logINFO) << "Get Type " << bstrClassName;
    hr = _spAssembly->GetType_2(bstrClassName, &_spType);
    if (FAILED(hr)){
		FILE_LOG(logERROR) << "Failed to get the Type interface w/hr 0x" << std::hex <<  hr;
		CleanUp();
        goto Cleanup;
    }
	
	FILE_LOG(logINFO) << "CreateInstance " << bstrClassName;
    hr = _spAssembly->CreateInstance(bstrClassName, &_vtObject);
    if (FAILED(hr)){
		FILE_LOG(logERROR) << "Assembly::CreateInstance of className " << bstrClassName << " failed w/hr 0x" << std::hex <<  hr;
		CleanUp();
        goto Cleanup;
    }
	
    // Call the instance method of the class.
    //   public string ToString();

    // Create a safe array to contain the arguments of the method.
    psaMethodArgs = SafeArrayCreateVector(VT_VARIANT, 0, 1);
	LONG index = 0;
	hr = SafeArrayPutElement(psaMethodArgs, &index, &vtStringArg);
	FILE_LOG(logINFO) << "SafeArrayPutElement ";
    if (FAILED(hr)){
		FILE_LOG(logERROR) << "SafeArrayPutElement failed w/hr 0x" << std::hex <<  hr;
		CleanUp();
        goto Cleanup;
    }

    // Invoke the  method from the Type interface.
	FILE_LOG(logINFO) << "InvokeMember_3 " << bstr_t(psaMethodArgs);
    hr = _spType->InvokeMember_3(bstrMethodName, static_cast<BindingFlags>(
        BindingFlags_InvokeMethod | BindingFlags_Instance | BindingFlags_Public),
        NULL, _vtObject, psaMethodArgs, &vtStringRet);
    if (FAILED(hr)){
		FILE_LOG(logERROR) << "Failed to invoke " << bstrMethodName <<" w/hr 0x" << std::hex <<  hr;
		CleanUp();
        goto Cleanup;
    }

	FILE_LOG(logINFO) << "Done " ;
    // Print the call result of the method.
    //wprintf(L"Call %s.%s() => %s\n", 
      //  static_cast<PCWSTR>(bstrClassName), 
        //static_cast<PCWSTR>(bstrMethodName), 
        //static_cast<PCWSTR>(vtStringRet.bstrVal));
Cleanup:
		
    if (psaMethodArgs)
    {
        SafeArrayDestroy(psaMethodArgs);
        psaMethodArgs = NULL;
    }
    return hr;
}

void slimhook::HostedDotNetRuntime::CleanUp()
{
	FILE_LOG(logINFO) << "HostedDotNetRuntime::CleanUp " ;
	if (_pMetaHost)
    {
        _pMetaHost->Release();
        _pMetaHost = NULL;
    }
    if (_pRuntimeInfo)
    {
        _pRuntimeInfo->Release();
        _pRuntimeInfo = NULL;
    }
    if (_pCorRuntimeHost)
    {
        // Please note that after a call to Stop, the CLR cannot be 
        // reinitialized into the same process. This step is usually not 
        // necessary. You can leave the .NET runtime loaded in your process.
        //wprintf(L"Stop the .NET runtime\n");
        //pCorRuntimeHost->Stop();

        _pCorRuntimeHost->Release();
        _pCorRuntimeHost = NULL;
    }
    
}

slimhook::HostedDotNetRuntime::~HostedDotNetRuntime(){
	FILE_LOG(logINFO) << "Destroy" ;
	CleanUp();
}


