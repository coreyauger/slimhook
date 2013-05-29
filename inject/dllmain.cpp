#include "stdafx.h"
#include "NCodeHook/NCodeHookInstantiation.h"
#include "HostedDotNetRuntime.h"

// NOET: this needs to be in global scope - otherwise we tear down the runtime 
// which will result in a program crash..
slimhook::HostedDotNetRuntime _host;			// Our global .NET host object.

// NOTE: this needs to be in global scope - otherwise the trampolines and hooks
// are deleted when the destructor of nCodeHook is called!
NCodeHookIA32 nCodeHook;

#include <iostream>
#include <fstream>

// NOTE: every injected dll has to export at least one symbol - otherwise
// the OS loader will fail with STATUS_INVALID_IMAGE_FORMAT (0x0C000007B)

// we load d3d9 dll
typedef IDirect3D9* (WINAPI *Direct3DCreate9Fptr)(UINT SDKVersion);
Direct3DCreate9Fptr pDirect3DCreate9= NULL;

const int VT_PRESENT = 17;
typedef HRESULT (WINAPI *PresentFptr)(const RECT *pSourceRect,const RECT *pDestRect,HWND hDestWindowOverride,const RGNDATA *pDirtyRegion);
PresentFptr pPresentOriginal = NULL;
PresentFptr pPresentNew = NULL;
PresentFptr pPresentTramp = NULL;

const int VT_ENDSCENE = 42;
typedef HRESULT (WINAPI *EndSceneFptr)(LPDIRECT3DDEVICE9 pDevice);
EndSceneFptr pEndSceneOriginal = NULL;
EndSceneFptr pEndSceneNew = NULL;
EndSceneFptr pEndSceneTramp = NULL;
EndSceneFptr pDotNet = NULL;

const int ALREADY_HOOKED = 4;
const int DLL_LOAD_FAIL	 = 2;
const int RET_FAIL		 = 3;

 HRESULT WINAPI MyPresent(const RECT *pSourceRect,const RECT *pDestRect,HWND hDestWindowOverride,const RGNDATA *pDirtyRegion)
 {
	 //HackLog("PRESENT\n");
	 return pPresentTramp(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);	 
 }

 HRESULT WINAPI MyEndScene(LPDIRECT3DDEVICE9 pDevice)
 {	
	if(pDotNet){	
		pDotNet(pDevice);
	}
	return pEndSceneTramp(pDevice);
 }


 __declspec(dllexport) void EndSceneDotNet(int callbackPointerAddr)
 {
	 pDotNet = (EndSceneFptr)callbackPointerAddr;
	 FILE_LOG(logINFO) << "EndSceneDotNet.  Installed Callback." << (int)pDotNet;
 }

 __declspec(dllexport) void PresentD3D11DotNet(int callbackPointerAddr)
 {
//	 pD3D11DotNet = (D3D11PresentFptr)callbackPointerAddr;
//	 FILE_LOG(logINFO) << "PresentD3D11DotNet.  Installed Callback." << (int)pD3D11DotNet;
 }
 

