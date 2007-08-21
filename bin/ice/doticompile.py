# doticompile.py
# 
# Manage .icompile files

import ConfigParser, string, os, copyifnewer, templateG3D, templateHello
from utils import *
from doxygen import *
from variables import *
from help import *


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
#  builddir           Build directory, relative to ice.txt
#
#  tempdir            Temp directory, relative to ice.txt
#
#  beep               If True, beep after compilation
#
# DEBUG and RELEASE Sections
#
#  compileoptions                     
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
#   <NEWESTCOMPILER> The newest version of gcc or Visual Studio on your system.
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
defaultinclude:  $(INCLUDE);/usr/include;/usr/local/include;/usr/local/include/SDL11;/usr/include/SDL;/usr/X11R6/include;
defaultlibrary:  $(LIBRARY);$(LD_LIBRARY_PATH);/usr/lib;/usr/X11R6/lib;/usr/local/lib
defaultcompiler: <NEWESTCOMPILER>
defaultexclude:  <EXCLUDE>
beep:            True
tempdir:         .ice-tmp
builddir:        build

[DEBUG]

[RELEASE]

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

compileoptions:

linkoptions:

################################################################
[RELEASE]

compileoptions:

linkoptions:

"""




#################################################################
#                 Configuration & Project File                  #
#################################################################

""" Reads [section]name from the provided configuration, replaces
    <> and $() values with the appropriate settings.

    If exp is False $() variables are *not* expanded. 

    If
