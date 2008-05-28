#
# icedepend.py
#
# Determines if files are out of date. Routines:
# 
#  getOutOfDateFiles
#  getObjectFilename
#  getDependencies
#  getDependencyInformation
#  getLibrarySiblingDirs
#  identifySiblingLibraryDependencies
#

from utils import *
from variables import *
from sets import Set
from library import *
from doticompile import *
from help import *

# If True, the project will not be rebuilt if all dependencies
# other than iCompile are up to date.  This is handy when
# working on iCompile and testing with large libraries.
_IGNORE_ICOMPILE_DEPENDENCY = False

###############################################################

"""
  cfiles - list of fully qualified source files
  dependences - table(cfile, list of files on which it depends)
  files - list of all fully qualified files that are the dependencies of anything
  timeStamp - table(filename, timestamp) for everything in files list

  Returns a list of all files that are older than their dependencies.
  
  The dependences have already been recursively expanded; if A depends on B and
  B depends on C, then dependencies[A] = [B, C]
"""
def getOutOfDateFiles(state, cfiles, dependencies, files, timeStamp):
    
    buildList = []
    
    # Need to rebuild all if iCompile itself was modified more
    # recently than a given file.
    icompileTime = getTimeStamp(sys.argv[0])
    if _IGNORE_ICOMPILE_DEPENDENCY:
        # Set to the beginning of time, so that it looks like
        # iCompile was never modified.
        icompileTime = 0

    # Rebuild all if ice.txt or .icompile was modified
    # more recently.
    if os.path.exists('ice.txt'):
        iceTime = getTimeStamp('ice.txt')
        if iceTime > icompileTime:
            icompileTime = iceTime

    if os.path.exists(state.preferenceFile()):
        configTime = getTimeStamp(state.preferenceFile())
        if configTime > icompileTime:
            icompileTime = configTime


    # Generate a list of all files newer than their targets
    for cfile in cfiles:
        ofile = getObjectFilename(state, cfile)
        otime = getTimeStamp(ofile)

        # See if the object file is out of date because it is older than iCompile
        rebuild = (otime < icompileTime)
        if rebuild and (verbosity >= TRACE):
            print 'iCompile is newer than ' + ofile

        # See if the object file is out of date because it is older than a dependency
        if not rebuild:
            # Check dependencies
            for d in dependencies[cfile]:
                dtime = timeStamp[d]
                if otime < dtime:
                    if verbosity >= TRACE:
                        print d + " is newer than " + ofile
                    rebuild = 1
                    break

        if rebuild:
            # This file needs to be rebuilt
            buildList.append(cfile)

    return buildList

#########################################################################

"""Given a source file (relative to rootDir) returns an object file
  (relative to rootDir).
"""
def getObjectFilename(state, sourceFile):
    # strip the extension and replace it with .o
    i = string.rfind(sourceFile, '.')
    return state.objDir + sourceFile[len(state.rootDir):i] + '.o'

#########################################################################

# Returns a new list that has any -arch flags removed from oldList.
def _removeArch(oldList):
    if not ('-arch' in oldList):
        return oldList
    
    newList = []
    skipNext = False
    for arg in oldList:
        if skipNext:
            # explicitly skip this element, but disable skipping from now on
            skipNext = False
        elif (arg == '-arch'):
            skipNext = True
            # and implicitly skip this element
        else:
            newList += [arg]

    return newList


