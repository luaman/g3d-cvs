/**
  @file NSAutoreleasePoolWrapper.mm

  @maintainer Morgan McGuire, morgan@graphics3d.com, Casey O'Donnell, caseyodonnell@gmail.com
  @created 2006-08-27
  @edited  2006-08-31
*/

#import <Foundation/NSAutoreleasePool.h>
#import "GLG3D/NSAutoreleasePoolWrapper.h"

NSAutoreleasePoolWrapper::NSAutoreleasePoolWrapper() : _pool(0) {
	_pool = [[NSAutoreleasePool alloc] init];
}

NSAutoreleasePoolWrapper::~NSAutoreleasePoolWrapper() {
	if(_pool)
	{
		[(NSAutoreleasePool*)_pool release];
	}
}

