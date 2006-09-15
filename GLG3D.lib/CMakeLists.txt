######################################################################
# DIRECTORY:	glg3d.lib
# PROGRAMMER:	Daniel Hilferty
# CREATED:	3 August 2006
# MODIFIED:	15 August 2006
#
# DESCRIPTION:
# 	Build File for GLG3D.  It specifies OpenGL as a dependency.
######################################################################
# Create list of source files used in glg3d -- yes, this is poor form,
# but as long as you recognize the need to re-run cmake after adding
# a file to the project, there will be no problems.
FILE(GLOB source_list source/*.cpp)
FILE(GLOB header_list include/GLG3D/*.h)

# Specify code files for the library.
ADD_LIBRARY(GLG3D ${source_list} ${header_list})

# Required link libraries
TARGET_LINK_LIBRARIES(GLG3D G3D ${OPENGL_LIBRARIES} ${SDL_LIBRARY})

FILE (GLOB gl_header_list include/GL/*.h)

INSTALL (FILES ${header_list} DESTINATION ${G3D_INSTALL_DIR}/include/GLG3D)
INSTALL (FILES ${gl_header_list} DESTINATION ${G3D_INSTALL_DIR}/include/GL)
INSTALL (TARGETS GLG3D 
                 LIBRARY DESTINATION ${G3D_INSTALL_DIR}/${PLATFORM_DIR}-lib
                 ARCHIVE DESTINATION ${G3D_INSTALL_DIR}/${PLATFORM_DIR}-lib)
