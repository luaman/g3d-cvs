######################################################################
# DIRECTORY:	demos/models
# PROGRAMMER:	??
# CREATED:	??
# MODIFIED:	??
#
# DESCRIPTION:
#	Builds model.
######################################################################
# Create glob of files used in empty
AUX_SOURCE_DIRECTORY(. source_list)

# Specify files that make up empty
ADD_EXECUTABLE(modeldemo ${source_list})

IF (APPLE)
    SET (CARBON "-framework Carbon")
ENDIF(APPLE)

IF (UNIX)
    # Specify dependencies
    TARGET_LINK_LIBRARIES(modeldemo G3D GLG3D zip ${ZLIB} ${CARBON} ${OPENGL_LIBRARY} ${SDL_LIBRARY})
ELSE (UNIX)
    # Specify dependencies
    TARGET_LINK_LIBRARIES(modeldemo G3D GLG3D zip ${ZLIB} ${OPENGL_LIBRARY})
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
INSTALL (TARGETS modeldemo RUNTIME DESTINATION ${G3D_INSTALL_DIR}/${PLATFORM_DIR}-bin)
