# gfxmeter executable
# tools/gfxmeter
#
# Installation: gfxmeter -> bin/gfxmeter/
# Extra Installation: data-files/* -> bin/gfxmeter/


# create source and data file lists for easy use
FILE(GLOB source_list source/*.cpp)
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

# specify gfxmeter source
ADD_EXECUTABLE(gfxmeter ${source_list})

# specify gfxmeter executable linking dependencies 
TARGET_LINK_LIBRARIES(gfxmeter ${ALL_EXE_LIBRARIES})

# specify executable output directory
SET (EXECUTABLE_OUTPUT_PATH ${G3D_BINARY_DIR}/build/${PLATFORM_DIR}/bin/gfxmeter)

# copy data-files to build directory
#COPY_IF_DIFFERENT("" ${G3D_BINARY_DIR}/build/${PLATFORM_DIR}/bin/gfxmeter ${data_files} gfxmeter)

# copy gfxmeter data-files to installation directory
INSTALL (FILES ${data_files} DESTINATION ${G3D_INSTALL_DIR}/${PLATFORM_DIR}/bin/gfxmeter)

# copy gfxmeter to installation directory
INSTALL (TARGETS gfxmeter RUNTIME DESTINATION ${G3D_INSTALL_DIR}/${PLATFORM_DIR}/bin/gfxmeter)
