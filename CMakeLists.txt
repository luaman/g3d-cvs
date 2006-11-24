######################################################################
#
# Main CMakeLists.txt file.  This file specifies a minimum cmake version
# due to use of advanced features and certain bugs on earlier versions.
#
# @maintainer Daniel Hilferty, djhilferty@sourceforge.net
#
# @created 2006-08-18
# @edited  2006-09-01
#
######################################################################

# Minimum CMake Version
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

# Set Project Name
PROJECT(G3D)

SET(CMAKE_DEBUG_POSTFIX "d")

# Include Modules to find packages
INCLUDE (FindOpenGL)
INCLUDE (FindSDL)
INCLUDE (FindPNG)
INCLUDE (FindZLIB)
INCLUDE (FindJPEG)
INCLUDE (FindX11)

# Include Standard Modules to find certain libraries
FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(SDL REQUIRED)
FIND_PACKAGE(PNG)
FIND_PACKAGE(ZLIB)
FIND_PACKAGE(JPEG)
FIND_PACKAGE(X11)

IF(NOT PLATFORM_DIR)
    SET (PLATFORM_DIR ".")
ENDIF(NOT PLATFORM_DIR)

MESSAGE ("Platform Directory:   " ${PLATFORM_DIR})

# Set library and executable output paths
SET (LIBRARY_OUTPUT_PATH ${G3D_BINARY_DIR}/lib)
SET (EXECUTABLE_OUTPUT_PATH ${G3D_BINARY_DIR}/bin)
SET (G3D_INSTALL_DIR ${G3D_SOURCE_DIR}/build/install)

######################################################################
# Configure environment specific settings

IF (NOT ZLIB_FOUND)
    SET (ZLIB_LIBRARIES zlib)
    SET (ZLIB_INCLUDE_DIR ${G3D_SOURCE_DIR}/zlib.lib/include/zlib)
    INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIR})
    ADD_SUBDIRECTORY (zlib.lib)
ENDIF (NOT ZLIB_FOUND)

IF (NOT PNG_FOUND)
    SET (PNG_LIBRARIES png)
    SET (PNG_INCLUDE_DIR ${G3D_SOURCE_DIR}/png.lib/include)
    INCLUDE_DIRECTORIES(${PNG_INCLUDE_DIR})
    ADD_SUBDIRECTORY (png.lib)
ENDIF (NOT PNG_FOUND)

IF (NOT JPEG_FOUND)
    SET (JPEG_LIBRARIES jpeg)
    SET (JPEG_INCLUDE_DIR ${G3D_SOURCE_DIR}/jpeg.lib/include)
    INCLUDE_DIRECTORIES(${JPEG_INCLUDE_DIR})
    ADD_SUBDIRECTORY (jpeg.lib)
ENDIF (NOT JPEG_FOUND)

INCLUDE_DIRECTORIES(${X11_INCLUDE_DIR}
                    ${PNG_INCLUDE_DIR}
                    ${ZLIB_INCLUDE_DIR}
                    ${JPEG_INCLUDE_DIR}
                    ${G3D_SOURCE_DIR}/G3D.lib/include 
                    ${G3D_SOURCE_DIR}/GLG3D.lib/include
                    ${OPENGL_INCLUDE_DIR}
                    ${SDL_INCLUDE_DIR})

IF (APPLE)
    SET (CARBON_FRAMEWORK "-framework Carbon")
ENDIF(APPLE)

######################################################################
# BUILD DIRECTORIES

# Specify library subdirectory targets
ADD_SUBDIRECTORY (GLG3D.lib)
ADD_SUBDIRECTORY (G3D.lib)

# Determine whether to build executables
IF (BUILD_BINARIES)
    ADD_SUBDIRECTORY (test)
    ADD_SUBDIRECTORY (tools/gfxmeter)
ENDIF (BUILD_BINARIES)
