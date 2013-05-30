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

