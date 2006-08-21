# mpyutils.py
#
# Morgan's Python Utility Functions
#

import sys, string, os, os.path, fileinput, tempfile, shutil, re
import commands, pickle, time, ConfigParser

from icevariables import *

##############################################################################
#                              Color Printing                                #
##############################################################################

# Used by colorPrint
ansiColor = {
    'black'            :   "0",
    'red'              :   "1",
    'green'            :   "2",
    'brown'            :   "3",
    'blue'             :   "4",
    'purple'           :   "5",
    'cyan'             :   "6",
    'white'            :   "7",
    'defaultnounderscore' : "8",
    'default'          :   "9",}

ansiStyle = {
    'bold'             :   "1",
    'dim'              :   "2",  # Unsupported by most terminals
    'italic'           :   "3",  # Unsupported by most terminals
    'underline'        :   "4",
    'blink'            :   "5",  # Unsupported by most terminals
    'fastblink'        :   "6",  # Unsupported by most terminals
    'reverse'          :   "7",
    'hidden'           :   "8",  # Unsupported by most terminals
    'strikethrough'    :   "9"}  # Unsupported by most terminals

""" Used by colorPrint """
useColor = 'Unknown'

""" If the terminal supports color, prints in the specified color.  
    Otherwise, prints using normal color. The color argument
    must have the form:

    [bold|underline|reverse|italic|blink|fastblink|hidden|strikethrough] [FGCOLOR] [on BGCOLOR]

    COLOR = {default, black, red, green, brown, blue, purple, cyan, white}

    Yellow = light brown, Pink = light red, etc.

"""
def colorPrint(text, color = 'default'):
    global useColor

    if useColor == 'Unknown':
        # Figure out if this device supports color
        useColor = (os.environ.has_key('TERM') and
                    ((os.environ['TERM'] == 'xterm') or 
                     (os.environ['TERM'] == 'xterm-color')))
   
    if not useColor:

        print text
    
    else:

        # Parse the color

        # First divide up into lower-case words
        tokens = string.lower(color).split(' ')
        if len(tokens) == 0:
            # Give up
            print ('Warning: illegal icompile color specified ("' +
                   color + '")\n\n')
            useColor = false
            print text
            return

        styleString     = ''
        foreColorString = ''
        backColorString = ''        

        if ansiStyle.has_key(tokens[0]):
            styleString = tokens[0]
            tokens = tokens[1:]

        if (len(tokens) > 0) and (tokens[0] != 'on'):
            # Foreground color
            foreColorString = tokens[0]
            tokens = tokens[1:]

        if len(tokens) > 0:
            # Background color, must start with 'on' keyword
            if (tokens[0] != 'on') or (len(tokens) < 2):
                # Give up
                useColor = false
                print ('Warning: illegal icompile background color' +
                       ' specified ("' + color + '")\n\n')
                print text
                return
            backColorString = tokens[1]

        foreDigit = '3'
        backDigit = '4'

        style     = ''
        foreColor = ''
        backColor = ''
    
        if styleString != '':
            style     = ansiStyle[styleString]

        if (foreColorString != '') and ansiColor.has_key(foreColorString):
            foreColor = foreDigit + ansiColor[foreColorString]

        if (backColorString != '') and ansiColor.has_key(backColorString):
            backColor = backDigit + ansiColor[backColorString]

        featureString = ''
        for s in [style, foreColor, backColor]:
            if (s != ''):
                if (featureString != ''):
                    featureString = featureString + ';' + s
                else:
                    featureString = s

        openCol = '\033['
        closeCol = 'm'
        stop = openCol + '0' + closeCol
        start = openCol + featureString + closeCol
        print start + text + stop


WARNING_COLOR = 'bold red'
SECTION_COLOR = 'bold'
COMMAND_COLOR = 'green'

def printBar():
    print "_______________________________________________________________________\n"

def beep():
    print '\a'

##############################################################################
#                                 getch                                      #
##############################################################################

class _Getch:
    """Gets a single character from standard input.  Does not echo to the
    screen."""
    def __init__(self):
        try:
            self.impl = _GetchWindows()
        except ImportError:
            self.impl = _GetchUnix()
            
    def __call__(self): return self.impl()
            
            
class _GetchUnix:
    def __init__(self):
        import tty, sys
        
    def __call__(self):
        import sys, tty, termios
        fd = sys.stdin.fileno()
        old_settings = termios.tcgetattr(fd)
        try:
            tty.setraw(sys.stdin.fileno())
            ch = sys.stdin.read(1)
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
            return ch
            
            
