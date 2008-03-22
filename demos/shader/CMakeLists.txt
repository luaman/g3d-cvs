# shader executable
# demos/shader
#
# Installation: shader -> bin/shader/
# Extra Installation: data-files/* -> bin/shader/


# create source and data file lists for easy use
FILE(GLOB source_list source/*.cpp)

# specify include paths during compilation
INCLUDE_DIRECTORIES(${ALL_EXE_INCLUDE_DIRS})

# specify shader source
ADD_EXECUTABLE(shader ${source_list})

IF (MINGW)
    # set mingw compiler flag to create non-console executables
    SET_TARGET_PROPERTIES(shader PROPERTIES LINK_FLAGS -mwindows)
ENDIF (MINGW)

# specify shader executable linking dependencies 
TARGET_LINK_LIBRARIES(shader ${ALL_EXE_LIBRARIES})

# specify executable output directory
SET (EXECUTABLE_OUTPUT_PATH ${G3D_BINARY_DIR}/bin/shader)

# copy files to build directory
COPY_IF_DIFFERENT(${CMAKE_CURRENT_SOURCE_DIR}/data-files ${G3D_BINARY_DIR}/bin/shader shader)

# copy shader data-files to installation directory
INSTALL (DIRECTORY data-files/ DESTINATION bin/shader PATTERN "CVS" EXCLUDE)

# copy shader to installation directory
INSTALL (TARGETS shader RUNTIME DESTINATION bin/shader)