__declspec(dllexport) int HookD3d9(wchar_t* asemblyBase) 
{	
	// first off we check if either of the modules we want are loaded
	HMODULE hDll = ::GetModuleHandle(L"d3d9.dll");	// TODO: we will want to check which d3d/ogl/gdi is loaded and just get a handle to the module..
	if( !hDll ){
		return DLL_LOAD_FAIL;
	}
	if( pDotNet ){	// we already have this process hooked
		return ALREADY_HOOKED;
	}

	TCHAR szPath[MAX_PATH];
	HRESULT hRes = SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, szPath);
	if ( !SUCCEEDED(hRes) ){
		return RET_FAIL;
	}
	TCHAR szExeFile[MAX_PATH];
	::GetModuleFileName(0, szExeFile, MAX_PATH);
	LPTSTR exe = ::PathFindFileName(szExeFile);

	TCHAR szLogFile[MAX_PATH];
	wsprintf(szLogFile, L"_hook_%s.txt",exe);
	PathAppend(szPath,szLogFile);

	FILELog::ReportingLevel() = logDEBUG3;
	FILE* log_fd = fopen( bstr_t( szPath ) , "a" );
	Output2FILE::Stream() = log_fd;
	
	FILE_LOG(logINFO) << "START .Net Runtime";
		
	HKEY   hkey;
	DWORD  dwDisposition; 
	DWORD dwType, dwSize;

	_host.RuntimeHostV4(L"v4.0.30319", asemblyBase, L"SlimHook3D", L"SlimHook3D.D3D9Hooks", asemblyBase);
	
	pDirect3DCreate9 = (Direct3DCreate9Fptr)::GetProcAddress(hDll, "Direct3DCreate9");
	if (pDirect3DCreate9 == NULL) 
	{
		FILE_LOG(logERROR) << "Unable to find Direct3DCreate9";
		return 1;
	}
	IDirect3D9 *pD3D = pDirect3DCreate9(D3D_SDK_VERSION);
	if( !pD3D){
		FILE_LOG(logERROR) <<"Direct3DCreate9 FAILED";
	}

	// get IDirect3DDevice9
	D3DDISPLAYMODE d3ddm;
	hRes = pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm );
    if (FAILED(hRes))
	{
		FILE_LOG(logERROR) <<"GetAdapterDisplayMode failed: 0x" << std::hex << hRes;
		return 1;
    }
	D3DPRESENT_PARAMETERS d3dpp; 
    ZeroMemory( &d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = true;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = d3ddm.Format;	
	// need to get the HWND of this process
	
	HWND hwnd = ::CreateWindowA("STATIC","dummy",WS_VISIBLE,-1000,-1000,100,100,NULL,NULL,NULL,NULL);
	::SetWindowTextA(hwnd,"Dummy Window!");
	
	IDirect3DDevice9 *pD3DDevice;
	hRes = pD3D->CreateDevice( 
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		hwnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp, &pD3DDevice);

    if (FAILED(hRes)){
		FILE_LOG(logERROR) << "CreateDevice failed: 0x" <<std::hex << hRes;
		return 1;
    }
	UINT_PTR* pVTable  = (UINT_PTR*)(*((UINT_PTR*)&(*pD3DDevice)));
	
	// dont need to hook present...
	/*
	pPresentOriginal = (PresentFptr)*(DWORD*)&pVTable[VT_PRESENT];	
	pPresentTramp = nCodeHook.createHook(*pPresentOriginal, (PresentFptr)&MyPresent);
	if(!pPresentTramp){
		FILE_LOG(logERROR) << "failed to hook Present";
	}
	*/
	pEndSceneOriginal = (EndSceneFptr)*(DWORD*)&pVTable[VT_ENDSCENE];		
	pEndSceneTramp = nCodeHook.createHook(*pEndSceneOriginal, (EndSceneFptr)&MyEndScene);
	if(!pEndSceneTramp){
		FILE_LOG(logERROR) << "failed to hook EndScene";
	}	
	
	pD3DDevice->Release();
	pD3D->Release();
		
	return 0;
}



