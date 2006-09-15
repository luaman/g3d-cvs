######################################################################
# DIRECTORY:	gfxmeter
# PROGRAMMER:	Daniel Hilferty
# CREATED:	3 August 2006
# MODIFIED:	15 September 2006
#
# DESCRIPTION:
#	Builds gfxmeter.
######################################################################
# Create glob of files used in gfxmeter
AUX_SOURCE_DIRECTORY(source source_list)

# Specify files that make up gfxmeter
ADD_EXECUTABLE(gfxmeter ${source_list})

IF (APPLE)
    SET (CARBON "-framework Carbon"
ENDIF(APPLE)

IF (UNIX)
    # Specify dependencies
    TARGET_LINK_LIBRARIES(gfxmeter G3D GLG3D ${CARBON} ${OPENGL_LIBRARY} ${SDL_LIBRARY})
ELSE (UNIX)
    # Specify dependencies
    TARGET_LINK_LIBRARIES(gfxmeter G3D GLG3D ${OPENGL_LIBRARY})
ENDIF (UNIX)

FILE (GLOB DATA_FILES 
           data-files/*.fnt 
           data-files/*.ico 
           data-files/*.ifs 
           data-files/*.md2 
           data-files/*.jpg 
           data-files/*.png)

# Specify installation locations
INSTALL (FILES ${DATA_FILES} DESTINATION ${G3D_INSTALL_DIR}/${PLATFORM_DIR}-bin/data-files)
INSTALL (TARGETS gfxmeter RUNTIME DESTINATION ${G3D_INSTALL_DIR}/${PLATFORM_DIR}-bin)
