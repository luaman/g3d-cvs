# PNG library
# png.lib/
# 
# Installation: png -> lib/
# Extra Installation: include/*.h -> include/png/


# create source and include file lists for easy use
FILE(GLOB source_list source/*.c)
FILE(GLOB header_list include/*.h)

# specify include paths during compilation
INCLUDE_DIRECTORIES(include ${ZLIB_INCLUDE_DIR})

# specify png source
ADD_LIBRARY(png ${source_list})

# copy png headers to installation directory
INSTALL (FILES ${header_list} DESTINATION include)

# copy png installation directory
INSTALL (TARGETS png 
                 LIBRARY DESTINATION lib
                 ARCHIVE DESTINATION lib)