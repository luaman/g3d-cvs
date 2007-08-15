# G3D project
#
# 
# Library Installation: G3D GLG3D PNG JPEG ZLIB ZIP


# Minimum CMake Version
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

# create project
# this creates G3D_BINARY_DIR and G3D_SOURCE_DIR set to top-level dir
PROJECT(G3D)

# helper macro for copying data files
MACRO(COPY_IF_DIFFERENT FROM_DIR TO_DIR TARGET)
#   TARGET   - target this command is associated with (runs when target runs)
IF (UNIX)
    ADD_CUSTOM_COMMAND(
        TARGET  ${TARGET}
        PRE_BUILD
        COMMAND cmakeutil ARGS copy_if_newer ${FROM_DIR} ${TO_DIR}
        WORKING_DIRECTORY ${G3D_SOURCE_DIR}
    )
ELSE (UNIX)
    ADD_CUSTOM_COMMAND(
        TARGET  ${TARGET}
        PRE_BUILD
        COMMAND python ARGS cmakeutil copy_if_newer ${FROM_DIR} ${TO_DIR}
        WORKING_DIRECTORY ${G3D_SOURCE_DIR}
    )
ENDIF (UNIX)    
ENDMACRO(COPY_IF_DIFFERENT FROM_DIR TO_DIR FILES TARGET)


# include cmake library find modules
INCLUDE (FindOpenGL)
INCLUDE (FindSDL)
INCLUDE (FindPNG)
INCLUDE (FindZLIB)
INCLUDE (FindJPEG)
INCLUDE (FindX11)

# find required library dependencies and setup library variables
# LIBRARY_LIBRARIES = library name (for referencing library during linking)
# LIBRARY_INCLUDE_DIR = library include path (for referencing library during compilation of dependents)
FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(SDL)
FIND_PACKAGE(PNG)
FIND_PACKAGE(ZLIB)
FIND_PACKAGE(JPEG)
FIND_PACKAGE(X11)

# set installation prefix
IF (NOT DEFINED CMAKE_INSTALL_PREFIX)
    SET(CMAKE_INSTALL_PREFIX "build/install")
ENDIF (NOT DEFINED CMAKE_INSTALL_PREFIX)

# set debug suffix
SET(CMAKE_DEBUG_POSTFIX "d")

# setup build directory name based on OS and compiler
IF (NOT PLATFORM_DIR)
    IF (MSVC80)
        SET (PLATFORM_DIR "win-vc8.0")
    ENDIF (MSVC80)
    IF (MSVC71)
        SET (PLATFORM_DIR "win-vc7.1")
    ENDIF (MSVC71)
    IF (MINGW)
        SET (PLATFORM_DIR "win-mingw")
    ENDIF (MINGW)
    IF (APPLE)
        # xcode is assumed for osx for now
	    SET (PLATFORM_DIR "osx-xcode")
    ENDIF (APPLE)
    IF (UNIX AND NOT APPLE)
        SET (PLATFORM_DIR "linux-g++")
    ENDIF (UNIX AND NOT APPLE)
    # provide a default
    IF (NOT PLATFORM_DIR)
        SET (PLATFORM_DIR "${CMAKE_SYSTEM_NAME}-unknown")
    ENDIF(NOT PLATFORM_DIR)
ENDIF (NOT PLATFORM_DIR)
MESSAGE ("Output Platform Build Directory:   build/${PLATFORM_DIR}")

# set library and installation output paths (executable path will be per-target)
SET (LIBRARY_OUTPUT_PATH ${G3D_BINARY_DIR}/build/${PLATFORM_DIR}/lib)
SET (G3D_INSTALL_DIR ${G3D_SOURCE_DIR}/build/install)


# add zlib library on systems that need it
IF (NOT ZLIB_FOUND)
    SET (ZLIB_LIBRARIES zlib)
    SET (ZLIB_INCLUDE_DIR ${G3D_SOURCE_DIR}/zlib.lib/include)
    ADD_SUBDIRECTORY (zlib.lib)
ENDIF (NOT ZLIB_FOUND)

# add png library on systems that need it
IF (NOT PNG_FOUND)
    SET (PNG_LIBRARIES png)
    SET (PNG_INCLUDE_DIR ${G3D_SOURCE_DIR}/png.lib/include)
    ADD_SUBDIRECTORY (png.lib)
ENDIF (NOT PNG_FOUND)

# add jpeg library on systems that need it
IF (NOT JPEG_FOUND)
    SET (JPEG_LIBRARIES jpeg)
    SET (JPEG_INCLUDE_DIR ${G3D_SOURCE_DIR}/jpeg.lib/include)
    ADD_SUBDIRECTORY (jpeg.lib)
ENDIF (NOT JPEG_FOUND)

# add zip library
SET (ZIP_LIBRARIES zip)
SET (ZIP_INCLUDE_DIR ${G3D_SOURCE_DIR}/zip.lib/include)
ADD_SUBDIRECTORY (zip.lib)

# helper variable to easily reference all required include paths
# when compiling executables that use g3d project
SET (ALL_EXE_INCLUDE_DIRS   ${OPENGL_INCLUDE_DIR}
                            ${X11_INCLUDE_DIR}
                            ${PNG_INCLUDE_DIR}
                            ${JPEG_INCLUDE_DIR}
                            ${ZLIB_INCLUDE_DIR}
                            ${ZIP_INCLUDE_DIR}
                            ${G3D_SOURCE_DIR}/G3D.lib/include
                            ${G3D_SOURCE_DIR}/GLG3D.lib/include)

IF (APPLE)
    LIST(APPEND ALL_EXE_INCLUDE_DIRS ${SDL_INCLUDE_DIR})
ENDIF(APPLE)

# helper variable to link all needed platform-specific libraries for executables
SET (ALL_EXE_LIBRARIES GLG3D G3D ${PNG_LIBRARIES} ${ZIP_LIBRARIES} ${ZLIB_LIBRARIES} ${JPEG_LIBRARIES})
If (APPLE)
    LIST(APPEND ALL_EXE_LIBRARIES "-framework Carbon"  ${SDL_LIBRARY} ${OPENGL_LIBRARY})
ENDIF (APPLE)
IF (UNIX AND NOT APPLE)
    LIST(APPEND ALL_EXE_LIBRARIES ${X11_LIBRARIES} ${SDL_LIBRARY} ${OPENGL_LIBRARY})
ENDIF (UNIX AND NOT APPLE)
IF (MSVC)
    LIST(APPEND ALL_EXE_LIBRARIES ${SDL_LIBRARY} ${OPENGL_LIBRARY} ole32 ws2_32 winmm imagehlp gdi32 user32 kernel32 version advapi32)
ENDIF (MSVC)
IF (MINGW)
    LIST(APPEND ALL_EXE_LIBRARIES ${SDL_LIBRARY} ${OPENGL_LIBRARY} ole32 ws2_32 winmm imagehlp gdi32 user32 kernel32 version advapi32)
ENDIF (MINGW)


# add glg3d and g3d libraries to build
ADD_SUBDIRECTORY (GLG3D.lib)
ADD_SUBDIRECTORY (G3D.lib)


# determine whether to add tool and demo targets
ADD_SUBDIRECTORY (test)
ADD_SUBDIRECTORY (tools/gfxmeter)
ADD_SUBDIRECTORY (tools/viewer)
ADD_SUBDIRECTORY (demos/starter)
ADD_SUBDIRECTORY (demos/models)

# add data-files installation
INSTALL(DIRECTORY data-files DESTINATION data PATTERN "CVS" EXCLUDE)