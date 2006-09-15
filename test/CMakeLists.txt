######################################################################
# DIRECTORY:	test
# PROGRAMMER:	Daniel Hilferty
# CREATED:	3 August 2006
# MODIFIED:	15 September 2006
#
# DESCRIPTION:
#	Builds test program
######################################################################
# Create glob of files used in gfxmeter
AUX_SOURCE_DIRECTORY(./ source_list)

# Specify files that make up gfxmeter
ADD_EXECUTABLE(test ${source_list})

IF (UNIX)
    # Specify dependencies
    TARGET_LINK_LIBRARIES(test ncurses G3D GLG3D ${OPENGL_LIBRARY} ${SDL_LIBRARY})
ELSE (UNIX)
    # Specify dependencies
    TARGET_LINK_LIBRARIES(test G3D GLG3D ${OPENGL_LIBRARY})
ENDIF (UNIX)

INSTALL (TARGETS test RUNTIME DESTINATION ${G3D_INSTALL_DIR}/${PLATFORM_DIR}-bin)

