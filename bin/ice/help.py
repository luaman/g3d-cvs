# help.py
#
# Help Information and Printing Routines for iCompile


from variables import *
from utils import *
import copyifnewer

##############################################################################
#                                  Version                                   #
##############################################################################

def printVersion():
    print "iCompile " + versionToString(version)
    print "Copyright 2003-2007 Morgan McGuire"
    print "All rights reserved"
    print
    print "http://ice.sf.net"
    print

##############################################################################
#                                    Help                                    #
##############################################################################

def printHelp():
    print ("""
iCompile: the zero-configuration build system
    
icompile  [--doc] [--opt|--debug] [--clean] [--version]
          [--config <custom .icompile>] [--verbosity n]
          [--help] [--noprompt] [--run|--gdb ...]

iCompile can build most C++ projects without options or manual
configuration.  Just type 'icompile' with no arguments.  Run in an
empty directory to generate a set of starter files.

Options:
 --doc            Generate documentation before building.
 
 --debug          (Default) Create a debug executable (define _DEBUG,
                  disable optimizations).
                  
 --opt or -O      Generate an optimized executable.
 
 --run            Run the program if compilation succeeds, passing all
                  further arguments (...) to the program.

 --config <file>  Use <file> instead of ~/.icompile as the user configuration
                  file.  This allows you to build certain projects with
                  a different compiler or include paths without changing the
                  project ice.txt file, e.g., when installing a 3rd party library
                  
 --gdb            Run the program under gdb if compilation succeeds,
                  passing all further arguments (...) to the program.
                  You can also just run gdb yourself after using iCompile.

 --verbosity n    Change the amount of information printed by icompile

                  n   |   Result
                  ----|---------------------------------------------------
                  0   |   Quiet:  Only errors and prompts are displayed.
                      |
                  1   |   Normal (Default): Filenames and progress information
                      |   are also displayed
                  2   |   Verbose: Executed commands are also displayed
                      |
                  3   |   Trace: Additional debugging information is also
                      |   displayed.

 --noprompt       Run even if there is no ice.txt file, don't prompt the
                  user for input.  This is handy for launching iCompile from
                  automated build shell scripts.

Exclusive options:
 --help           Print this information.
 
 --clean          Delete all generated files (but not library generated files).
 
 --version        Print the version number of iCompile.

Special directory names:
 build            Output directory
 data-files       Files that will be needed at runtime
 doc-files        Files needed by your documentation (Doxygen output)

iCompile will not look for source files in directories matching: """ +
           str(copyifnewer._excludeDirPatterns) +
"""

You may edit ice.txt and ~/.icompile if your project has unusual configuration
needs.  See manual.html or http://ice.sf.net for full information.
""")
    sys.exit(0)


""" If the exact warning string passed in hasn't been printed
in the past 72 hours, it is printed and listed in the cache under
the "warnings" key.  Otherwise it is supressed."""
def maybeWarn(warning, state):

    MINUTE = 60
    HOUR = MINUTE * 60
    DAY = HOUR * 12
    WARNING_PERIOD = 3 * DAY
    now = time.time()

    if state == None or state.cache == None:
        # the cache has not been loaded yet
	colorPrint(warning, WARNING_COLOR)
        return

    if not state.cache.has_key('warnings'):
        state.cache['warnings'] = {}

    allWarnings = state.cache['warnings']

    if (not allWarnings.has_key(warning) or
        ((allWarnings[warning] + WARNING_PERIOD) < time.time())):

        # Either this key is not in the dictionary or
        # the warning period has expired.  Print the warning
        # and update the time in the cache
        allWarnings[warning] = time.time()
        colorPrint(warning, WARNING_COLOR)