class _GetchWindows:
    def __init__(self):
        import msvcrt
        
    def __call__(self):
        import msvcrt
        return msvcrt.getch()
        
        
getch = _Getch()


##############################################################################
#                               Shell Helpers                                #
##############################################################################

"""Create a directory if it does not exist."""
def mkdir(path, echo = True):
    if (path[-1] == "/"):
        path = path[:-1]

    if ((not os.path.exists(path)) and (path != "./")):
        if echo: colorPrint('mkdir ' + path, COMMAND_COLOR)
        # TODO: set group and permissions from parent directory
        os.makedirs(path)

##############################################################################

""" Turns a list of filenames into a list of at most four directories. """
def shortlist(L):
    num = len(L)
    if (num > 4):
        return L[0] + ', ' + L[1] + ', ' + L[2] + ', ' + L[3] + ', ...'
    if (num > 3):
        return L[0] + ', ' + L[1] + ', ' + L[2] + ', ...'
    if (num > 2):
        return L[0] + ', ' + L[1] + ', ...'
    elif (num > 1):
        return L[0] + ', ' + L[1]
    elif (num > 0):
        return L[0]
    else:
        return ''
    
##############################################################################
        
"""Recursively remove a directory tree if it exists."""
def rmdir(path, echo = True):
    if (os.path.exists(path)):
        if echo: colorPrint('rm -rf ' + path, COMMAND_COLOR)
        shutil.rmtree(path, 1)

""" Remove a single file, if it exists. """
def rm(file, echo = True):
    if (os.path.exists(file)):
        if echo: colorPrint('rm ' + file, COMMAND_COLOR)
        os.remove(file)

##############################################################################

""" Runs a program and returns a string of its output. """
def shell(cmd, printCmd = True):
    if printCmd: maybeColorPrint(cmd, COMMAND_COLOR)
    return commands.getoutput(cmd)

##############################################################################
        
"""Finds an executable on Windows."""
def _findBinary(program):     
    # Paths that may contain the program

    PATH = os.getenv('PATH', '').split(';') + \
          ['.',\
           'C:/Program Files/Microsoft Visual Studio/Common/MSDev98/Bin',\
           'C:/python',\
           'C:/doxygen/bin',\
           'C:/Program Files/PKZIP']

    for path in PATH:
        filename = path + '/' + program + '.exe'
        if (os.path.exists(filename)):
            return filename
            break

        filename = path + '/' + program + '.com'
        if (os.path.exists(filename)):
            return filename
            break

    raise 'Error', 'Cannot find "' + program + '"'
    return program


"""Convert path separators to local style from Unix style.
   s is a string that contains a path name."""
def toLocalPath(s):
    return string.replace(s, '/', os.sep)

#############################################################################

"""Run a program with command line arguments.

args must be a list of arguments (argv).  Spaces in arguments are *not* the same as having
separate list elements; these will not be re-parsed when they become the argv 
strings.

Switches the slashes from unix to dos style in program.
Blocks until shell returns, then returns the exit code of the program.
"""
def run(program, args = [], echo = True):
    # Must have platform correct slashes
    program = toLocalPath(program)

    # Windows doesn't support spawnvp, so we have to locate the binary
    if (os.name == 'nt'):
        program = _findBinary(program)

    # If the program name contains spaces, we
    # add quotes around it.
    if (' ' in program) and not ('"' in program):
        program = '"' + program + '"'

    # spawn requires specification of argv[0]
    args = [program] + args
    if echo: colorPrint(string.join(args), COMMAND_COLOR)

    if (os.name == 'nt'):
        # Windows doesn't support spawnvp
        exitcode = os.spawnv(os.P_WAIT, program, args)
    else:
        exitcode = os.spawnvp(os.P_WAIT, program, args)

    # Since we mutated the list, remove the element
    # that was inserted.
    args.pop(0)

    return exitcode

##############################################################################

"""Returns 0 if the file does not exist, otherwise returns the modification
time of the file."""
def getTimeStamp(file):
   try:
       return os.path.getmtime(file)
   except OSError:
       return

##############################################################################

"""Determine if a target is out of date.

Returns nonzero if file1 is newer than file2.
Throws an error if file1 does not exist, returns
nonzero if file2 does not exist."""
def newer(file1, file2):
   time1 = os.path.getmtime(file1)
   time2 = 0
   try:
       time2 = os.path.getmtime(file2)
   except OSError:
       time2 = 0
       
   return time1 >= time2


