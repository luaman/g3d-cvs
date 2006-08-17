######################################################################
# DIRECTORY:	G3D
# PROGRAMMER:	Daniel Hilferty
# CREATED:	3 August 2006
# MODIFIED:	15 August 2006
#
# DESCRIPTION:
#	Project build file.  This file specifies a minimum cmake version
# due to use of advanced features and certain bugs on earlier versions.
######################################################################
# Minimum CMake Version
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

# Set Project Name
PROJECT(G3D)
# Include Standard Modules to find certain libraries
INCLUDE (FindOpenGL)

# Find SDL library for use on Unix Systems
IF(UNIX)
    INCLUDE (FindSDL)
    FIND_PACKAGE(SDL REQUIRED)
    # Specify include directories
    INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
ENDIF (UNIX)

# Find OpenGL 
FIND_PACKAGE(OpenGL REQUIRED)
# Build jpeg, png and zlib libraries if using windows
IF (WIN32)
    SUBDIRS (libjpeg.lib libpng.lib zlib.lib)
	SET (LIB_INCLUDE_DIR ${G3D_SOURCE_DIR}/libpng.lib/include/libpng
				   ${G3D_SOURCE_DIR}/zlib.lib/include/zlib
				   ${G3D_SOURCE_DIR}/libjpeg.lib/include/libjpeg)
	SET (JPEG_LIBRARIES jpeg)
	SET (PNG_LIBRARIES png zlib)
	SET (ZLIB_LIBRARIES zlib)
ENDIF (WIN32)

# If on a unix system, find the required libraries.  On windows systems
# they will simply be built. Unix covers both OS X aND Linux.
IF(UNIX)
	INCLUDE (FindPNG)
	INCLUDE (FindJPEG)
	FIND_PACKAGE(PNG)
	FIND_PACKAGE(JPEG)
ENDIF (UNIX)

# Set some basic CMake variables
SET (G3D_INSTALL_DIR ${G3D_BINARY_DIR}/install)
SET (LIBRARY_OUTPUT_PATH ${G3D_INSTALL_DIR}/lib)
SET (EXECUTABLE_OUTPUT_PATH ${G3D_INSTALL_DIR}/bin)

INCLUDE_DIRECTORIES(${G3D_SOURCE_DIR}/G3D.lib/include 
                    ${G3D_SOURCE_DIR}/GLG3D.lib/include
                    ${PNG_INCLUDE_DIR}
			        ${LIB_INCLUDE_DIR})

# Specify debug postfix for all debug libraries
# Note: The position of this specification guarantees that zlib, jpeg and png 
# libraries will not have this postfix.
SET(CMAKE_DEBUG_POSTFIX "-debug")

# Specify library subdirectory targets
SUBDIRS(GLG3D.lib G3D.lib test gfxmeter)
