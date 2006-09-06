######################################################################
# DIRECTORY:	zlib.lib
# PROGRAMMER:	Daniel Hilferty
# CREATED:	3 August 2006
# MODIFIED:	28 August 2006
#
# DESCRIPTION:
# 	Build File for zlib.lib.  For use with Window's builds.
######################################################################
# Create list of source files used in glg3d -- yes, this is poor form,
# but as long as you recognize the need to re-run cmake after adding
# a file to the project, there will be no problems.
AUX_SOURCE_DIRECTORY(source source_list)
FILE(GLOB header_list include/zlib/*.h)


# Specify code files for the library.
ADD_LIBRARY(zlib ${source_list} ${header_list})

