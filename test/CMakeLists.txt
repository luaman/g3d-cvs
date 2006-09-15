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

IF (APPLE)
    SET (CARBON "-framework Carbon")
ENDIF(APPLE)

IF (UNIX)
    # Specify dependencies
    TARGET_LINK_LIBRARIES(test ncurses G3D GLG3D ${CARBON} ${OPENGL_LIBRARY} ${SDL_LIBRARY})
ELSE (UNIX)
    # Specify dependencies
    TARGET_LINK_LIBRARIES(test G3D GLG3D ${OPENGL_LIBRARY})
ENDIF (UNIX)

IF(APPLE)
  FIND_PROGRAM(VTK_APPLE_RESOURCE Rez /Developer/Tools)
  IF(VTK_APPLE_RESOURCE)
    ADD_CUSTOM_COMMAND(SOURCE test
                       COMMAND ${VTK_APPLE_RESOURCE}
                       ARGS Carbon.r -o 
                       ${G3D_BINARY_DIR}/bin/test
                       TARGET test)
  ENDIF(VTK_APPLE_RESOURCE)
ENDIF(APPLE)

INSTALL (TARGETS test RUNTIME DESTINATION ${G3D_INSTALL_DIR}/${PLATFORM_DIR}-bin)

