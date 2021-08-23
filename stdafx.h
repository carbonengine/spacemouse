// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#if _WIN32

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

#define NO_SPACE_MOUSE 0

/* SpaceWare Specific Includes */
#include "spwmacro.h"  /* Common macros used by SpaceWare functions. */
#include "si.h"        /* Required for any SpaceWare support within an app.*/
#include "siapp.h"     /* Required for siapp.lib symbols */

#pragma warning( push )
#pragma warning( disable: 4995 )
#include "siappcmd.h"
#pragma warning( pop )

#elif __APPLE__

#include <vector>
#include <mach-o/dyld.h>

#define NO_SPACE_MOUSE 0

#else

#define NO_SPACE_MOUSE 1

#endif

#include "Python.h"

