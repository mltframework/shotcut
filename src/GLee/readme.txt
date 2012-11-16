GLee
GL Easy Extension Library
Version 5.4

By Ben Woodhouse
http://elf-stone.com


LICENSE

Copyright (c)2009  Ben Woodhouse  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are 
met:
1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer as
the first lines of this file unmodified.
2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY BEN WOODHOUSE ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL BEN WOODHOUSE BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.



CONTACT

Send any general questions, bug reports etc to me (Ben Woodhouse):
  bw [at] elf-stone.com

Questions about the D port for GLee should go to Joel Anderson: 
  anderson [at] badmama.com.au
  
Questions about GLee on OS X should go to Tristam MacDonald
  swiftcoder [at] gmail.com
  


INTRODUCTION

GLee provides a simple interface for using extensions and core OpenGL 
functionality beyond OpenGL version 1.1, and automates the otherwise tedious 
process of linking function pointers. GLee works with both C and C++ 
compilers.

Because the code is automatically generated, the latest extensions can 
be included rapidly in new versions. Currently there is support 
for OpenGL up to 3.0 and almost all registered extensions. For a complete 
list of extensions, please see the accompanying extensionList.txt file. 

For extension specifications, please visit: 

   http://www.opengl.org/registry/


CREDITS

Tristam MacDonald for adding OS-X compatibility.

Martin Büchler (LousyPhreak on the GameDev.net forums) for 
a huge amount of testing, tweaking, suggestions and advice to get GLee to 
work properly with Linux. 

Daniel Jo (Ostsol on the OpenGL.org forums) for helping to get  
GLEE 2.1 working with the OGLSL extensions (ARB_shader_objects, 
ARB_vertex_shader and ARB_fragment_shader). 

Joel Anderson for his for his D port of GLee 3.0

Jacques Beaurain for pointing out a potential access violation in 
__GLeeGetExtensions. 


WHAT'S NEW IN GLEE 5.0

GLeeGen, the program that generates the code for the GLee library, has been
rewritten to support parsing of extension specifications. As a result, GLee 5.0 and upwards support 
all extensions for which there is a specification, regardless of that 
extension's inclusion in glext.h, wglext.h or glxext.h. 

GLee 5.0 adds support for forced extension loading through the 
GLeeForceLink function. This makes it possible to force the linking of 
extension functions (or core functions) which are not reported by the video driver. 

GLee now supports lazy loading of extensions, meaning it is no longer 
necessary to call GLeeInit() at initialisation time. 



HOW TO INSTALL 

MSVC 2003 binary version

  1. Copy GLee.lib to your visual C++ lib directory 
  (eg C:\Program Files\Microsoft Visual Studio .NET 2003\Vc7\PlatformSDK\Lib)

  2. Copy GLee.h to your visual C++ include\GL directory 
  (eg C:\Program Files\Microsoft Visual Studio .NET 2003\Vc7\PlatformSDK\include\gl)
  

Linux binary version (GLee-*-bin.tar.gz)
  (Installation has to be done as root)

  1. unpack GLee-*-bin.tar.gz
  (tar -xzf GLee-2.0-bin.tar.gz)

  2. Install with the inst script
  (cd glee && ./inst)

Linux source version (GLee-*-src.tar.gz)
  1. unpack GLee-*-src.tar.gz
  (tar -xzf GLee-2.0-src.tar.gz)

  2. cd to "glee" and run the gnu build chain
  (cd glee && ./configure && make)

  3. Become root (password needed)
  (su)

  4. Install the lib
  (make install)
  


HOW TO USE

If you're using the binary version then you'll need to link to 
glee.lib and #include <gl\glee.h> in your project. Otherwise, 
just add glee.h and glee.c in your project and #include "glee.h".
You can also build the library yourself using glee.c and glee.h. 

From this point onwards, you should have access to all OpenGL 
extensions and core functionality. 

