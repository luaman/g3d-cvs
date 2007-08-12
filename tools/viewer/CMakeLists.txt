# viewer executable
# tools/viewer
#
# Installation: gfxmeter -> bin/viewer/
# Extra Installation: data-files/* -> bin/viewer/


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

# specify viewer source
ADD_EXECUTABLE(viewer ${source_list})

# specify viewer executable linking dependencies 
TARGET_LINK_LIBRARIES(viewer ${ALL_EXE_LIBRARIES})

# specify executable output directory
SET (EXECUTABLE_OUTPUT_PATH ${G3D_BINARY_DIR}/build/${PLATFORM_DIR}/bin/viewer)

# copy data-files to build directory
#COPY_IF_DIFFERENT("" ${G3D_BINARY_DIR}/build/${PLATFORM_DIR}/bin/viewer ${data_files} viewer)

# copy viewer data-files to installation directory
INSTALL (FILES ${data_files} DESTINATION ${G3D_INSTALL_DIR}/${PLATFORM_DIR}/bin/viewer)

# copy viewer to installation directory
INSTALL (TARGETS viewer RUNTIME DESTINATION ${G3D_INSTALL_DIR}/${PLATFORM_DIR}/bin/viewer)
