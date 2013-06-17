slimhook
========

Demonstration of dll injection.  As well loading .net runtime and calling .net code.  Example hijacking d3d9 dll and altering rendering of games.

This project has 2 goals.  The first is to have a simple clean interface for performing DLL injection on a windows shared library.  The second is to load and execute your custom code in the .net runtime.

Last this project puts these 2 concepts together an performs a DLL injection on a direct3d video game.  It then deals with the d3d driver using the SlimDX framework in .net.  In this simple example we turn your triple A PC game graphics into “toon” style shading.  Which is to say, we reduce the color space significantly using a technique described here: http://en.wikipedia.org/wiki/Posterization

## Requirements:
* __N-CodeHook__: http://newgre.net/ncodehook 
This is included in this project since there is a free license to do what you want with it.  N-CodeHook is based on Microsoft detours(http://research.microsoft.com/en-us/projects/detours/).. but is completely free.  The main advantage to N-CodeHook is the inline patching takes care of your “trampoline”.  More info on this is availabe here: http://newgre.net/node/5

* __SlimDX__
SlimDX is a free open source framework that enables developers to easily build DirectX applications using .NET technologies such as C#, VB.NET, and IronPython.
http://slimdx.org/
You need to goto the site and download the latest 4.0 runtime version of the library in order to run the demo code.


## There are 3 projects in the solution.

* __inject.prog__ (Native C++ dll)
This is the native C++ dll that will perform the dll injection on the target process.  This is also responsible for loading the .net runtime and registering the function callbacks

* __SlimHook3D__ (C# .net)
This is the code we want to execute in the targeted applications memory space.  In the case of this demo we require SlimDX and perform graphics operations on the frame buffer of a hooked video game.

* __WinHookApp__ (C# ,net)
Simple WinForm application that allow you to put in the process ID of the target process.  This will then start the dll injection process.

## How it works

The WinHookApp includes “System.Runtime.InteropServices” to load the symbols from our inject.dll.  This can be seen with the following code

    [DllImport("inject.dll", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.StdCall)]
    internal static extern int Inject(int pid, StringBuilder amsBase);

Here we are loading the function that takes to parameters.  The PID of the process we want to inject and the assembly base for the .net assembly that we want to load.

Once the inject function is called
    __declspec(dllexport) int WINAPI Inject(int pid, wchar_t* asemblyBase)

We start the process of DLL injection.  We intend to inject the same dll we are executing from, namely inject.dll.

We start by getting a handle to the kernel32 lib
    
    HMODULE hKernel32 = ::GetModuleHandle(L"Kernel32");
  
Next we call OpenProcess on the process we are trying to inject

    HANDLE hProcess = ::OpenProcess( PROCESS_ALL_ACCESS,FALSE, pid );

Next we allocate memory inside that process with enouch room to hold the path to this library

    pLibRemote = ::VirtualAllocEx( hProcess, NULL, sizeof(szLibPath), MEM_COMMIT, PAGE_READWRITE );
    
Now we write the path to this lib in the memory we just allocated.

    ::WriteProcessMemory( hProcess, pLibRemote, (void*)szLibPath, sizeof(szLibPath), NULL );
    
From here we create a remote thread that will call "LoadLibraryA" passing it the location of the path we just wrote

    hThread = ::CreateRemoteThread( hProcess, NULL, 0,(LPTHREAD_START_ROUTINE) ::GetProcAddress( hKernel32,"LoadLibraryA" ),pLibRemote, 0, NULL );
    
We now should have this dll loaded into the remote process.  The next thing we need to do is call a method in our dll.  In this case we want to call "HookD3d9".  We first get the address of the function.

    FARPROC hookProc = ::GetProcAddress( hLoaded,"HookD3d9" );
    
Now we load the parameters to the function in the same manore that we did before.  This parameter will be the base path for the .net dll asembly.

    ::WriteProcessMemory( hProcess, pLibRemote, (void*)asemblyBase, sizeof(szLibPath), NULL );
    
We now calculate the offset to our function "HookD3d9" and call the function with passing in the parameter.

    DWORD offset = (char*)hookProc - (char*)hLoaded;
    myfile<< std::hex << "Offset: " << offset <<std::endl;

	  // Call the real action now that we are loaded.  We can not do anything interesting from the dllMain so we need another exprot to call
	  DWORD entry = (DWORD)hLibModule+offset;
	  myfile<<"Create Remote Thread 2 at entry: "<< std::hex << entry  <<std::endl;
	  HANDLE hThread2 = ::CreateRemoteThread( hProcess, NULL, 0,(LPTHREAD_START_ROUTINE)entry, pLibRemote, 0, NULL );
    
We now will be executing "HookD3d9" inside of the other process.

## Loading and running the .NET runtime

Most of this can be obtained from just looking at the code... but if there is any interest in this let me know and I will finish this doc ;)


