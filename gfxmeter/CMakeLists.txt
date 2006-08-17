######################################################################
# DIRECTORY:	gfxmeter
# PROGRAMMER:	Daniel Hilferty
# CREATED:	3 August 2006
# MODIFIED:	15 August 2006
#
# DESCRIPTION:
#	Builds gfxmeter.
######################################################################
# Create glob of files used in gfxmeter
AUX_SOURCE_DIRECTORY(source source_list)

# Specify files that make up gfxmeter
ADD_EXECUTABLE(gfxmeter ${source_list})

IF (UNIX)
    # Specify dependencies
    TARGET_LINK_LIBRARIES(gfxmeter G3D GLG3D ${OPENGL_LIBRARY} ${SDL_LIBRARY})
ELSE (UNIX)
    # Specify dependencies
    TARGET_LINK_LIBRARIES(gfxmeter G3D GLG3D ${OPENGL_LIBRARY})
ENDIF (UNIX)

FILE (GLOB DATA_FILES data-files/*)
INSTALL_FILES (${G3D_INSTALL_DIR}/debug DATA_FILES)
