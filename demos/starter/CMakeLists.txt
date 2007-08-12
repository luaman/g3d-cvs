# starter executable
# demos/starter
#
# Installation: starter -> bin/starter/
# Extra Installation: data-files/* -> bin/starter/


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

# specify starter source
ADD_EXECUTABLE(starter ${source_list})

# specify starter executable linking dependencies 
TARGET_LINK_LIBRARIES(starter ${ALL_EXE_LIBRARIES})

# specify executable output directory
SET (EXECUTABLE_OUTPUT_PATH ${G3D_BINARY_DIR}/build/${PLATFORM_DIR}/bin/starter)

# copy data-files to build directory
#COPY_IF_DIFFERENT("" ${G3D_BINARY_DIR}/build/${PLATFORM_DIR}/bin/starter ${data_files} starter)

# copy starter data-files to installation directory
INSTALL (FILES ${data_files} DESTINATION ${G3D_INSTALL_DIR}/${PLATFORM_DIR}/bin/starter)

# copy starter to installation directory
INSTALL (TARGETS starter RUNTIME DESTINATION ${G3D_INSTALL_DIR}/${PLATFORM_DIR}/bin/starter)
