# models executable
# demos/models
#
# Installation: models -> bin/models/
# Extra Installation: data-files/* -> bin/models/


# create source and data file lists for easy use
FILE(GLOB source_list *.cpp)

# specify include paths during compilation
INCLUDE_DIRECTORIES(${ALL_EXE_INCLUDE_DIRS})

# specify models source
ADD_EXECUTABLE(models ${source_list})

IF (MINGW)
    # set mingw compiler flag to create non-console executables
    SET_TARGET_PROPERTIES(models PROPERTIES LINK_FLAGS -mwindows)
ENDIF (MINGW)

# specify models executable linking dependencies 
TARGET_LINK_LIBRARIES(models ${ALL_EXE_LIBRARIES})

# specify executable output directory
SET (EXECUTABLE_OUTPUT_PATH ${G3D_BINARY_DIR}/bin/models)

# copy files to build directory
COPY_IF_DIFFERENT(${CMAKE_CURRENT_SOURCE_DIR}/data-files ${G3D_BINARY_DIR}/bin/models models)

# copy models data-files to installation directory
INSTALL (DIRECTORY data-files/ DESTINATION bin/models 
  PATTERN "CVS" EXCLUDE
  )

# copy models to installation directory
INSTALL (TARGETS models RUNTIME DESTINATION bin/models)