"""    
def configGet(state, config, section, name, exp = True):
    try:
        val = config.get(section, name)
    except ConfigParser.InterpolationMissingOptionError:
	maybeWarn('Variable \'' + name + '\' in ' + ' the [' + section + '] section of ' + 
                  state.rootDir + 'ice.txt may have an illegal value.  If that ice.txt ' +
                  'file is from a previous version of iCompile you should delete it.\n', state)
        return ''

    # Replace special values
    if '<' in val:
        if '<NEWESTGCC>' in val:
            (gppname, ver) = newestCompiler()
            val = val.replace('<NEWESTGCC>', gppname)

        if '<NEWESTCOMPILER>' in val:
            (compname, ver) = newestCompiler()
            val = val.replace('<NEWESTCOMPILER>', compname)

        val = val.replace('<EXCLUDE>', string.join(copyifnewer._cppExcludePatterns + ['^CMakeFiles$'], '|'))

    val = os.path.expanduser(val)
        
    if exp:
        val = expandvars(val)

    return val


class FakeFile:
    _textLines = []
    _currentLine = 0

    def __init__(self, contents):
        self._currentLine = 0
        self._textLines = string.split(contents, '\n')

    def readline(self):
        if (self._currentLine >= len(self._textLines)):
            # end of file
            return ''
        else:
            self._currentLine += 1
            return self._textLines[self._currentLine - 1] + '\n'
           

""" Called from processProjectFile """ 
def _processDotICompile(state, config):

    # Set the defaults from the default .icompile and ice.txt
    config.readfp(FakeFile(defaultDotICompile))
    config.readfp(FakeFile(defaultProjectFileContents))

    # Process .icompile
    if os.path.exists(state.preferenceFile()):
        if verbosity >= TRACE: print 'Processing ' + state.preferenceFile()
        config.read(state.preferenceFile())
    else:
        success = False

        HOME = os.environ['HOME']
        preferenceFile = HOME + '/.icompile'
        # Try to generate a default .icompile
        if os.path.exists(HOME):
            f = file(preferenceFile, 'wt')
            if f != None:
                f.write(defaultDotICompile)
                f.close()
                success = True
                if verbosity >= TRACE:
                    colorPrint('Created a default preference file for ' +
                                    'you in ' + preferenceFile + '\n',
                                    SECTION_COLOR)
                
        # We don't need to read this new .icompile because
        # it matches the default, which we already read.
                           
        if not success and verbosity >= TRACE:
            print ('No ' + preferenceFile + ' found and cannot write to '+ HOME)

""" Process the project file and .icompile so that we can use configGet.
    Sets a handful of variables."""
def processProjectFile(state):

    config = ConfigParser.SafeConfigParser()
    _processDotICompile(state, config)

    # Process the project file
    projectFile = 'ice.txt'
    if verbosity >= TRACE: print 'Processing ' + projectFile
    config.read(projectFile)

    # Don't expand '$' envvar in regular expressions since
    # $ means end of pattern.
    exclude = configGet(state, config, 'GLOBAL', 'exclude', False)
    state.excludeFromCompilation = re.compile(exclude)
 
    # Parses the "uses" line, if it exists
    L = ''
    try:
        L = configGet(state, config, 'GLOBAL', 'uses')
    except ConfigParser.NoOptionError:
        # Old files have no 'uses' section
        pass

    for u in string.split(L, ':'):
        if u.strip() != '':
            if os.path.exists(pathConcat(u, 'ice.txt')):
                # This is another iCompile project
                state.usesProjectsList.append(u)
            else:
                state.usesLibrariesList.append(u)

    state.buildDir = addTrailingSlash(configGet(state, config, 'GLOBAL', 'builddir', True))
    
    state.tempDir = addTrailingSlash(pathConcat(configGet(state, config, 'GLOBAL', 'tempdir', True), state.projectName))

    state.beep = configGet(state, config, 'GLOBAL', 'beep')
    state.beep = (state.beep == True) or (state.beep.lower() == 'true')

    # Include Paths
    state.addIncludePath(makePathList(configGet(state, config, 'GLOBAL', 'include')))

    # Add our own include directories
    if isLibrary(state.binaryType):
        state.addIncludePath(['include', 'include/' + state.projectName])

    # Library Paths
    state.addLibraryPath(makePathList(configGet(state, config, 'GLOBAL', 'library')))

    state.compiler = configGet(state, config, 'GLOBAL', 'compiler')

    state.compilerOptions = string.split(configGet(state, config, state.target, 'compileoptions'), ' ')
    state.linkerOptions   = string.split(configGet(state, config, state.target, 'linkoptions'), ' ')


#########################################################################

    
# Loads configuration from the current directory, where args
# are the arguments preceding --run that were passed to iCompile
#
# Returns the configuration state
def getConfigurationState(args):
    
    state = State()

    state.args = args

    state.noPrompt = '--noprompt' in args

    state.template = ''
    if ('--template' in args):
        for i in xrange(0, len(args)):
            if args[i] == '--template':
                if i < len(args) - 1:
                    state.template = args[i + 1]

    if state.template != '' and not state.noPrompt:
        colorPrint("ERROR: cannot specify --template without --noprompt", ERROR_COLOR)
        sys.exit(-208)
        
    if state.template != 'hello' and state.template != 'G3D' and state.template != 'empty' and state.template != '':
        colorPrint("ERROR: 'hello', 'G3D', and 'empty' are the only legal template names (template='" +
                   state.template + "')", ERROR_COLOR)
        sys.exit(-209)

    # Root directory
    state.rootDir  = os.getcwd() + "/"

    # Project name
    state.projectName = string.split(state.rootDir, ('/'))[-2]

    ext = string.lower(extname(state.projectName))
    state.projectName = rawfilename(state.projectName)

    # Binary type    
    if (ext == 'lib') or (ext == 'a'):
        state.binaryType = LIB
    elif (ext == 'dll') or (ext == 'so'):
        state.binaryType = DLL
    elif (ext == 'exe') or (ext == ''):
        state.binaryType = EXE
    else:
        state.binaryType = EXE
        maybeWarn("This project has unknown extension '" + ext +
                  "' and will be compiled as an executable.", state)

    # Choose target
    if ('--opt' in args) or ('-O' in args) or (('--deploy' in args) and not ('--debug' in args)):
        if ('--debug' in args):
            colorPrint("Cannot specify '--debug' and '--opt' at " +
                       "the same time.", WARNING_COLOR)
            sys.exit(-1)

        state.target          = RELEASE
        d                     = ''
    else:
        state.target          = DEBUG
        d                     = 'd'


    # Find an icompile project file.  If there isn't one, give the
    # user the opportunity to create one or abort.
    checkForProjectFile(state, args)

    # Load settings from the project file.
    processProjectFile(state)

    discoverPlatform(state)

    unix = not state.os.startswith('win')

    # On unix-like systems we prefix library names with 'lib'
    prefix = ''
    if unix and isLibrary(state.binaryType):
        prefix = 'lib'

    state.installDir = state.buildDir + state.platform + '/'

    # Binary name
    if (state.binaryType == EXE):
        state.binaryDir  = state.installDir
        state.binaryName = state.projectName + d
    elif (state.binaryType == DLL):
        state.binaryDir  = state.installDir + 'lib/'
        state.binaryName = prefix + state.projectName + d + '.so'
    elif (state.binaryType == LIB):
        state.binaryDir  = state.installDir + 'lib/'
        state.binaryName = prefix + state.projectName + d + '.a'

    # Make separate directories for object files based on
    # debug/release
    state.objDir = state.tempDir + state.platform + '/' + state.target + '/'

    return state

##################################################################################

""" Checks for ice.txt and, if not found, prompts the user to create it
    and returns if they press Y, otherwise exits."""
def checkForProjectFile(state, args):
    # Assume default project file
    projectFile = 'ice.txt'
    if os.path.exists(projectFile): return

    # Everything below here executes only when there is no project file
    if not state.noPrompt:

        if ('--clean' in args) and not os.path.exists(state.buildDir):
            print
            colorPrint('Nothing to clean (you have never run iCompile in ' +
                   os.getcwd() + ')', WARNING_COLOR)
            print
            # Doesn't matter, there's nothing to delete anyway, so just exit
            sys.exit(0)

        print
        inHomeDir = (os.path.realpath(os.getenv('HOME')) == os.getcwd())

        if inHomeDir:
            colorPrint(' ******************************************************',
                       WARNING_COLOR)
            colorPrint(' * You are about run iCompile in your home directory! *',
                       'bold red')
            colorPrint(' ******************************************************',
                       WARNING_COLOR)
        else:        
            colorPrint('You have never run iCompile in this directory before.',
                       WARNING_COLOR)
        print
        print '  Current Directory: ' + os.getcwd()
    
        # Don't show dot-files first if we can avoid it
        dirs = listDirs()
        dirs.reverse()
        num = len(dirs)
        sl = shortlist(dirs)
    
        if (num > 1):
            print '  Contains', num, 'directories (' + sl + ')'
        elif (num > 0):
            print '  Contains 1 directory (' + dirs[0] + ')'
        else:
            print '  Contains no subdirectories'

        cfiles = listCFiles()
        num = len(cfiles)
        sl = shortlist(cfiles)
    
        if (num > 1):
            print '  Subdirectories contain', num, 'C++ files (' + sl + ')'
        elif (num > 0):
            print '  Subdirectories contain 1 C++ file (' + cfiles[0] + ')'
        else:
            print '  Subdirectories contain no C++ files'    

        print
    
        dir = string.split(os.getcwd(), '/')[-1]
        if inHomeDir:
            prompt = ('Are you sure you want to run iCompile '+
                      'in your home directory? (Y/N)')
        else:
            prompt = ("Are you sure you want to compile the '" +
                      dir + "' project? (Y/N)")
        
        colorPrint(prompt, 'bold')
        if string.lower(getch()) != 'y':
            sys.exit(0)

        if (num == 0):
            prompt = ("Would you like to generate a set of starter files for the '" +
                      dir + "' project? (Y/N)")
            colorPrint(prompt, 'bold')
            if string.lower(getch()) == 'y':
                prompt = "Select a project template:\n  [H]ello World\n  [G]3D\n"
                colorPrint(prompt, 'bold')
                if string.lower(getch()) == 'h':
                    templateHello.generateStarterFiles(state)
                else:
                    templateG3D.generateStarterFiles(state)

    if state.noPrompt and state.template != '':
        if (state.template == 'hello'):
            templateHello.generateStarterFiles(state)
        elif (state.template == 'G3D'):
            templateG3D.generateStarterFiles(state)
        elif (state.template == 'empty'):
            # Intentionally do nothing
            ''
        else:
            print 'ERROR: illegal template'
            
    writeFile(projectFile, defaultProjectFileContents);

    
