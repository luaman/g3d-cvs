# templateG3D.py
#

from utils import *
from variables import *
import copyifnewer
import os
import doticompile
import copy

def findG3DStarter(state):

    # load the default .icompile
    fakeState = copy.copy(state)
    doticompile.processProjectFile(fakeState, True)

    locations = [pathConcat(x, '../') for x in fakeState.includePaths()]
    locations += [
        pathConcat(os.environ['HOME'], 'Projects/G3D/'),
        '/usr/local/G3D/']
    
    for path in locations:
        f = pathConcat(path, 'demos/starter')
        if os.path.exists(f):
            return f

    raise Exception('Could not find G3D installation in ' + '\n'.join(locations))

""" Generates an empty project. """
def generateStarterFiles(state):
    if isLibrary(state.binaryType):
        colorPrint("ERROR: G3D template cannot be used with a library", ERROR_COLOR)
        sys.exit(-232)
                
    starterPath = findG3DStarter(state)
    
    print '\nCopying G3D starter files from ' + starterPath

    mkdir('doc-files')
    for d in ['data-files', 'source']:
        copyifnewer.copyIfNewer(pathConcat(starterPath, d), pathConcat('source', d))
     