""" Removes quotation marks from the outside of a string. """
def removeQuotes(s):
    if (s[1] == '"'):
        s = s[2:]
    if (s[(len(s)-2):] == '"'):
        s = s[:len(s)-2]
    return s

###############################################################################

"""
  verInfo: A string containing (somewhere) a version number.  Typically, the
  output of commands.getoutput().  Returns the version as a list of version
  numbers.
"""
def findVersionInString(verInfo):

    # Look for a number followed by a period.
    for i in xrange(1, len(verInfo) - 1):
        if (verInfo[i] == '.' and 
           (verInfo[i - 1]).isdigit() and 
           (verInfo[i + 1]).isdigit()):

            # We've found a version number.  Walk back to the
            # beginning.
            i -= 2
            while (i > 0) and verInfo[i].isdigit():
                i -= 1
            i += 1

            version = []
            while (i < len(verInfo)) and verInfo[i].isdigit():
                d = ''

                # Now walk forward
                while (i < len(verInfo)) and verInfo[i].isdigit():
                    d += verInfo[i]
                    i += 1

                version.append(int(d))

                # Skip the non-digit
                i += 1
           
            return version     

    return [0]

###############################################################################

""" Takes a version list and converts it to a string."""
def versionToString(v):
    s = ''
    for i in v:
        s += str(i) + '.'
    return s[:-1]

#############################################################################

def beginswith(a, prefix):
   return a[:len(prefix)] == prefix


def removeTrailingSlash(s):
    if (s[-1] == '/'):
        s = s[:-1]
    if (s[-1] == '\\'):
        s = s[:-1]
    return s

"""
Strips the path from the front of a filename.

os.path.basename strips one extra character from the beginning.
This restores it.
"""
def betterbasename(filename):
    # Find the index of the last slash
    i = max(string.rfind(filename, '/'), string.rfind(filename, '\\'))

    # Copy from there on (whole string if no slashes)
    return filename[(i + 1):]


""" Returns the part of a full filename after the path and before the last ext"""
def rawfilename(filename):
    f = betterbasename(filename)
    period = string.find(f, '.')

    if period > 0:
        return f[0:period]
    else:
        return f
    

""" Returns the extensions from a full filename."""
def extname(filename):
    f = betterbasename(filename)
    period = string.find(filename, '.')

    if period > 0:
        return f[(period + 1):]
    else:
        return ''

"""
 Concatenates a file or path onto a path with a '/' if the first
 is non-empty and lacks a '/'
"""
def pathConcat(a, b):
    if len(b) == 0:
        return a
 
    if len(a) == 0:
        return b

    # remove any leading slash from b
    if ((b[0] == '/') or
        (b[0] == '\\')):
       b = b[1:]

    if ((a != '') and
        (a[-1] != '/') and
        (a[-1] != '\\')):
        return a + '/' + b
    else:
        return a + b

""" Returns the version number of a file as a list.  Note that under
comparison, 1.10 != 1.1 and 1.01 == 1.1, which is usually what you
want.
"""
def getVersion(filename):
    cmd = filename

    base = betterbasename(filename)

    # We check only the beginning of a filename because it may have
    # a version number as part of the name.
    if beginswith(base, 'g++'):
        cmd = filename + ' --version'
    elif beginswith(base, 'python'):
        cmd = filename + " -V"
    elif beginswith(base, 'cl'):
        # MSVC++ compiler
        cmd = filename
    elif beginswith(base, 'doxygen'):
        cmd = filename + " --version"
    elif beginswith(base, 'ar'):
        cmd = filename + " --version"
    elif beginswith(base, 'ld'):
        cmd = filename + " --version"
    else:
        # Unsupported
        return [0, 0, 0]
    
    return findVersionInString(commands.getoutput(cmd))

def maybeColorPrint(text, color = 'default'):
    if verbosity >= VERBOSE:
        colorPrint(text, color)

""" Prints a line if quiet is false. """
def maybePrintBar():
    if verbosity >= VERBOSE:
        printBar()

""" Returns a list of all directories (without '..') that are
    next to this directory.
"""
def getSiblingDirs():
    siblings = []
    me = betterbasename(os.getcwd())

    for node in os.listdir('..'):
        # See if the node is a directory (and not *this* directory!)
        if node != me and os.path.isdir('../' + node):
            siblings.append(node)

    return siblings

""" Returns the index of x in list L, starting at start.  Returns -1 if not found."""
def find(L, x, start = 0):
    i = start
    while i < len(L):
        if L[i] == x:
            return i
        i += 1
    return -1   