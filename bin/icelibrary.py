# library.py
#
# Definition of C libraries available for linking

from icetopsort import *

# Files can trigger additional linker options.  This is used to add
# libraries to the link list based on what is #included.  Used by
# getLinkerOptions.

STATIC    = 1
DYNAMIC   = 2
FRAMEWORK = 3

class Library:
    name             = None
    
    # STATIC or DYNAMIC.  If it only exists as a framework, then FRAMEWORK
    type             = STATIC
    releaseLib       = None
    debugLib         = None

    # None if there is no OS X framework.  If there is a framework,
    # it will be preferred on OS X
    releaseFramework = None
    debugFramework   = None

    # List of all headers that should trigger a link
    # to this library
    headerList       = None

    # If any of these symbols are unbound in the final object files then
    # trigger a link to this library.  This helps us find dependencies
    # of static libraries, which are otherwise not revealed by headers.
    symbolList       = None

    # Other libraries, by canonical name, on which this library depends.
    # This is both a table of known dependencies and input to the topological
    # sort for link ordering
    dependsOnList    = None

    def __init__(self, name, type, releaseLib, debugLib, 
                 releaseFramework, debugFramework, headerList,
                 symbolList, dependsOnList):
        self.name             = name
        self.type             = type
        self.releaseLib       = releaseLib
        self.debugLib         = debugLib
        self.releaseFramework = releaseFramework
        self.debugFramework   = debugFramework
        self.headerList       = headerList
        self.symbolList       = symbolList
        self.dependsOnList    = dependsOnList

#
# Create a table mapping canonical library names to descriptions of the library
#
libraryTable = {}

# header name to lists of canonical names of libraries
headerToLibraryTable = {}

# symbol name to lists of canonical names of libraries
symbolToLibraryTable = {}

def defineLibrary(lib):
    global libraryTable, headerToLibraryTable, symbolToLibraryTable

    if (libraryTable.has_key(lib.name)):
        colorPrint("ERROR: Library '" + lib.name + "' defined twice.", WARNING_COLOR)
        sys.exit(-1)

    libraryTable[lib.name] = lib

    for header in lib.headerList:
        if headerToLibraryTable.has_key(header):
            headerToLibraryTable[header] += [lib.name]
        else:
            headerToLibraryTable[header] = [lib.name]

    for symbol in lib.symbolList:
        if symbolToLibraryTable.has_key(symbol):
           symbolToLibraryTable[symbol] += [lib.name]
        else:
           symbolToLibraryTable[symbol] = [lib.name]


for lib in [
#       Canonical name  Type       Release    Debug      F.Release   F.Debug  Header List       Symbol list                                    Depends on
Library('SDL',         DYNAMIC,   'SDL',     'SDL',     'SDL',      'SDL',    ['SDL.h'],        ['SDL_GetMouseState'],                         ['OpenGL', 'Cocoa', 'pthread']),
Library('curses',      DYNAMIC,   'curses',  'curses',   None,       None,    ['curses.h'],     [],                                            []),
Library('zlib',        DYNAMIC,   'z',       'z',        None,       None,    ['zlib.h'],       ['compress2'],                                 []),
Library('glut',        DYNAMIC,   'glut',    'glut',     None,       None,    ['glut.h'],       [],                                            []),
Library('GLU',         DYNAMIC,   'GLU',     'GLU',      None,       None,    ['glu.h'],        ['gluBuild2DMipmaps'],                         ['OpenGL']),
Library('OpenGL',      DYNAMIC,   'GL',      'GL',      'OpenGL',   'OpenGL', ['gl.h'],         ['glBegin', 'glVertex3'],                      []),
Library('jpeg',        DYNAMIC,   'jpeg',    'jpeg',     None,       None,    ['jpeg.h'],       ['jpeg_memory_src', 'jpeg_CreateCompress'],    []),
Library('png',         DYNAMIC,   'png',     'png',      None,       None,    ['png.h'],        ['png_create_info_struct'],                    []),
Library('Cocoa',       FRAMEWORK,  None,      None,     'Cocoa',    'Cocoa',  ['Cocoa.h'],      ['DebugStr'],                                  []),
Library('Carbon',      FRAMEWORK,  None,      None,     'Carbon',   'Carbon', ['Carbon.h'],     ['ShowWindow'],                                []),
Library('G3D',         STATIC,    'G3D',     'G3Dd',     None,       None,    ['graphics3d.h'], [],                                            ['zlib', 'jpeg', 'png', 'Cocoa', 'pthread', 'Carbon']),
Library('GLG3D',       STATIC,    'GLG3D',   'GLG3Dd',   None,       None,    ['GLG3D.h', 'RenderDevice.h'],      [],                          ['G3D', 'SDL', 'OpenGL']),
Library('pthread',     DYNAMIC,   'pthread', 'pthread',  None,       None,    ['pthread.h'],    [],                                            []),
Library('QT',          DYNAMIC,   'qt-mt',   'qt-mt',    None,       None,    ['qobject.h'],    [],                                            []),
Library('X11',         DYNAMIC,   'X11',     'X11',      None,       None,    ['x11.h'],        ['XSync', 'XFlush'],                           [])
]:
    defineLibrary(lib)


""" Constructs a dictionary mapping a library name to its
    relative dependency order in a library list. """
def _makeLibOrder():
    # Make a set of partial order pairs from the
    # dependencies in the library table
    
    pairs = []
    
    for parent in libraryTable:
        for child in libraryTable[parent].dependsOnList:
            pairs.append( (parent, child) )

    pairs = [('GLG3D', 'G3D'), ('G3D', 'Cocoa'), ('Cocoa', 'SDL'), ('SDL', 'OpenGL'), ('GLU', 'OpenGL'), 
            ('GLU', 'GLG3D'), ('G3D', 'zlib'), ('G3D', 'png'), ('G3D', 'jpeg'), ('Cocoa', 'pthread'), 
            ('Cocoa', 'zlib'), ('OpenGL', 'png'), ('OpenGL', 'jpeg'), ('OpenGL', 'pthread'), ('Cocoa', 'Carbon')]

    E, V = pairsToVertexEdgeGraph(pairs)
    L = topSort(E, V)

    order = {}
    for i in xrange(0, len(L)):
        order[L[i]] = i

    return order

_libOrder = None

""" Sort predicate for library dependencies. """
def _libSorter(x, y):
    global _libOrder

    hasX = _libOrder.has_key(x)
    hasY = _libOrder.has_key(y)

    if hasX and hasY:
        return _libOrder[x] - _libOrder[y]

    elif hasX:
        # x is known, y is known.  Decide that x > y to put all unknown libraries first
        # since the known libraries can't depend on them (known libraries
        # have known dependencies)
       return 1

    elif hasY:

       return -1

    else:

       # Two libraries with no known dependencies; put them in alphabetical order
       # (since we have no other metric!) to guarantee a stable sort
       return cmp(x, y)


"""
Accepts a list of library canonical names and sorts it in place.
"""
def sortLibraries(liblist):
    global _libOrder
    _libOrder = _makeLibOrder()
    liblist.sort(_libSorter)
    _libOrder = None
    