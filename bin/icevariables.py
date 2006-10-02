# variables.py
#
# Global variables for iCompile modules

import os

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

#############################################################################
# Verbosity levels
# Print only errors and prompts
QUIET                     = 10

NORMAL                    = 11

# Print complete commands
VERBOSE                   = 12

# Print additional debugging information
TRACE                     = 13

##############################################################################
#                                  Globals                                   #
##############################################################################

# Parameters used by iCompile.  All directories must be relative to the
# root directory to allow the source code to be moved around.
# Directories must end in '/' and must not be '' (use './' if
# necessary). 

verbosity                 = NORMAL

# Adds a new path or list of paths to an existing path list
def _pathAppend(plist, newPath):
    if isinstance(newPath, list):
        for p in newPath:
            _pathAppend(plist, p)
    else:
        if os.path.exists(newPath):
            plist.append(newPath)

# Use getConfigurationState to load
#
class State:
    # e.g., linux-gcc4.1
    platform                 	= None

    # The arguments that were supplied to iCompile preceeding --run
    args                        = None

    # e.g., 'osx', 'freebsd', 'linux'
    os                          = None

    # Temp directory where scratch and .o files are stored, relative to rootDir
    tempDir                     = None

    # Absolute location of the project root directory.  Ends in "/".
    rootDir                     = None

    # EXE, LIB, or DLL. Set by setVariables.
    binaryType                  = None

    # RELEASE or DEBUG
    target                      = None

    # Name of the project (without .lib/.dll extension)
    projectName                 = None

    # A dictionary used to store values between invocations of iCompile
    cache                       = {}

    # Filename of the compiler
    compiler                    = None

    # A list of options to be passed to the compiler.  Does not include
    # verbose or warnings options.
    compilerOptions             = []

    # Options regarding warnings
    compilerWarningOptions      = []

    # Options regarding verbose
    compilerVerboseOptions      = []

    # A list of options to be passed to the linker.  Does not include
    # options specified in the configuration file.
    linkerOptions               = []
 
    # List of all library canonical names that are known to be
    # used by this project.
    usesLibraries               = []

    # List of all projects that this one depends on, determined by the
    # uses: line of 
    usesProjectsList            = []
    usesLibrariesList           = []

    # Location to which all build files are written relative to 
    # rootDir
    buildDir                    = None
     
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
  
    # Compiled regular expression for files to ignore during compilation
    excludeFromCompilation      = None

    # All paths for #include; updated as libraries are detected
    _includePaths               = []
    _libraryPaths               = []

    # path is either a string or a list of paths
    # Paths are only added if they exist.
    def addIncludePath(self, path):
        _pathAppend(self._includePaths, path)

    # Returns a list of all include paths
    def includePaths(self):
        return self._includePaths

    def addLibraryPath(self, path):
        _pathAppend(self._libraryPaths, path)

    # Returns a list of all include paths
    def libraryPaths(self):
        return self._libraryPaths

state = None

###############################################

def isLibrary(L):
    return L == LIB or L == DLL
