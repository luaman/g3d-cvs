# test executable
# test/
#
# Installation: test -> bin/test/
# Extra Installation: data-files -> bin/test/


# create source and data file lists for easy use
FILE(GLOB source_list *.cpp)
FILE(GLOB   data_files 
            data-files/*.fnt 
            data-files/*.ico 
            data-files/*.ifs 
            data-files/*.md2 
            data-files/*.jpg 
            data-files/*.png
            data-files/*.zip
            data-files/*.txt)

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
#COPY_IF_DIFFERENT("" ${G3D_BINARY_DIR}/build/${PLATFORM_DIR}/bin/test ${data_files} test)

# copy test data-files to installation directory
INSTALL (FILES ${data_files} DESTINATION ${G3D_INSTALL_DIR}/${PLATFORM_DIR}/bin/test)

# copy test to installation directory
INSTALL (TARGETS test RUNTIME DESTINATION ${G3D_INSTALL_DIR}/${PLATFORM_DIR}/bin/test)

