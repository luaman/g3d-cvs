# copyifnewer.py
#
#

import re, string
from utils import *

_excludeDirPatterns = \
    ['^\.',\
     '^#',\
     '~$',\
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
     '^\.ice-tmp$', \
     '^build$']


""" Regular expression patterns that will be excluded from copying by 
    copyIfNewer.
"""
_excludeFromCopyingPatterns =\
    ['\.ncb$', \
    '\.opt$', \
    '\.ilk$', \
    '\.cvsignore$', \
    '^\.\#', \
    '\.pdb$', \
    '\.bsc$', \
    '^\.DS_store$', \
    '\.o$', \
    '\.pyc$', \
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
  Regular expression patterns (i.e., directory and filename patterns) that are 
  excluded from the search for cpp files
"""
_cppExcludePatterns = ['^test$', '^tests$', '^#.*#$', '~$', '^old$'] + _excludeFromCopyingPatterns

"""
A regular expression matching files that should be excluded from copying.
"""
excludeFromCopying  = re.compile(string.join(_excludeFromCopyingPatterns, '|'))

_copyIfNewerCopiedAnything = False

"""
Recursively copies all contents of source to dest 
(including source itself) that are out of date.  Does 
not copy files matching the excludeFromCopying patterns.

Returns true if any files were copied

If actuallyCopy is false, doesn't actually copy the files, but still prints.
"""
def copyIfNewer(source, dest, echoCommands = True, echoFilenames = True, actuallyCopy = True):
    global _copyIfNewerCopiedAnything
    _copyIfNewerCopiedAnything = False
    
    if source == dest:
        # Copying in place
        return False

    dest = removeTrailingSlash(dest)

    if (not os.path.exists(source)):
        # Source does not exist
        return False

    if (not os.path.isdir(source) and newer(source, dest)):
        if echoCommands: 
            colorPrint('cp ' + source + ' ' + dest, COMMAND_COLOR)
        elif echoFilenames:
            print source
        
        if actuallyCopy:
            shutil.copyfile(source, dest)
            
        _copyIfNewerCopiedAnything = True
        
    else:

        # Walk is a special iterator that visits all of the
        # children and executes the 2nd argument on them.  
        os.path.walk(source, _copyIfNewerVisit, 
                     [len(source), dest, echoCommands, echoFilenames, actuallyCopy])

    if not _copyIfNewerCopiedAnything and echoCommands:
        print dest + ' is up to date with ' + source
        
    return _copyIfNewerCopiedAnything
    
#########################################################################
    
"""Helper for copyIfNewer.

args is a list of:
[length of the source prefix in sourceDirname,
 rootDir of the destination tree,
 echo commands
 echo filenames]
"""
def _copyIfNewerVisit(args, sourceDirname, names):
    global _copyIfNewerCopiedAnything

    prefixLen   = args[0]
    # Construct the destination directory name
    # by concatenating the root dir and source dir
    destDirname = pathConcat(args[1], sourceDirname[prefixLen:])
    dirName     = betterbasename(destDirname)

    echoCommands = args[2]
    echoFilenames = args[3]
    actuallyCopy = args[4]
        
    if (excludeFromCopying.search(dirName) != None):
        # Don't recurse into subdirectories of excluded directories
        del names[:]
        return

    # Create the corresponding destination dir if necessary
    if actuallyCopy:
        mkdir(destDirname, echoCommands)

    # Iterate through the contents of this directory   
    for name in names:
        source = pathConcat(sourceDirname, name)

        if ((excludeFromCopying.search(name) == None) and 
            (not os.path.isdir(source))):
            
            # Copy files if newer
            dest = pathConcat(destDirname, name)
            if (newer(source, dest)):
                if echoCommands:
                    colorPrint('cp ' + source + ' ' + dest, COMMAND_COLOR)
                elif echoFilenames: 
                    print name
                _copyIfNewerCopiedAnything = True
                if actuallyCopy:
                    shutil.copyfile(source, dest)


                    
