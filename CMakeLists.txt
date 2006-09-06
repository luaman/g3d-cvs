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

SET(CMAKE_DEBUG_POSTFIX "-debug")

# Set library and executable output paths
IF(CMAKE_BUILD_TYPE)
    SET (LIBRARY_OUTPUT_PATH ${G3D_BINARY_DIR}/${CMAKE_BUILD_TYPE}/lib)
    SET (EXECUTABLE_OUTPUT_PATH ${G3D_BINARY_DIR}/${CMAKE_BUILD_TYPE}/bin)
ELSE(CMAKE_BUILD_TYPE)
    SET (LIBRARY_OUTPUT_PATH ${G3D_BINARY_DIR}/Release/lib)
    SET (EXECUTABLE_OUTPUT_PATH ${G3D_BINARY_DIR}/Release/bin)
ENDIF(CMAKE_BUILD_TYPE)

######################################################################
# Configure environment specific settings
IF (UNIX)

    # Build jpeg.lib for the Apple build
    IF (OSX)
        ADD_SUBDIRECTORY (jpeg.lib)
        INCLUDE_DIRECTORIES(${G3D_SOURCE_DIR}/jpeg.lib/include/jpeg)
    ELSE (OSX)
        INCLUDE (FindJPEG)
        FIND_PACKAGE(JPEG)
    ENDIF(OSX)

    INCLUDE (FindPNG)
    INCLUDE (FindSDL)
    INCLUDE (FindZLIB)

    FIND_PACKAGE(PNG)
    FIND_PACKAGE(SDL REQUIRED)
    FIND_PACKAGE(ZLIB)

    # Specify SDL as an included directory
    INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})

ELSE(UNIX)

    IF (WIN32)

        # Add include directories for jpeg, png and zlib libraries
	INCLUDE_DIRECTORIES(${G3D_SOURCE_DIR}/png.lib/include/png
                            ${G3D_SOURCE_DIR}/zlib.lib/include/zlib
                            ${G3D_SOURCE_DIR}/jpeg.lib/include/jpeg)

        # Set the variables normally defined in FIND_PACKAGE on Unix
        SET (JPEG_LIBRARIES jpeg)
        SET (PNG_LIBRARIES png)
        SET (ZLIB_LIBRARIES zlib)

        # Add library subdirectories
        ADD_SUBDIRECTORY (zlib.lib)
        ADD_SUBDIRECTORY (png.lib)
        ADD_SUBDIRECTORY (jpeg.lib)

    ENDIF (WIN32)

ENDIF(UNIX)

# Include Standard Modules to find certain libraries
INCLUDE (FindOpenGL)
FIND_PACKAGE(OpenGL REQUIRED)


# Set GLG3D.lib and G3D.lib include directories
INCLUDE_DIRECTORIES(${G3D_SOURCE_DIR}/G3D.lib/include 
                    ${G3D_SOURCE_DIR}/GLG3D.lib/include)


######################################################################
# BUILD DIRECTORIES

# Specify library subdirectory targets
ADD_SUBDIRECTORY (GLG3D.lib)
ADD_SUBDIRECTORY (G3D.lib)

# Determine whether to build executables
IF (BUILD_BINARIES)
    ADD_SUBDIRECTORY (test)
    ADD_SUBDIRECTORY (gfxmeter)
ENDIF (BUILD_BINARIES)
