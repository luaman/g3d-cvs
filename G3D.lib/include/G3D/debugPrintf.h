/**
 @file debugPrintf.h
 
 @maintainer Morgan McGuire, matrix@graphics3d.com
 
 @created 2001-08-26
 @edited  2007-07-20

 Copyright 2000-2007, Morgan McGuire.
 All rights reserved.
 */

#ifndef G3D_DEBUGPRINTF_H
#define G3D_DEBUGPRINTF_H

#include "G3D/platform.h"
#include <stdio.h>
#include <cstdarg>
#include "G3D/format.h"
#include <string>

namespace G3D {

typedef void (*ConsolePrintHook)(const std::string&);

namespace _internal {
    extern ConsolePrintHook _consolePrintHook;
}

/** Called by consolePrintf after the log and terminal have been written to.
 Used by GConsole to intercept printing routines.*/
void setConsolePrintHook(ConsolePrintHook h);

ConsolePrintHook consolePrintHook();

/**
   Sends output to the log and to the last GConsole instantiated.
   Under visual studio, the output also appears in the VS debug pane.
   On unix-based operating systems the output is also sent to stderr.

   Guarantees that the output has been flushed by the time the routine
   returns.
   @sa G3D::logPrintf, G3D::screenPrintf
 */
void __cdecl consolePrintf(const char* fmt ...) G3D_CHECK_PRINTF_ARGS;

/**
 @deprecated See consolePrintf and screenPrintf
 */
inline void __cdecl debugPrintf(const char* fmt ...) {
    va_list argList;
    va_start(argList, fmt);
    std::string s = G3D::vformat(fmt, argList);
    va_end(argList);
    consolePrintf("%s", s.c_str());
}

} // namespace G3D

#endif

