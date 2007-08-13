# viewer executable
# tools/viewer
#
# Installation: gfxmeter -> bin/viewer/
# Extra Installation: data-files/* -> bin/viewer/


# create source and data file lists for easy use
FILE(GLOB source_list *.cpp)

# specify include paths during compilation
INCLUDE_DIRECTORIES(${ALL_EXE_INCLUDE_DIRS})

# specify viewer source
ADD_EXECUTABLE(viewer ${source_list})

IF (MINGW)
    # set mingw compiler flag to create non-console executables
    SET_TARGET_PROPERTIES(viewer PROPERTIES LINK_FLAGS -mwindows)
ENDIF (MINGW)

# specify viewer executable linking dependencies 
TARGET_LINK_LIBRARIES(viewer ${ALL_EXE_LIBRARIES})

# specify executable output directory
SET (EXECUTABLE_OUTPUT_PATH ${G3D_BINARY_DIR}/build/${PLATFORM_DIR}/bin/viewer)

# copy data-files to build directory

# copy viewer data-files to installation directory
INSTALL (DIRECTORY data-files/ DESTINATION bin/viewer PATTERN "CVS" EXCLUDE)

# copy viewer to installation directory
INSTALL (TARGETS viewer RUNTIME DESTINATION bin/viewer)
