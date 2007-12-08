#
# Common routines for python build scripts
#
# @maintainer Morgan McGuire, matrix@graphics3d.com
#
# @created 2003-02-15
# @edited  2006-08-18
#
# This script is written in Python.  Be aware that
# whitespaces (particularly, indentation and newlines) 
# are significant in Python when editing the file.
#

import sys
import string
import os
import os.path
import fileinput
import tempfile
import shutil
import re

###############################################################################

"""
Reads the command line arguments (sys.argv) and dispatches to the appropriate
build target by name.

validTargets - A list of functions that are build targets.
help         - A build target to execute if no target is specified
"""
def dispatchOnTarget(argv, validTargets, help):

    # Execute the appropriate target:
    if (len(argv) == 1) or (argv[1] == "help"):
        # No arguments; print help
        help()
        sys.exit(-1)

    target = argv[1]
    targetArgs = argv[2:]

    found = 0
    for v in validTargets:
        if (target == v.func_name):
            v(targetArgs)
            found = 1
            break

    if (not found):
        print '"' + target + '" is not a valid build target (type "build help" to see all targets).'
        sys.exit(-1)

    sys.exit(0)


###############################################################################
"""Create a directory if it does not exist."""
def mkdir(path):
    if (not os.path.exists(path)):
        print 'mkdir ' + path
        os.makedirs(path)

def rmdir(path):
    if (os.path.exists(path)):
        print 'rmdir ' + path
        shutil.rmtree(path, 1)

def rm(path):
    if os.path.exists(path):
        print 'rm ' + path
        os.remove(path)

###############################################################################
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


###############################################################################

"""
Finds an executable on Windows."""
def findBinary(program):     
    # Paths that may contain the program

    PROGRAMFILES = os.getenv('PROGRAMFILES', '')
    SYSTEMDRIVE = os.getenv('SystemDrive', '')

    PATH = os.getenv('PATH', '').split(';') + \
          ['.',\
           PROGRAMFILES + '/Microsoft Visual Studio 8/Common7/IDE',\
           PROGRAMFILES + '/Microsoft Visual Studio/Common/MSDev98/Bin',\
           PROGRAMFILES + '/Microsoft Visual Studio .NET 2003/Common7/IDE',\
           PROGRAMFILES + '/Microsoft Visual Studio .NET 2002/Common7/IDE',\
           PROGRAMFILES + '/Microsoft Visual Studio .NET/Common7/IDE',\
           SYSTEMDRIVE + '/python',\
           SYSTEMDRIVE + '/doxygen/bin',\
           PROGRAMFILES + '/doxygen/bin',\
           PROGRAMFILES + '/PKZIP',\
           'bin']
           
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

###############################################################################

"""Runs MSDEV (VC6) on the given dsw filename and builds the 
specified configs.  configs is a list of strings
"""
def msdev(filename, configs):
    binary = 'msdev'

    logfile = tempfile.mktemp()
    args = [filename]

    for config in configs:
        args.append('/MAKE')
        args.append('"' + config + '"')

    args.append('/OUT')
    args.append(logfile)

    x = run(binary, args)
  
    # Print the output to standard out
    for line in fileinput.input(logfile):
        print line.rstrip('\n')
 
    return x

###############################################################################

"""Runs DEVENV (VC7) on the given sld filename and builds the 
specified configs.  configs is a list of strings
"""
def devenv(filename, configs):
    binary = 'devenv'

    for config in configs:
        for target in ['debug', 'release']:
            logfile = tempfile.mktemp()
            args = [filename]

            args.append('/build')
            args.append(target)
            args.append('/project "' + config + '"')

            args.append('/out')
            args.append(logfile)

            x = run(binary, args)
  
            # Print the output to standard out
            for line in fileinput.input(logfile):
                print line.rstrip('\n')

            if x != 0:
                # Abort-- a build failed
                return x;
 
    return 0

##############################################
"""Runs VCExpress (VC8) on the given sln filename and builds the 
specified configs.  configs is a list of strings
"""
def VCExpress(filename, configs):
    binary = 'VCExpress'

    for config in configs:
        for target in ['debug', 'release']:
            logfile = tempfile.mktemp()
            args = [to_local_path(filename)]

            args.append('/build')
            args.append(target)
            args.append('/project "' + config + '"')

            args.append('/out')
            args.append(logfile)

            x = run(binary, args)
  
            # Print the output to standard out
            for line in fileinput.input(logfile):
                print line.rstrip('\n')

            if x != 0:
                # Abort-- a build failed
                return x;
 
    return 0

###############################################################################
""" 
 VC8 dispatcher
"""

win32RegKeyTable = \
	{'vcexpress'	: r"HKEY_LOCAL_MACHINE\Software\Microsoft\VCExpress\8.0\Setup",
	 'vc8' 			: r"HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio\8.0\Setup\VS"}