"""

   Returns a list of *all* files on which file depends (including
   itself).  Returned filenames will either be fully qualified or
   relative to state.rootDir.

   May modify the default compiler and linker options as a result of
   discovering common library dependencies.
 
   timeStamp = table(filename, timestamp) that is updated as getDependencies runs   
"""
def getDependencies(state, file, verbosity, timeStamp, iteration = 1):
    # dependencyCache: table(file, (timestamp at which dependencies were computed, dependency list))
    if not state.cache.has_key('dependencies'):
        state.cache['dependencies'] = {}
    dependencyCache = state.cache['dependencies']
    
    # Assume that we can trust the cache, and try to prove otherwise
    trustCache = dependencyCache.has_key(file)
    if trustCache:
        entry = dependencyCache[file]
        timeComputed = entry[0]
        if getTimeStampCached(file, timeStamp) > timeComputed:
            if verbosity >= SUPERTRACE:
                print ('Cannot use cached dependency information for ' + file +
                       ' because it has changed.')
            # The file has changed since we checked dependencies
            trustCache = False
        else:
            dependencies = entry[1]
            for d in dependencies:
                if getTimeStampCached(d, timeStamp) > timeComputed:
                    if verbosity >= SUPERTRACE:
                        print ('Cannot use cached dependency information for ' + file +
                        ' because ' + d + ' has changed.')
                    # One of the dependencies has itself changed since
                    # we checked dependencies, which means that there might
                    # be new dependencies for file
                    trustCache = False
                    break
    else:
        if verbosity >= SUPERTRACE:
            print 'There is no cached dependency information for ' + file + '.'
                    
    if trustCache:
        # The entry for this file is up to date
        if verbosity >= SUPERTRACE: print 'Using cached dependency information for ' + file        
        return dependencies
    
    # ...otherwise, dependency information for this file is out of date, so recompute it

    # We need to use the -msse2 flad during dependency checking because of the way
    # xmmintrin.h is set up
    #
    # -MG means "don't give errors for header files that don't exist"
    # -MM means "don't tell me about system header files"
    #
    # We have to remove -arch flags, which are not compatible with the -M flags
    raw = shell(state.compiler + ' -M -msse2 -MG ' +
                string.join(_removeArch(getCompilerOptions(state, [])), ' ') + ' ' + file,
                verbosity >= SUPERTRACE)
    
    if verbosity >= SUPERTRACE:
        print 'ffff'
        print raw

    if raw.startswith('In file included from'):
        # If there was an error, the output will have the form
        # "In file included from _____:" and a list of files
        
        if iteration == 3:
            # Give up; we can't resolve the problem
            print raw
            sys.exit(-1)
        
        if verbosity >= VERBOSE:
            print ('\nThere were some errors computing dependencies.  ' +
                   'Attempting to recover.('), iteration,')'
        
        # Generate a list of files and see if they are something we
        # know how to fix.
        noSuchFile = []
        for line in string.split(raw, '\n'):
            if line.endswith(': No such file or directory'):
                x = line[:-len(': No such file or directory')]
                j = x.rfind(': ')
                if (j >= 0): x = x[(j+2):]

                # x now has the filename
                noSuchFile.append(betterbasename(x))

        if verbosity >= NORMAL: print 'Files not found:', noSuchFile
        
        # Look for specific header files that we know how to handle
        for f in noSuchFile:
            if f == 'wx.h':
                if verbosity >= NORMAL: print 'wxWindows detected.'
                state.compilerOptions.append(shell('wx-config --cxxflags', verbosity >= VERBOSE))
                state.linkerOptions.append(shell('wx-config --gl-libs --libs', verbosity >= VERBOSE))
                
        return getDependencies(state, file, verbosity, timeStamp, iteration + 1)

    else:

        # Normal case, no error
        if 'implemented' in raw: print raw
        result = []
        raw = raw.replace('\\', ' ')
        for line in raw.splitlines():
            # gcc 3.4.2 likes to print the name of the file first, as in
            # """# 1 "/home/faculty/morgan/Projects/ice/tests/meta/helper.lib//""""
            if not line.startswith('# '):
                result += string.split(line, ' ')

        # There is always at least one file in the raw file list,
        # since every file depends on itself.
        
        # The first element of result will be "file:", so strip it
        files = result[1:]
        result = []

        # Add the './' to raw files
        for f in files:
            f = f.strip()
            if '/' in f:
                result.append(f)
            elif f == '':
                # empty filename
                files.remove(f)
            else:
                result.append('./' + f)

        # Update the cache
        dependencyCache[file] = (time.time(), result)

        return result


#########################################################################

