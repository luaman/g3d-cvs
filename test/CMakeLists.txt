# test executable
# test/
#
# Installation: test -> bin/test/
# Extra Installation: data-files -> bin/test/


# create source and data file lists for easy use
FILE(GLOB source_list *.cpp)

# specify include paths during compilation
INCLUDE_DIRECTORIES(${ALL_EXE_INCLUDE_DIRS})

# specify test source
ADD_EXECUTABLE(test ${source_list})

# specify test executable linking dependencies 
TARGET_LINK_LIBRARIES(test ${ALL_EXE_LIBRARIES})

IF (APPLE)
  FIND_PROGRAM(VTK_APPLE_RESOURCE Rez /Developer/Tools)
  IF(VTK_APPLE_RESOURCE)
    ADD_CUSTOM_COMMAND(SOURCE test
                       COMMAND ${VTK_APPLE_RESOURCE}
                       ARGS Carbon.r -o 
                       ${G3D_BINARY_DIR}/bin/test
                       TARGET test)
  ENDIF(VTK_APPLE_RESOURCE)
ENDIF(APPLE)

# specify executable output directory
SET (EXECUTABLE_OUTPUT_PATH ${G3D_BINARY_DIR}/build/${PLATFORM_DIR}/bin/test)

# copy data-files to build directory

# copy test data-files to installation directory
INSTALL (DIRECTORY data-files/ DESTINATION bin/test PATTERN "CVS" EXCLUDE)

# copy test to installation directory
INSTALL (TARGETS test RUNTIME DESTINATION bin/test)

