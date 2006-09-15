######################################################################
# DIRECTORY:	png.lib
# PROGRAMMER:	Daniel Hilferty
# CREATED:	3 August 2006
# MODIFIED:	28 August 2006
#
# DESCRIPTION:
# 	Build File for png.lib.  For use with Window's builds.
######################################################################
# Create list of source files used in glg3d -- yes, this is poor form,
# but as long as you recognize the need to re-run cmake after adding
# a file to the project, there will be no problems.
AUX_SOURCE_DIRECTORY(source source_list)
FILE(GLOB header_list include/*.h)

# Specify code files for the library.
ADD_LIBRARY(png ${source_list} ${header_list})
TARGET_LINK_LIBRARIES(png ${ZLIB_LIBRARIES})

INSTALL (FILES ${header_list} DESTINATION ${G3D_INSTALL_DIR}/include/png)
INSTALL (TARGETS png 
                 LIBRARY DESTINATION ${G3D_INSTALL_DIR}/${PLATFORM_DIR}-lib
                 ARCHIVE DESTINATION ${G3D_INSTALL_DIR}/${PLATFORM_DIR}-lib)