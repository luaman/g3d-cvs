######################################################################
# DIRECTORY:	g3d.lib
# PROGRAMMER:	Daniel Hilferty
# CREATED:	3 August 2006
# MODIFIED:	15 August 2006
#
# DESCRIPTION:
# 	Build File for G3D.  PNG and JPEG are listed as dependencies.  
######################################################################
# Create list of source files used in G3D -- yes, this is poor form,
# but as long as you recognize the need to re-run cmake after adding
# a file to the project, there will be no problems.
FILE(GLOB source_list source/*.cpp)
FILE(GLOB header_list include/G3D/*.h)

# Specify code files for the library.
ADD_LIBRARY(G3D ${source_list} ${header_list})

# Specify libraries to link
TARGET_LINK_LIBRARIES(G3D ${PNG_LIBRARIES} ${JPEG_LIBRARIES} ${ZLIB_LIBRARIES})

INSTALL (FILES ${header_list} DESTINATION ${G3D_INSTALL_DIR}/include/G3D)
INSTALL (TARGETS G3D LIBRARY 
                 LIBRARY DESTINATION ${G3D_INSTALL_DIR}/${PLATFORM_DIR}-lib
                 ARCHIVE DESTINATION ${G3D_INSTALL_DIR}/${PLATFORM_DIR}-lib)