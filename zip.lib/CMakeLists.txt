######################################################################
# DIRECTORY:	zip.lib
# PROGRAMMER:	??
# CREATED:	??
# MODIFIED:	??
#
# DESCRIPTION:
# 	Build File for zip.lib
######################################################################
# Create list of source files used in glg3d -- yes, this is poor form,
# but as long as you recognize the need to re-run cmake after adding
# a file to the project, there will be no problems.
AUX_SOURCE_DIRECTORY(source source_list)
FILE(GLOB header_list include/zip/*.h)

# Specify code files for the library.
ADD_LIBRARY(zip ${source_list} ${header_list})
INCLUDE_DIRECTORIES(${ZIP_INCLUDE_DIR}/zip)

INSTALL (FILES ${header_list} DESTINATION ${G3D_INSTALL_DIR}/include/zip)
INSTALL (TARGETS zip 
                 LIBRARY DESTINATION ${G3D_INSTALL_DIR}/${PLATFORM_DIR}-lib
                 ARCHIVE DESTINATION ${G3D_INSTALL_DIR}/${PLATFORM_DIR}-lib)
