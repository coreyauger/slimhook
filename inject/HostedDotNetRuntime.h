#pragma once 

#include "stdafx.h"

#import "mscorlib.tlb" raw_interfaces_only				\
    high_property_prefixes("_get","_put","_putref")		\
    rename("ReportEvent", "InteropServices_ReportEvent")
using namespace mscorlib;

//void HackLog(const char* msg );

//HRESULT RuntimeHostV4(PCWSTR pszVersion, PCWSTR pszAssemblyBase, PCWSTR pszAssemblyName, PCWSTR pszClassName, PCWSTR channel);

// forward declare pointers
class ICLRMetaHost;
class ICLRRuntimeInfo;
class ICorRuntimeHost;

namespace slimhook{

	class HostedDotNetRuntime
	{
	private:
		IAppDomainSetupPtr		_spAppDomainSetup;
		_AssemblyPtr			_spAssembly;
		_AppDomainPtr			_spDefaultAppDomain;	
		ICLRMetaHost*			_pMetaHost;
		ICLRRuntimeInfo*		_pRuntimeInfo;
		ICorRuntimeHost*		_pCorRuntimeHost;
		_TypePtr 				_spType;


	public:
		HostedDotNetRuntime()
			: _spAppDomainSetup(NULL)
			, _spAssembly(NULL)
			, _spDefaultAppDomain(NULL)
			, _pMetaHost(NULL)
			, _pRuntimeInfo(NULL)
			, _pCorRuntimeHost(NULL)
			, _spType(NULL)
		{}
		~HostedDotNetRuntime();

		HRESULT RuntimeHostV4(PCWSTR pszVersion, PCWSTR pszAssemblyBase, PCWSTR pszAssemblyName, PCWSTR pszClassName, PCWSTR channel);
		void CleanUp();

	};
	
}