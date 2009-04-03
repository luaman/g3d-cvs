/**
 @file G3D/glheaders.h

 #includes the OpenGL headers

 @maintainer Morgan McGuire, morgan@cs.williams.edu

 @created 2002-08-07
 @edited  2007-06-01

 G3D Library
 Copyright 2002-2007, Morgan McGuire.
 All rights reserved.
*/

#ifndef G3D_GLHEADERS_H
#define G3D_GLHEADERS_H

#include "G3D/platform.h"
#include "GLG3D/GL/glew.h"

#ifdef G3D_WIN32
#   include "GLG3D/GL/wglew.h"
#elif defined(G3D_OSX) || defined(G3D_FREEBSDF) || defined(G3D_LINUX)
#   include "GLG3D/GL/glxew.h"
#endif

#endif
