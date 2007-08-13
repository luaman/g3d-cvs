# ZIP library
# zip.lib/
# 
# Installation: zip -> lib/
# Extra Installation: include/zip/*.h -> include/zip/


# create source and include file lists for easy use
FILE(GLOB source_list source/*.c)
FILE(GLOB header_list include/zip/*.h)

# specify include paths during compilation
INCLUDE_DIRECTORIES(include/zip ${ZLIB_INCLUDE_DIR})

# specify zip source
ADD_LIBRARY(zip ${source_list})

# copy zip headers to installation directory
INSTALL (FILES ${header_list} DESTINATION include/zip)

# copy zip to installation directory
INSTALL (TARGETS zip 
                 LIBRARY DESTINATION lib
                 ARCHIVE DESTINATION lib)