You can query the availability of an extension like this:

if (GLEE_ARB_point_parameters)
{
   //GL_ARB_point_parameters is supported
   glPointParameterfARB(...);
}

WGL extensions have a GLEE_WGL_ prefix. For example, 
GLEE_WGL_ARB_pbuffer. GLX extensions work in a similar way. 

You can also query the OpenGL version:
if (GLEE_VERSION_1_3)
{
  //OpenGL 1.3 is supported 
  glLoadTransposeMatrixf(...) ; 
  ...
}



ERROR CHECKING

Optionally, you can add a call to GLeeInit() after your OpenGL 
initialisation code, however this is no longer required as of 
GLee 5.0. GLeeInit returns a boolean value to indicate success
or failure. 

If GLeeInit returns false you can get more detailed error 
information by calling GLeeGetErrorString(). This returns a pointer 
to a string which contains error description(s). 

At any time you can call GLeeGetExtStrGL() or GLeeGetExtStrWGL() 
(win32) or GLeeGetExtStrGLX() (linux) to retrieve a pointer to the 
corresponding extension list string.  



FORCED EXTENSION SUPPORT

By default, only those extensions reported by OpenGL's 
glGetString(GL_EXTENSIONS) are loaded by GLeeInit(). Likewise, 
only core functions whose version doesn't exceed the version number 
returned by glGetString(GL_VERSION) are loaded.

However, GLee 5.0 and above can load additional extension and core 
functions using the GLeeForceLink function:

	GLint GLeeForceLink(const char * extensionName)

For example, GLeeForceLink("GL_ARB_point_parameters") will attempt to 
load the ARB_point_parameters function. 

For core functions, you can use the GL version string for the version 
you wish to initialise. For example, GLeeForceLink( "GL_VERSION_2_0" ) 
will attempt to initialise OpenGL 2.0 core functions. 
	
Return Value:
  GLEE_LINK_COMPLETE is returned if all functions were 
  successfully linked.

  GLEE_LINK_PARTIAL is returned if only some functions were 
  linked successfully.
  
  GLEE_LINK_FAIL is returned if no functions could be linked
  successfully. 
  
Functions which could not be linked will be set to NULL. 

A successful link does not guarantee that a function will work 
correctly. If you want to be safe, only use extensions which are
reported by the driver. 



LINUX NOTES - by LousyPhreak

The binary version was compiled with gcc 3.3 and was also tested 
with gcc 3.2, but if you still have gcc 2.x you need the source 
version. If you don't know which version you have on your system just 
look at the output of 'gcc --version'.

You should be using the NVIDIA headers if you have the NVIDIA drivers
installed, and the MESA headers otherwise.

One more note:
The binary version might complain about missing glXGetProcAddressARB
on the linker stage, but if you get this error, use the source 
version and email me (lousyphreak [at] gmx.at).

Compiling on linux:
You just need to replace the linking of libGL by libGLee:
gcc main.cpp whatever.cpp -lGLee -o myproject

Everything else should be the same as on windows.



NOTES ON THE GLEE 3.0 D PORT FOR WIN32 - by Joel Anderson

1) Put the GLee.lib in the lib folder (ie C:\dmd\lib)
2) Put glee.d and glConstants.d in the include folder (ie create c:\glee 
and use the -IC:\glee on the compiler.) 
3) The D code is pretty much the same as the C code but you need to specify:

import glee; //Instead of #include "glee.h"

Notes
- The GLee.lib was built using dmc.  I did try the one that comes with the 
C version, but that doesn't work with D. You can compile the lib yourself 
using cbuild\go.bat
- I haven't tested everything.

You can contact me (Joel Anderson) at anderson@badmama.com.au

(21/01/2004) - AU


CHANGES

