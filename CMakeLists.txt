# G3D project
#
# 
# Library Installation: G3D GLG3D PNG JPEG ZLIB ZIP

#-----------------------------------------------------------------------------
#
# We don't allow in-source builds. 
#
IF(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  MESSAGE(
    FATAL_ERROR 
    "\nCMake generation for G3D is not allowed within the source directory!\n"
    "Make a build directory, and run cmake from there. e.g.\n\n"
    "\tmkdir build; cd build; cmake ../\n"
    )
ENDIF(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})

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
        COMMAND ${G3D_SOURCE_DIR}/cmakeutil ARGS copy_if_newer ${FROM_DIR} ${TO_DIR}
        WORKING_DIRECTORY ${G3D_BINARY_DIR}
    )
ELSE (UNIX)
    ADD_CUSTOM_COMMAND(
        TARGET  ${TARGET}
        PRE_BUILD
        COMMAND python ARGS ${G3D_SOURCE_DIR}/cmakeutil copy_if_newer ${FROM_DIR} ${TO_DIR}
        WORKING_DIRECTORY ${G3D_BINARY_DIR}
    )
ENDIF (UNIX)    
ENDMACRO(COPY_IF_DIFFERENT FROM_DIR TO_DIR FILES TARGET)

# find required library dependencies and setup library variables
FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(SDL)
FIND_PACKAGE(PNG)
FIND_PACKAGE(ZLIB)
FIND_PACKAGE(JPEG)
FIND_PACKAGE(X11)

# set installation prefix
# IF (NOT DEFINED CMAKE_INSTALL_PREFIX)
    # SET(CMAKE_INSTALL_PREFIX "build/install")
# ENDIF (NOT DEFINED CMAKE_INSTALL_PREFIX)

# set debug suffix
SET(CMAKE_DEBUG_POSTFIX "d")

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
SET (ZIP_FOUND 1)

# helper variable to easily reference all required include paths
# when compiling executables that use g3d project
SET (ALL_EXE_INCLUDE_DIRS   ${OPENGL_INCLUDE_DIR}
                            ${PNG_INCLUDE_DIR}
                            ${JPEG_INCLUDE_DIR}
                            ${ZLIB_INCLUDE_DIR}
                            ${ZIP_INCLUDE_DIR}
                            ${G3D_SOURCE_DIR}/G3D.lib/include
                            ${G3D_SOURCE_DIR}/GLG3D.lib/include)

# helper variable to link all needed platform-specific libraries for executables
SET (ALL_EXE_LIBRARIES GLG3D G3D ${PNG_LIBRARIES} ${ZIP_LIBRARIES} ${ZLIB_LIBRARIES} ${JPEG_LIBRARIES})
If (APPLE)
    FIND_LIBRARY(CARBON_LIBRARY Carbon REQUIRED)
    LIST(APPEND ALL_EXE_LIBRARIES ${CARBON_LIBRARY} ${OPENGL_LIBRARY})
ENDIF (APPLE)
IF (UNIX AND NOT APPLE)
    LIST(APPEND ALL_EXE_LIBRARIES ${X11_LIBRARIES} ${SDL_LIBRARY} ${OPENGL_LIBRARY})
    LIST(APPEND ALL_EXE_INCLUDE_DIRS ${X11_INCLUDE_DIR})
ENDIF (UNIX AND NOT APPLE)
IF (MSVC)
    LIST(APPEND ALL_EXE_LIBRARIES ${SDL_LIBRARY} ${OPENGL_LIBRARY} ole32 ws2_32 winmm imagehlp gdi32 user32 kernel32 version advapi32)
ENDIF (MSVC)
IF (MINGW)
    LIST(APPEND ALL_EXE_LIBRARIES ${SDL_LIBRARY} ${OPENGL_LIBRARY} ole32 ws2_32 winmm imagehlp gdi32 user32 kernel32 version advapi32)
ENDIF (MINGW)

# OS X specific g++ compile/link flags
IF (APPLE)
  # fix for Leopard bug
  # (http://wiki.finkproject.org/index.php/Fink:Packaging:Preparing_for_10.5#OpenGL_Bug)
  SET(CMAKE_EXE_LINKER_FLAGS
    "-Wl,-dylib_file,/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib:/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib")
  # make compile output a little cleaner, because of the recently deprecated
  # CarbonWindow features
  SET(CMAKE_CXX_FLAGS "-Wno-deprecated -Wno-deprecated-declarations")
ENDIF (APPLE)

SET(G3D_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS})

# add glg3d and g3d libraries to build
ADD_SUBDIRECTORY (GLG3D.lib)
ADD_SUBDIRECTORY (G3D.lib)

# tool and demo targets
ADD_SUBDIRECTORY (tools)
ADD_SUBDIRECTORY (demos)

# add test targets, but not to the default target
ADD_SUBDIRECTORY (test EXCLUDE_FROM_ALL)

# add data-files installation
INSTALL(DIRECTORY data-files/ DESTINATION data PATTERN "CVS" EXCLUDE)

#-----------------------------------------------------------------------------
#
# Configuration
# 
# Set up paths such that other programs can find this library
#
SET(G3D_DEP_INCLUDE_DIRS   ${OPENGL_INCLUDE_DIR}
                           ${JPEG_INCLUDE_DIR}
                           ${ZLIB_INCLUDE_DIR}
                           ${ZIP_INCLUDE_DIR})

SET(G3D_DATA_DIR ${G3D_INSTALL_DIR}/data/data-files)
SET(G3D_INCLUDE_DIRS ${G3D_INSTALL_DIR}/include)
SET(G3D_LIBRARY_DIRS ${G3D_INSTALL_DIR}/lib)
SET(G3D_LIBS ${ALL_EXE_LIBRARIES})
SET(G3D_LIBS_DEBUG ${ALL_EXE_LIBRARIES})
SET(G3D_INSTALL_DIR ${CMAKE_INSTALL_PREFIX})

# Make a configure file for external programs
CONFIGURE_FILE(
  "${PROJECT_SOURCE_DIR}/G3DConfig.cmake.in"
  "${PROJECT_SOURCE_DIR}/G3DConfig.cmake"
  @ONLY
)

