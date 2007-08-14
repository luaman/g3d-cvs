# gfxmeter executable
# tools/gfxmeter
#
# Installation: gfxmeter -> bin/gfxmeter/
# Extra Installation: data-files/* -> bin/gfxmeter/


# create source and data file lists for easy use
FILE(GLOB source_list source/*.cpp)

# specify include paths during compilation
INCLUDE_DIRECTORIES(${ALL_EXE_INCLUDE_DIRS})

# specify gfxmeter source
ADD_EXECUTABLE(gfxmeter ${source_list})

# specify gfxmeter executable linking dependencies 
TARGET_LINK_LIBRARIES(gfxmeter ${ALL_EXE_LIBRARIES})

# specify executable output directory
SET (EXECUTABLE_OUTPUT_PATH ${G3D_BINARY_DIR}/build/${PLATFORM_DIR}/bin/gfxmeter)

# copy files to build directory
COPY_IF_DIFFERENT(${CMAKE_CURRENT_SOURCE_DIR}/data-files ${G3D_BINARY_DIR}/build/${PLATFORM_DIR}/bin/gfxmeter gfxmeter)

# copy gfxmeter data-files to installation directory
INSTALL (DIRECTORY data-files/ DESTINATION bin/gfxmeter PATTERN "CVS" EXCLUDE)

# copy gfxmeter to installation directory
INSTALL (TARGETS gfxmeter RUNTIME DESTINATION bin/gfxmeter)
