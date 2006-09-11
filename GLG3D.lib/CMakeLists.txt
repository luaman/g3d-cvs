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
AUX_SOURCE_DIRECTORY(source source_list)
FILE(GLOB header_list include/GLG3D/*.h)

# Specify code files for the library.
ADD_LIBRARY(GLG3D ${source_list} ${header_list})

# Required link libraries
TARGET_LINK_LIBRARIES(GLG3D G3D ${OPENGL_LIBRARIES} ${SDL_LIBRARY})

