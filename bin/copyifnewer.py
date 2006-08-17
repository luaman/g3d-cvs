# copyifnewer.py
#
#

import re, string
from mpyutils import *

_excludeDirPatterns = \
    ['^\.',\
     '^#',\
     '^\.svn$',\
     '^CVS$', \
     '^Debug$', \
     '^Release$', \
     '^graveyard$', \
     '^tmp$', \
     '^temp$', \
     '^doc-files$', \
     '^data-files$', \
     '^\.icompile-temp$', \
     '^build$']

"""
  Regular expression patterns (i.e., directory patterns) that are 
  excluded from the search for cpp files
"""
_cppExcludePatterns = ['^old$'] + _excludeDirPatterns

""" Regular expression patterns that will be excluded from copying by 
    copyIfNewer.
"""
_excludeFromCopyingPatterns =\
    ['\.ncb$', \
    '\.opt$', \
    '\.ilk$', \
    '\.pdb$', \
    '\.bsc$', \
    '\.o$', \
    '\.obj$', \
    '\.pyc$', \
    '\.plg$', \
    '^#.*#$', \
    '~$', \
    '\.old$' \
    '^log.txt$', \
    '^stderr.txt$', \
    '^stdout.txt$', \
    '\.log$', \
    '\^.cvsignore$'] + _excludeDirPatterns

"""
A regular expression matching files that should be excluded from copying.
"""
excludeFromCopying  = re.compile(string.join(_excludeFromCopyingPatterns, '|'))

_copyIfNewerCopiedAnything = False

"""
Recursively copies all contents of source to dest 
(including source itself) that are out of date.  Does 
not copy files matching the excludeFromCopying patterns.
"""
def copyIfNewer(source, dest):
    global _copyIfNewerCopiedAnything
    _copyIfNewerCopiedAnything = False
    
    if source == dest:
        # Copying in place
        return

    dest = removeTrailingSlash(dest)

    if (not os.path.exists(source)):
        # Source does not exist
        return

    if (not os.path.isdir(source) and newer(source, dest)):
        maybeColorPrint('cp ' + source + ' ' + dest, COMMAND_COLOR)
        shutil.copyfile(source, dest)
        _copyIfNewerCopiedAnything = YES
        
    else:

        # Walk is a special iterator that visits all of the
        # children and executes the 2nd argument on them.  
        os.path.walk(source, _copyIfNewerVisit, [len(source), dest])

    if not _copyIfNewerCopiedAnything and not quiet:
        print dest + ' is up to date with ' + source
    
#########################################################################
    
"""Helper for copyIfNewer.

args is a list of:
[length of the source prefix in sourceDirname,
 rootDir of the destination tree]
"""
def _copyIfNewerVisit(args, sourceDirname, names):
    global _copyIfNewerCopiedAnything

    prefixLen   = args[0]
    # Construct the destination directory name
    # by concatenating the root dir and source dir
    destDirname = pathConcat(args[1], sourceDirname[prefixLen:])
    dirName     = betterbasename(destDirname)
        
    if (excludeFromCopying.search(dirName) != None):
        # Don't recurse into subdirectories of excluded directories
        del names[:]
        return

    # Create the corresponding destination dir if necessary
    mkdir(destDirname)

    # Iterate through the contents of this directory   
    for name in names:
        source = pathConcat(sourceDirname, name)

        if ((excludeFromCopying.search(name) == None) and 
            (not os.path.isdir(source))):
            
            # Copy files if newer
            dest = pathConcat(destDirname, name)
            if (newer(source, dest)):
                maybeColorPrint('cp ' + source + ' ' + dest, COMMAND_COLOR)
                _copyIfNewerCopiedAnything = True
                shutil.copyfile(source, dest)