5.4   : Added 5 new extensions : 
            GL_EXT_texture_swizzle
            GL_NV_explicit_multisample
            GL_NV_transform_feedback2
            WGL_NV_gpu_affinity
            GLX_ARB_create_context
            
        Removed some duplicate extensions:
            GL_NV_geometry_program4 (use GL_NV_gpu_program4)
            GL_NV_vertex_program4   (use GL_NV_gpu_program4)
            GL_NV_fragment_program4 (use GL_NV_gpu_program4)
            WGL_NV_video_out        (use WGL_NV_video_output)
            
        Fixed GL_NV_gpu_program4 by merging with GL_NV_geometry_program4, 
        GL_NV_vertex_program4 and GL_NV_fragment_program4. 
        
        Added support for multiple extensions including the same functions, so 
        GL_EXT_geometry_shader4 and GL_NV_geometry_program4 can coexist without
        compile errors.  
          
        Fixed GL_EXT_geometry_shader4 so it no longer has missing functions and constants. 
        These were excluded previously to avoid clashes with GL_NV_vertex_program4, which 
        included a bunch of functions with the same names.
        
        Various GLeeGen/GLeeXMLGen fixes, mainly to fix issues with NVidia extensions, most
        notably:        
          Added support for overriding extension spec files so we can manually fix 
          buggy specs which don't follow the standard (like GL_NV_gpu_program4).
            

Reworked GLeeGen and GLeeXMLGen to allow overriding of 

5.33  : Fixed some GCC warnings 
        Fixed some OS-X warnings
        Fixed an issue in __GLeeGetExtensions which could cause two
          extensions to go missing on some platforms

5.32  : Fixed a potential access violation in __GLeeGetExtensions

5.31  : Fixed GLX compile error

5.3   : OpenGL 3.0 support

5.21  : Nvidia geometry shader extensions 

5.2   : OS-X support

5.1   : Fixed some ansi-C compatibility issues. 
		New GLeeGen version
		Added 5 new GL extensions and 1 new WGL extension. 

5.04  : Added the latest extensions

5.03  : Some minor compatibility fixes. 

		Malloc.h no longer used. 
		
		GLee no longer fails if platform-specific (GLX or WGL) extensions 
		could not be loaded (a problem with some cards apparently).

5.02  : Changed extension querying variable system. Removed GLEE_ENABLED() 
        extension checking macro introduced in 5.00, since it is no longer 
        needed

5.01  : Fixed GLX_SGIX_HYPERPIPE bug

5.00  : Rewrote GLeeGen to parse extension specs
        
        Added lazy loading for extensions
        
        Added experimental support

4.00  : Updated using extensions from glext.h version 26, wglext.h version 6, 
        and glxext.h version 10. OpenGL 2.0 is now supported. 

3.05  : Updated with glExt.h version 23 extensions. 

3.04  : Updated with glExt.h version 22 extensions. 

3.03  : Another linux compatibility bug fixed

3.02  : GLX typedef bug fixed

3.01  : Minor linux compatibility bugs fixed

3.0   : Removed STL and other C++ specific features to make GLee 
        compatible with C compilers 
        
        Added a number of WGL extensions that had previously been
        excluded due to a bug with GLeeGen (now fixed)

2.21  : Fixed VC6 compilation bug

2.2   : Added full OpenGL 1.5 support

2.1   : Added the OpenGL shading language extensions: 
        ARB_shader_objects, ARB_vertex_shader and ARB_fragment_shader. 
		
2.01  : Fixed missing description comment in header
        Fixed include guard

2.0   : Removed dependency on glext.h and wglext.h
        Added cross-platform code for support with linux      
        Fixed potential stability problems 

1.2   : Made library compatible with VC7 and VC7.1

1.12  : Cleaned up some commenting errors in GLeeGen

1.11  : Added functions to get WGL and GL extension strings.
        Fixed minor formatting errors caused by a bug in GLeeGen (now fixed)

1.1   : Added detailed error checking. 
        Fixed possible buffer overrun issue 

1.01  : GLeeInit no longer requires a device context handle.