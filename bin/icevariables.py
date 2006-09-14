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

##################################################
# Verbosity levels
# Print only errors and prompts
QUIET                     = 10

NORMAL                    = 11

# Print complete commands
VERBOSE                   = 12

# Print additional debugging information
TRACE                     = 13

# Temp directory where scratch and .o files are stored, relative to rootDir
tempDir                   = '.ice-tmp/'

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

verbosity                 = NORMAL

# Options for this target.  Set by configureCompiler
defaultCompilerOptions    = []

# Options regarding warnings. Set by configureCompiler
compilerWarningOptions    = []

# Options regarding verbose. Set by configureCompiler
compilerVerboseOptions    = []

# Set by configureCompiler.
defaultLinkerOptions      = []

defaultDynamicLibs = []
defaultStaticLibs = []

class State:
    # e.g., linux-gcc4.1
    platform                 	= None

    # e.g., 'osx', 'freebsd', 'linux'
    os                          = None

    # List of all library canonical names that are known to be
    # used by this project.
    usesLibraries               = []

    # EXE, LIB, or DLL. Set by setVariables.
    binaryType                  = None

    # RELEASE or DEBUG
    target                      = None

    # Name of the project (without .lib/.dll extension)
    projectName                 = None

    # A dictionary used to store values between invocations of iCompile
    cache                       = {}

    tempDir                     = '.ice-tmp/'

    # Location to which all build files are written
    buildDir                    = 'build/'
     
    # Location to which generated binaries are written
    binaryDir                   = None

    # Binary name not including directory.  Set by setVariables.
    binaryName                  = None

    # Location of the output binary relative to rootDir.  Set by setVariables.
    binaryDir                   = None

    # EXE, LIB, or DLL. Set by setVariables.
    binaryType                  = None

    # Location to which object files are written (target-specific).
    # Set by setVariables
    objDir                      = None
  
    # If true, the user is never prompted
    noPrompt                    = False

state = State()