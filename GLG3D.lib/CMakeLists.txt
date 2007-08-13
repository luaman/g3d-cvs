# GLG3D library
# GLG3D.lib/
# 
# Installation: GLG3D -> lib/
# Extra Installation: include/GLG3D/*.h -> include/GLG3D
#                     include/GLG3D/GL/*.h -> include/GLG3D/GL

IF (APPLE)
    SET (MM_FILES source/*.mm)
ENDIF(APPLE)

# create source and include file lists for easy use
FILE(GLOB source_list source/*.cpp ${MM_FILES})
FILE(GLOB header_list include/GLG3D/*.h)
FILE(GLOB gl_header_list include/GL/*.h)

# specify include paths during compilation
INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIR}
                    ${X11_INCLUDE_DIR} 
                    ${PNG_INCLUDE_DIR}
                    ${JPEG_INCLUDE_DIR}
                    ${ZLIB_INCLUDE_DIR}
                    ${ZIP_INCLUDE_DIR}
                    ${G3D_SOURCE_DIR}/G3D.lib/include
                    include )

IF (UNIX OR APPLE)
    INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
ENDIF (UNIX OR APPLE)

# specify glg3d source
ADD_LIBRARY(GLG3D ${source_list})

# copy g3d headers to installation directory
INSTALL (FILES ${header_list} DESTINATION include/GLG3D)
INSTALL (FILES ${gl_header_list} DESTINATION include/GLG3D/GL)

# copy glg3d to installation directory
INSTALL (TARGETS GLG3D 
                 LIBRARY DESTINATION lib
                 ARCHIVE DESTINATION lib)
