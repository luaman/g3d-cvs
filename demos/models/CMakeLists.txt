# models executable
# demos/models
#
# Installation: models -> bin/models/
# Extra Installation: data-files/* -> bin/models/


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

# specify models source
ADD_EXECUTABLE(models ${source_list})

# specify models executable linking dependencies 
TARGET_LINK_LIBRARIES(models ${ALL_EXE_LIBRARIES})

# specify executable output directory
SET (EXECUTABLE_OUTPUT_PATH ${G3D_BINARY_DIR}/build/${PLATFORM_DIR}/bin/models)

# copy data-files to build directory
#COPY_IF_DIFFERENT("" ${G3D_BINARY_DIR}/build/${PLATFORM_DIR}/bin/models ${data_files} models)

# copy models data-files to installation directory
INSTALL (FILES ${data_files} DESTINATION ${G3D_INSTALL_DIR}/${PLATFORM_DIR}/bin/models)

# copy models to installation directory
INSTALL (TARGETS models RUNTIME DESTINATION ${G3D_INSTALL_DIR}/${PLATFORM_DIR}/bin/models)
