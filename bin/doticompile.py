# doticompile.py
# 
# Manage .icompile files

##############################################################################
#                             Default .icompile                              #
##############################################################################


configHelp = """
# If you have special needs, you can edit per-project ice.txt
# files and your global ~/.icompile file to customize the
# way your projects build.  However, the default values are
# probably sufficient and you don't *have* to edit these.
#
# To return to default settings, just delete ice.txt and
# ~/.icompile and iCompile will generate new ones when run.
#
#
# In general you can set values without any quotes, e.g.:
#
#  compileoptions = -O3 -g --verbose $(CXXFLAGS) %(defaultcompileoptions)s
#
# Adds the '-O3' '-g' and '--verbose' options to the defaults as
# well as the value of environment variable CXXFLAGS.
# 
# These files have the following sections and variables.
# Values in ice.txt override those specified in .icompile.
#
# GLOBAL Section
#  compiler           Path to compiler.
#  include            Semi-colon or colon (on Linux) separated
#                     include paths.
#
#  library            Same, for library paths.
#
#  defaultinclude     The initial include path.
#
#  defaultlibrary     The initial library path.
#
#  defaultcompiler    The initial compiler.
#
#  defaultexclude     Regular expression for directories to exclude
#                     when searching for C++ files.  Environment
#                     variables are NOT expanded for this expression.
#                     e.g. exclude: <EXCLUDE>|^win32$
# 
#  quiet              If True, always run in --quiet mode
#
#  beep               If True, beep after compilation
#
# DEBUG and RELEASE Sections
#  staticlibs         Semi-colon separated libraries to
#                     link against.  iCompile will automatically
#                     link against common libraries like
#                     OpenGL, SDL, G3D, zlib, and jpeg as needed.  
#                     e.g., staticlink = mylib.a; /u/uxf/lib/libpng.a
#
#                     You can also force linking using the '-l' linker
#                     option, however this method is more portable.
#
#  dynamiclibs        Same as above but for dynamic libraries.
#
#  defaultcompileoptions                     
#  compileoptions
#  defaultlinkoptions
#  linkoptions        Options *in addition* to the ones iCompile
#                     generates for the compiler and linker, separated
#                     by spaces as if they were on a command line.
#
#
# The following special values are available:
#
#   $(envvar)        Value of shell variable named envvar.
#                    Unset variables are the empty string.
#   $(shell ...)     Runs the '...' and replaces the expression
#                    as if it were the value of an envvar.
#   %(localvar)s     Value of a variable set inside ice.txt
#                    or .icompile (Yes, you need that 's'--
#                    it is a Python thing.)
#   <NEWESTGCC>      The newest version of gcc on your system.
#   <COMPILEOPTIONS> The default compiler option.s
#   <LINKOPTIONS>    The default linker options.
#   <DYNAMICLIBS>    Auto-detected dynamic libraries.
#   <STATICLIBS>     Auto-detected static libraries.
#   <EXCLUDE>        Default directories excluded from compilation.
#
# The special values may differ between the RELEASE and DEBUG
# targets.  The default .icompile sets the 'default' variables
# and the default ice.txt sets the real ones from those, so you
# can chain settings.
#
#  Colors have the form:
#
#    [bold|underline|reverse|italic|blink|fastblink|hidden|strikethrough]
#    [FG] [on BG]
#
#  where FG and BG are each one of
#   {default, black, red, green, brown, blue, purple, cyan, white}
#  Many styles (e.g. blink, italic) are not supported on most terminals.
#
#  Examples of legal colors: "bold", "bold red", "bold red on white", "green",
#  "bold on black"
#
"""

defaultDotICompile = """
# This is a configuration file for iCompile (http://ice.sf.net)
# """ + configHelp + """
[GLOBAL]
defaultinclude:  $(INCLUDE)
defaultlibrary:  $(LIBRARY);$(LD_LIBRARY_PATH);/usr/lib;/usr/X11R6/lib
defaultcompiler: <NEWESTGCC>
defaultexclude:  <EXCLUDE>
beep:            True
quiet:           False

[DEBUG]
defaultcompileoptions: <COMPILEOPTIONS>
defaultlinkoptions: <LINKOPTIONS>

[RELEASE]
defaultcompileoptions: <COMPILEOPTIONS>
defaultlinkoptions: <LINKOPTIONS>

"""

defaultProjectFileContents = """
# This project can be compiled by typing 'icompile'
# at the command line. Download the iCompile Python
# script from http://ice.sf.net
#
################################################################
""" + configHelp + """

################################################################
[GLOBAL]

compiler: %(defaultcompiler)s

include: %(defaultinclude)s

library: %(defaultlibrary)s

exclude: %(defaultexclude)s

# Colon-separated list of libraries on which this project depends.  If
# a library is specified (e.g., png.lib) the platform-appropriate 
# variation of that name is added to the libraries to link against.
# If a directory containing an iCompile ice.txt file is specified, 
# that project will be built first and then added to the include 
# and library paths and linked against.
uses:

################################################################
[DEBUG]

# Reserved for future use; ignored in this version of iCompile
staticlibs: <STATICLIBS>

# Reserved for future use; ignored in this version of iCompile
dynamiclibs: <DYNAMICLIBS>

compileoptions: %(defaultcompileoptions)s

linkoptions: %(defaultlinkoptions)s

################################################################
[RELEASE]

# Reserved for future use; ignored in this version of iCompile
staticlibs: <STATICLIBS>

# Reserved for future use; ignored in this version of iCompile
dynamiclibs: <DYNAMICLIBS>

compileoptions: %(defaultcompileoptions)s

linkoptions: %(defaultlinkoptions)s

"""

