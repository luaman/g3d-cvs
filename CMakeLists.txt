######################################################################
#
# Main CMakeLists.txt file.  This file specifies a minimum cmake version
# due to use of advanced features and certain bugs on earlier versions.
#
# @maintainer Daniel Hilferty, djhilferty@sourceforge.net
#
# @created 2006-08-18
# @edited  2006-08-18
#
######################################################################

# Minimum CMake Version
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

# Set Project Name
PROJECT(G3D)
######################################################################
# CONFIGURE LIBRARIES 

# Include Standard Modules to find certain libraries
INCLUDE (FindOpenGL)
FIND_PACKAGE(OpenGL REQUIRED)

# Find SDL library for use on Unix Systems
IF(UNIX)
    INCLUDE (FindSDL)
    FIND_PACKAGE(SDL REQUIRED)
    # Specify include directories
    INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
ENDIF (UNIX)

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

######################################################################
# SET VARIABLES

# Set some basic CMake variables
SET (G3D_INSTALL_DIR ${G3D_BINARY_DIR}/../)
SET (LIBRARY_OUTPUT_PATH ${G3D_INSTALL_DIR}/lib)
SET (EXECUTABLE_OUTPUT_PATH ${G3D_INSTALL_DIR}/bin/${CMAKE_BUILD_TYPE})

# Specify debug postfix for all debug libraries
# Note: The position of this specification guarantees that zlib, jpeg and png 
# libraries will not have this postfix.
SET(CMAKE_DEBUG_POSTFIX "-debug")

######################################################################
# INCLUDE DIRECTORIES

INCLUDE_DIRECTORIES(${G3D_SOURCE_DIR}/G3D.lib/include 
                    ${G3D_SOURCE_DIR}/GLG3D.lib/include
                    ${PNG_INCLUDE_DIR}
			        ${LIB_INCLUDE_DIR})

######################################################################
# BUILD DIRECTORIES

# Specify library subdirectory targets
SUBDIRS(GLG3D.lib G3D.lib)
MESSAGE ("VALUE" ${BUILD_BINARIES})
# Determine whether to build executables
IF (BUILD_BINARIES)
    SUBDIRS(test gfxmeter)
ENDIF (BUILD_BINARIES)