def VC8(filename, configs):
     from regconfig import RegConfig
     # find out the flavor of MSVC
     if RegConfig(win32RegKeyTable['vc8']).has_option('Pro','ProductDir'):
         return devenv('VC8/G3D.sln', ['jpeg.lib', 'png.lib', 'zlib.lib', 'G3D.lib', 'GLG3D.lib'])
     elif RegConfig(win32RegKeyTable['vc8']).has_option('Std','ProductDir'):
         return devenv('VC8/G3D.sln', ['jpeg.lib', 'png.lib', 'zlib.lib', 'G3D.lib', 'GLG3D.lib'])
     elif RegConfig(win32RegKeyTable['vcexpress']).has_option('VS','ProductDir'):
         return VCExpress('VC8/G3D.sln', ['jpeg.lib', 'png.lib', 'zlib.lib', 'G3D.lib', 'GLG3D.lib'])
     else:
         print "Failed to find MSVC environment. Is this a valid MSDEV shell?"
         return -1

###############################################################################
"""
 Recursively zips the source into zipfile
"""
def zip(source, zipfile):
    print 'Creating ' + zipfile

    if (os.name == 'nt'):
        run('PKZIP25', ['-add', '-rec', '-lev=9', '-path=relative', zipfile, source])
    else:
        run('zip', [])

###############################################################################
"""Convert path separators to local style from Unix style.

s is a string that contains a path name."""
def to_local_path(s):
    return string.replace(s, '/', os.sep)

###############################################################################

def removeQuotes(s):
    if (s[1] == '"'):
        s = s[2:]
    if (s[(len(s)-2):] == '"'):
        s = s[:len(s)-2]
    return s


###############################################################################

"""Run a program with command line arguments.

args must be a list.
Switches the slashes from unix to dos style in program.
Blocks until shell returns, then returns the exit code of the program.
"""
def run(program, args = [], env = {}):
    program = removeQuotes(to_local_path(program))

    # Windows doesn't support spawnvp, so we have to locate the binary
    if (os.name == 'nt'):
        program = findBinary(program)

    # spawn requires specification of argv[0]
    # Because the program name may contain spaces, we
    # add quotes around it.
    if (os.name == 'nt'):
        args.insert(0, '"' + to_local_path(program) + '"')
    else:
        # Linux doesn't like quotes around its programs
        args.insert(0, program)    
        print string.join(args) + '\n'

    print '\n'

    newEnv = {}
    newEnv.update(os.environ)
    newEnv.update(env)

    program = to_local_path(program)

    #print os.getcwd(), program, args

    if (os.name == 'nt'):
        # Windows doesn't support spawnvp
        if env != {}:
            exitcode = os.spawnve(os.P_WAIT, program, args, newEnv)
        else:
            exitcode = os.spawnv(os.P_WAIT, program, args)
    else:
        if env != {}:
            exitcode = os.spawnvpe(os.P_WAIT, program, args, newEnv)
        else:
            exitcode = os.spawnvp(os.P_WAIT, program, args)

    # Since we mutated the list, remove the element
    # that was inserted.
    args.pop(0)

    return exitcode


###############################################################################
def isDigit(c):
    return (c >= '0') and (c <= '9')

"""
  verInfo: A string containing (somewhere) a version number
"""
def findVersion(verInfo):

    # Look for a number followed by a period.
    for i in xrange(1, len(verInfo) - 1):
        if verInfo[i] == '.' and \
           isDigit(verInfo[i - 1]) and \
           isDigit(verInfo[i + 1]):

            # We've found a version number.  Walk back to the
            # beginning.
            i = i - 2
            while (i > 0) and isDigit(verInfo[i]):
                i = i - 1
            i = i + 1

            version = []
            while (i < len(verInfo)) and isDigit(verInfo[i]):
                d = ''

                # Now walk forward
                while (i < len(verInfo)) and isDigit(verInfo[i]):
                    d = d + verInfo[i]
                    i = i + 1

                version.append(int(d))

                # Skip the non-digit
                i = i + 1
           
            return version     

    return [0]

###############################################################################

class Error(Exception):
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return repr(self.value)

"""
  Errors out if the version number is not at least minVerString

  e.g. checkVersion('python -V', '2.0', 'Requires python 2.0', 1)
   
  stderr: must be set to true if the program prints its version to stderr
"""
def checkVersion(verCmdString, minVerString, errString, stderr = 0):

    print '--------------------------------'
    actualVerString = shell(verCmdString, stderr)
    print '--------------------------------'
    print actualVerString

    actualVer = findVersion(actualVerString)
    minVer    = findVersion(minVerString)


    # Make the lengths match

    while (len(actualVer) < len(minVer)):
        actualVer.append(0)

    while (len(actualVer) > len(minVer)):
        minVer.append(0)

    # Check the version from most significant bit to least.
    for i in xrange(0, len(minVer)):
        if actualVer[i] < minVer[i]:
            raise Error, "*** Error: unsupported tool version ***\n" +errString
        
        if actualVer[i] > minVer[i]:
            # We are over qualified
            return

    # If the for loop completes we exactly meet the min requirement.
    
###############################################################################

def getShellRetOnly(cmd):
    mkdir('temp')
    
    try:
         os.remove('tmp/system.tmp')
    except:
        pass

    ret = os.system(cmd + ' &> temp/system.tmp')
    return ret / 256

def shell(cmd, stderr = 0):
    mkdir('temp')
    print cmd
    
    try:
        os.remove('tmp/system.tmp')
    except:
        pass

    if (stderr):
        os.system(cmd + ' >& temp/system.tmp')
    else:
        os.system(cmd + ' > temp/system.tmp')

    result = ''
    for line in fileinput.input('temp/system.tmp'):
        result = result + line

    return result

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

