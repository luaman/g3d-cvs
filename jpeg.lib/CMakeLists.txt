# JPEG library
# jpeg.lib/
# 
# Installation: jpeg -> lib/
# Extra Installation: include/*.h -> include/jpeg/


# create source and include file lists for easy use
FILE(GLOB source_list source/*.c)
FILE(GLOB header_list include/*.h)

# specify include paths during compilation
INCLUDE_DIRECTORIES(include)

# specify jpeg source
ADD_LIBRARY(jpeg ${source_list})

# copy files to build directory
COPY_IF_DIFFERENT(${CMAKE_CURRENT_SOURCE_DIR}/include ${G3D_BINARY_DIR}/build/${PLATFORM_DIR}/include jpeg)

# copy jpeg headers to installation directory
INSTALL (FILES ${header_list} DESTINATION include)

# copy jpeg installation directory
INSTALL (TARGETS jpeg
                 LIBRARY DESTINATION lib
                 ARCHIVE DESTINATION lib)