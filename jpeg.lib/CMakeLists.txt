# PNG library
# png.lib/
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

# copy jpeg headers to installation directory
INSTALL (FILES ${header_list} DESTINATION ${G3D_INSTALL_DIR}/include/jpeg)

# copy jpeg installation directory
INSTALL (TARGETS jpeg
                 LIBRARY DESTINATION ${G3D_INSTALL_DIR}/${PLATFORM_DIR}/lib
                 ARCHIVE DESTINATION ${G3D_INSTALL_DIR}/${PLATFORM_DIR}/lib)