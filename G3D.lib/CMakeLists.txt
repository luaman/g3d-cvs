# G3D library
# G3D.lib/
# 
# Installation: G3D -> lib/
# Extra Installation: include/G3D/*.h -> include/G3D


# create source and include file lists for easy use
FILE(GLOB source_list source/*.cpp)
FILE(GLOB header_list include/G3D/*.h)

# specify include paths during compilation
INCLUDE_DIRECTORIES(${PNG_INCLUDE_DIR}
                    ${JPEG_INCLUDE_DIR}
                    ${ZLIB_INCLUDE_DIR}
                    ${ZIP_INCLUDE_DIR}
                    include)


# specify g3d source
ADD_LIBRARY(G3D ${source_list})

# copy g3d headers to installation directory
INSTALL (FILES ${header_list} DESTINATION include/G3D)

# copy g3d to installation directory
INSTALL (TARGETS G3D LIBRARY 
                 LIBRARY DESTINATION lib
                 ARCHIVE DESTINATION lib)