"""Finds all of the C++/C files in rootDir and returns
 dependency information for them as the tuple
 
   (allCFiles, dependencies, allDependencyFiles, parents)

 where: 
 
 allCFiles is the list of all source files to compile,

 dependencies[f] is the list of all files on which
 f depends,

 allDependencyFiles is the list of all files
 on which *any* file depends, and

 parents[f] is the list of all files that depend on f

 While getting dependencies, this may change the include/library
 list and restart the process if it appears that some include
 directory is missing.
"""
def getDependencyInformation(state, verbosity):
    # Cached time stamps
    timeStamp = {}
    
    # Hash table mapping C files to the list of all files
    # on which they depend.
    dependencies = {}

    parents = {}

    # All files on which something depends
    dependencySet = Set()

    # Find all the c files
    allCFiles = listCFiles(state.rootDir, state.excludeFromCompilation)

    if verbosity >= TRACE: print 'Source files found:', allCFiles

    # Get their dependencies
    for cfile in allCFiles:

        # Do not use warning or verbose options for this; they
        # would corrupt the output.
        dlist = getDependencies(state, cfile, verbosity, timeStamp)

        # Update the dependency set
        for d in dlist: 
            dependencySet.add(d)
            if d in parents:
                parents[d] += cfile
            else:
                parents[d] = [cfile]
 
        dependencies[cfile] = dlist
    
    # List of all files on which some other file depends
    dependencyFiles = [d for d in dependencySet]

    return (allCFiles, dependencies, dependencyFiles, parents)

###############################################################################


""" Returns sibling directories that are also iCompile libraries as names
    relative to the current directory.
    Allowed to recurse back two parent directories"""
def getLibrarySiblingDirs(howFarBack = 1):
    libsibs = []
    for dir in getSiblingDirs(howFarBack):

        # See if it contains an ice.txt and ends in a library extension
        ext = extname(dir).lower()
       
        if ((ext == 'lib') or (ext == 'a') or 
            (ext == 'so') or  (ext == 'dll')):
            libsibs.append(dir)

    return libsibs

#############################################################################

"""

For any header in files that is not found in the current include paths,
try to find it in a sibling directory that is an icompile library
and extend the state.includePaths and state.usesLibraryList as needed.

Called from buildBinary.

"""
def identifySiblingLibraryDependencies(files, parents, state):

    # See if any of the header files are not found; that might imply
    # sibling project is an implicit dependency.
    # Include the empty path on this list so that we'll find files
    # that are fully qualified as well as files in the current directory.
    # This list will be extended as we go forward.
    incPaths = [''] + state.includePaths()
    librarySiblingDirs = getLibrarySiblingDirs(3)

    for header in files:

        # Try to locate the file using the existing include path
        found = False
        i = 0
        while not found and (i < len(incPaths)):
            found = os.path.exists(pathConcat(incPaths[i], header))
            i += 1

        # Try to locate the file in a sibling directory
        if not found:
            # This header doesn't exist in any of the standard include locations.
            # See if it is in a sibling directory that is a library.
            i = 0

            while not found and (i < len(librarySiblingDirs)):
                dirname = librarySiblingDirs[i]
                found = os.path.exists(dirname + '/include/' + header)

                if found:
                    if verbosity >= TRACE: print "Found '" + header + "' in '" + dirname + "/include'."
                    # We have identified a sibling library on which this project appears
                    # to depend.  

                    type = EXE
                    libname = rawfilename(dirname)
                    ext = extname(dirname).lower()
                    if ext == 'dll' or ext == 'so':
                        type = DLL
                    elif ext == 'lib' or ext == 'a':
                        type = LIB

                    if isLibrary(type):
                        if not libraryTable.has_key(libname):
                           defineLibrary(Library(libname, type, libname, libname + 'd',  
	                                         None,  None, [betterbasename(header)], [], []))

                        if not libname in state.usesLibrariesList:
                            state.usesLibrariesList.append(libname)
                            
                        if not dirname in state.usesProjectsList:
                            print ('Detected dependency on ' + dirname + ' from inclusion of ' + 
                                   header + ' by'), shortname(state.rootDir, parents[header][0])

                            state.usesProjectsList.append(dirname)

                            # Load the configuration state for the dependency project
                            curDir = os.getcwd()
                            os.chdir(dirname)
                            other = getConfigurationState(state.args)
                            os.chdir(curDir)

                            includepath = pathConcat(dirname, 'include')
                            libpath = pathConcat(dirname, other.binaryDir)

                            state.addIncludePath(includepath, False)
                            state.addLibraryPath(libpath, False)
                            
                            # Update incPaths, which also includes the empty directory (that will
                            # not appear as a -I argument to the compiler because doing so generates an error)
                            incPaths = [''] + state.includePaths()

                            # TODO: recursively add all dependencies that come from this new header/library
                i += 1

        if not found:
            maybeWarn("Header file not found: '" + header + "'.", state)
