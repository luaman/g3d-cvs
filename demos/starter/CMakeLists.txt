# starter executable
# demos/starter
#
# Installation: starter -> bin/starter/
# Extra Installation: none


# create source and data file lists for easy use
FILE(GLOB source_list *.cpp)

# specify include paths during compilation
INCLUDE_DIRECTORIES(${ALL_EXE_INCLUDE_DIRS})

# specify starter source
ADD_EXECUTABLE(starter ${source_list})

IF (MINGW)
    # set mingw compiler flag to create non-console executables
    SET_TARGET_PROPERTIES(starter PROPERTIES LINK_FLAGS -mwindows)
ENDIF (MINGW)

# specify starter executable linking dependencies 
TARGET_LINK_LIBRARIES(starter ${ALL_EXE_LIBRARIES})

# specify executable output directory
SET (EXECUTABLE_OUTPUT_PATH ${G3D_BINARY_DIR}/build/${PLATFORM_DIR}/bin/starter)

# copy data-files to build directory

# copy starter to installation directory
INSTALL (TARGETS starter RUNTIME DESTINATION bin/starter)
