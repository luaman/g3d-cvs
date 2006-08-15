/** 
  @file G3DAll.h
 
  Includes all G3D and GLG3D files and uses the G3D namespace.
  
  This requires OpenGL and SDL headers.  If you don't want all of this,
  #include <G3D.h> separately.

  @maintainer Morgan McGuire, matrix@graphics3d.com
 
  @created 2002-01-01
  @edited  2006-08-13

 Copyright 2000-2006, Morgan McGuire.
 All rights reserved.
 */

#ifndef G3D_G3DALL_H
#define G3D_G3DALL_H

#include "G3D/G3D.h"
#include "GLG3D/GLG3D.h"

using namespace G3D;
// The following statements should be not be necessary. 
// However, we experienced problems under MSVC 6.00 SP4 
// when compiling against certain code in its own 
// namespace-- the 'using namespace G3D' failed to bring some 
// of the names, like Texture, into the global namespace.
using G3D::Texture;         
using G3D::TextureRef;
using G3D::RenderDevice;
using G3D::AMPM;

#endif
