# variables.py
#
# Global variables for iCompile modules

import os
from iceutils import *
from platform import machine


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


# Adds a new path or list of paths to an existing path list
# and then returns the mutated plist.  If plist was None
# when the function was invoked, a new list is returned.
def _pathAppend(plist, newPath):
    if plist == None:
        plist = []

    if isinstance(newPath, list):
        for p in newPath:
            _pathAppend(plist, p)
    else:
        if os.path.exists(newPath):
            plist.append(newPath)
   
    return plist

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
    cache                       = None

    # Filename of the compiler
    compiler                    = None

    # A list of options to be passed to the compiler.  Does not include
    # verbose or warnings options.
    compilerOptions             = None

    # Options regarding warnings
    compilerWarningOptions      = None

    # Options regarding verbose
    compilerVerboseOptions      = None

    # A list of options to be passed to the linker.  Does not include
    # options specified in the configuration file.
    linkerOptions               = None
 
    # List of all library canonical names that are known to be
    # used by this project.
    usesLibraries               = None

    # List of all projects that this one depends on, determined by the
    # uses: line of 
    usesProjectsList            = None
    usesLibrariesList           = None

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
    _includePaths               = None
    _libraryPaths               = None


    def __init__(self):
        self.usesProjectsList = []
        self.usesLibrariesList = []
        self.compilerVerboseOptions = []

    # path is either a string or a list of paths
    # Paths are only added if they exist.
    def addIncludePath(self, path):
        self._includePaths = _pathAppend(self._includePaths, path)

    # Returns a list of all include paths
    def includePaths(self):
        return self._includePaths

    def addLibraryPath(self, path):
        self._libraryPaths = _pathAppend(self._libraryPaths, path)

    # Returns a list of all include paths
    def libraryPaths(self):
        return self._libraryPaths

    def __str__(self):
        return ('State:' +
                '\n rootDir                = ' + str(self.rootDir) +
                '\n binaryName             = ' + str(self.binaryName) +
                '\n usesProjectsList       = ' + str(self.usesProjectsList) + 
                '\n compilerVerboseOptions = ' + str(self.compilerVerboseOptions))

###############################################

def isLibrary(L):
    return L == LIB or L == DLL

##################################################

"""allFiles is a list of all files on which something
   depends for the project.

   Returns a list of strings

   extraOpts are options that are needed for compilation
   but not dependency checking:
     state.compilerWarningOptions + state.compilerVerboseOptions
   """
def getCompilerOptions(state, allFiles, extraOpts = []):
    opt = state.compilerOptions + extraOpts + ['-c']
    
    for i in state.includePaths():
        opt += ['-I' + i]

    # See if the xmm intrinsics are being used
    # This was disabled because -msse2 allows code generation,
    # not just explicit use of intrinsics
    #for f in allFiles:
    #    if f[-11:] == 'xmmintrin.h':
    #        opt += ['-msse2']
    #        break
        
    return opt

###################################################################

##############################################################################
#                            Discover Platform                               #
##############################################################################

def discoverPlatform(state):

    state.os = ''
    compiler = ''

    # Figure out the os
    if (os.uname()[0] == 'Linux'):
        state.os = 'linux'
    elif (os.uname()[0] == 'FreeBSD'):
        state.os = 'freebsd'
    elif (os.uname()[0] == 'Darwin'):
        state.os = 'osx'
    else:
        raise 'Error', 'iCompile only supports Linux and OS X'

    # Figure out the compiler
    c = state.compiler
    compiler = betterbasename(c)

    if compiler.startswith('g++') or compiler.startswith('gcc'):
        compiler = 'g++'

    # Append the major and minor compiler version number to the compiler name
    v = getVersion(c)    
    if len(v) > 2:
        v = v[0:2]
    if len(v) < 2:
        # Version number was short; add a 0 minor number
        v = v + 0
    compiler += string.join(map(str, v), '.')

    state.platform = state.os + '-' + machine() + '-' + compiler
