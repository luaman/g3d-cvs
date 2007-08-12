# ZLIB library
# zlib.lib/
# 
# Installation: zlib -> lib/
# Extra Installation: include/*.h -> include/zlib/


# create source and include file lists for easy use
FILE(GLOB source_list source/*.c)
FILE(GLOB header_list include/*.h)

# specify include paths during compilation
INCLUDE_DIRECTORIES(include)

# specify zlib source
ADD_LIBRARY(zlib ${source_list})

# copy zlib headers to installation directory
INSTALL (FILES ${header_list} DESTINATION ${G3D_INSTALL_DIR}/include/zlib)

# copy zlib installation directory
INSTALL (TARGETS zlib 
                 LIBRARY DESTINATION ${G3D_INSTALL_DIR}/${PLATFORM_DIR}/lib
                 ARCHIVE DESTINATION ${G3D_INSTALL_DIR}/${PLATFORM_DIR}/lib)