__declspec(dllexport) int WINAPI Inject(int pid, wchar_t* asemblyBase)
{	
	HANDLE hThread;
	char    szLibPath[_MAX_PATH];  // The name of our "LibSpy.dll" module
								   // (including full path!);
	void*   pLibRemote;   // The address (in the remote process) where 
						  // szLibPath will be copied to;
	DWORD   hLibModule;   // Base address of loaded module (==HMODULE);
	HMODULE hKernel32 = ::GetModuleHandle(L"Kernel32");

	TCHAR modulePath[_MAX_PATH];
	GetModuleFileName(NULL, modulePath, _MAX_PATH);	
	PathRemoveFileSpec(modulePath);
	PathAppend(modulePath,L"inject.dll");

	std::stringstream ss;
	ss << asemblyBase << "_remotelog.txt";
	std::ofstream myfile;
	myfile.open (ss.str());
	
	myfile<<"Open file"<<std::endl;

	wcstombs(szLibPath, modulePath, _MAX_PATH);

	HANDLE hProcess = ::OpenProcess( PROCESS_ALL_ACCESS,FALSE, pid );
	if(!hProcess){
		std::cout<<"Could not find process. " << pid <<std::endl;
		return 0;
	}

	// 1. Allocate memory in the remote process for szLibPath
	// 2. Write szLibPath to the allocated memory
	pLibRemote = ::VirtualAllocEx( hProcess, NULL, sizeof(szLibPath), MEM_COMMIT, PAGE_READWRITE );

	if(!pLibRemote){
		DWORD err = ::GetLastError();
		myfile<<"VirtualAllocEx Failed CODE: "<< std::hex<< err<<std::endl;
	}

	::WriteProcessMemory( hProcess, pLibRemote, (void*)szLibPath, sizeof(szLibPath), NULL );


	// Load "inject.dll" into the remote process
	// (via CreateRemoteThread & LoadLibrary)
	myfile<<"Create Remote Thread 1"<<std::endl;	 
	hThread = ::CreateRemoteThread( hProcess, NULL, 0,(LPTHREAD_START_ROUTINE) ::GetProcAddress( hKernel32,"LoadLibraryA" ),
									pLibRemote, 0, NULL );
	::WaitForSingleObject( hThread, INFINITE );

	// Get handle of the loaded module
	if(!::GetExitCodeThread( hThread, &hLibModule )){
		myfile<<"GetExitCodeThread Failed"<<std::endl;
		return 0;
	}

	myfile<< "Module loaded at: " << std::hex << hLibModule << std::endl;

	// Clean up
	::CloseHandle( hThread );

	//HMODULE hLoaded = ::LoadLibraryA(szLibPath);
	HMODULE hLoaded = ::GetModuleHandleA(szLibPath);
	if(!hLoaded){
		DWORD err = ::GetLastError();
		myfile<<"GetModuleHandle Failed CODE: "<< std::hex<< err<<std::endl;
	}
	FARPROC hookProc = ::GetProcAddress( hLoaded,"HookD3d9" );
	if( !hookProc ){
		DWORD err = ::GetLastError();
		myfile<<"GetProcAddress Failed CODE: "<< std::hex<< err<<std::endl;
		return 0;
	}
	::WriteProcessMemory( hProcess, pLibRemote, (void*)asemblyBase, sizeof(szLibPath), NULL );

	DWORD offset = (char*)hookProc - (char*)hLoaded;
	myfile<< std::hex << "Offset: " << offset <<std::endl;

	// Call the real action now that we are loaded.  We can not do anything interesting from the dllMain so we need another exprot to call
	DWORD entry = (DWORD)hLibModule+offset;
	myfile<<"Create Remote Thread 2 at entry: "<< std::hex << entry  <<std::endl;
	HANDLE hThread2 = ::CreateRemoteThread( hProcess, NULL, 0,(LPTHREAD_START_ROUTINE)entry, pLibRemote, 0, NULL );
	::WaitForSingleObject( hThread2, INFINITE );
	
	::GetExitCodeThread( hThread2, &hLibModule );	// Get handle of the loaded module	
	::CloseHandle( hThread2 ); // Clean up		
	::VirtualFreeEx( hProcess, pLibRemote, sizeof(szLibPath), MEM_RELEASE ); // Clean up LoadLibrary Call

	myfile.close();
}


// CA - we can not do an inject from DllMain.  
// There are a number of things that we can not do inside DllMain.  Loading other libraries is NOT one of them.
// This is why we have to call our other exported function once the dll is loaded into the target process memory space.
BOOL APIENTRY DllMain( HMODULE hModule,DWORD  ul_reason_for_call,LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_THREAD_ATTACH:
		case DLL_PROCESS_ATTACH:
			
		break;								
	
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

