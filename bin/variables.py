# variables.py
#
# Global variables for iCompile modules

##############################################################################
#                                 Constants                                  #
##############################################################################

DEBUG                     = 'DEBUG'
RELEASE                   = 'RELEASE'
EXE                       = 'EXE'
DLL                       = 'DLL'
LIB                       = 'LIB'
YES                       = True
NO                        = False

BUILDDIR                  = 'build'
DISTRIBDIR                = 'install'


# Temp directory where scratch and .o files are stored, relative to rootDir
tempDir                   = '.icompile-temp/'

##############################################################################
#                                  Globals                                   #
##############################################################################

# Parameters used by iCompile.  All directories must be relative to the
# root directory to allow the source code to be moved around.
# Directories must end in '/' and must not be '' (use './' if
# necessary). 

# List of all projects that this one depends on, determined by the
# uses: line of 
usesProjectsList          = []
usesLibrariesList         = []

# Location of the project root relative to CWD.  Ends in "/".  Set by setVariables.
rootDir                   = ''

# DEBUG or RELEASE
target                    = ''

# Location to which object files are written (target-specific).
# Set by setVariables
objDir                    = ''

# EXE, LIB, or DLL. Set by setVariables.
binaryType                = ''

# Name of the project with no extension.  Set by setVariables.
projectName               = ''

# If YES, iCompile prints diagnostic information (helpful for debugging
# iCompile).  Set by setVariables.
verbose                   = NO

# Should debug symbols be stripped from debug build?
stripDebugSymbols         = NO

# A dictionary used to store values between invocations of iCompile
cache                     = {}

# Binary name.  Set by setVariables.
binaryName                = ''

# Location of the output binary relative to rootDir.  Set by setVariables.
binaryDir                 = ''

# Options for this target.  Set by configureCompiler
defaultCompilerOptions    = []

# Options regarding warnings. Set by configureCompiler
compilerWarningOptions    = []

# Options regarding verbose. Set by configureCompiler
compilerVerboseOptions    = []

# Set by configureCompiler.
defaultLinkerOptions      = []

# Set by configureCompiler
defaultCompilerName       = ''

# Supresses all non-error output. Set by setVariables.
quiet                     = NO

# e.g. linux-gcc4.0
platform                  = ''

defaultDynamicLibs = []
defaultStaticLibs = []

# Should the built executable be run under GDB?
doGDB                     = False

# Should the built executable be run
doRun                     = False
