# doxygen.py
#
# Doxygen Management

from utils import *

##############################################################################
#                            Doxygen Management                              #
##############################################################################

"""
 Called from buildDocumentation.
"""
def createDoxyfile(state):
    # Create the template, surpressing Doxygen's usual output
    shell("doxygen -g Doxyfile > /dev/null")

    # Edit it
    f = open('Doxyfile', 'r+')
    text = f.read()

    # TODO: excludes
    propertyMapping = {
    'PROJECT_NAME'            : '"' + state.projectName.capitalize() + '"',
    'OUTPUT_DIRECTORY'        : '"' + pathConcat(state.buildDir, 'doc') + '"',
    'EXTRACT_ALL'             : "YES",
    'STRIP_FROM_PATH'         : '"' + state.rootDir + '"',
    'TAB_SIZE'                : "4",
    'QUIET'                   : 'YES',
    'WARN_IF_UNDOCUMENTED'    : 'NO',
    'WARN_NO_PARAMDOC'        : 'NO',
    'HTML_OUTPUT'             : '"./"',
    'GENERATE_LATEX'          : 'NO',
    'RECURSIVE'               : 'YES',
    'SORT_BRIEF_DOCS'         : 'YES',
    'JAVADOC_AUTOBRIEF'       : 'YES',
    'EXCLUDE'                 : 'build graveyard temp doc-files data-files',
    "ALIASES"                 : ('"cite=\par Referenced Code:\\n " ' +
                                 '"created=\par Created:\\n" ' +
                                 '"edited=\par Last modified:\\n" ' + 
                                 '"maintainer=\par Maintainer:\\n" ' +
                                 '"units=\par Units:\\n"')
    }

    # Rewrite the text by replacing any of the above properties
    newText = ""
    for line in string.split(text,"\n"):
        newText += (doxyLineRewriter(line, propertyMapping) + "\n")

    # Write the file back out
    f.seek(0)
    f.write(newText)
    f.close()

#########################################################################

""" Called from createDoxyfile. """
def doxyLineRewriter(lineStr, hash):
    line = string.strip(lineStr) # remove leading and trailing whitespace
    if (line == ''): # it's a blank line
        return lineStr
    elif (line[0] == '#'): # it's a comment line
        return lineStr
    else : # here we know it's a property assignment
        prop = string.strip(line[0:string.find(line, "=")])
        if hash.has_key(prop):
            print prop + ' = ' + hash[prop]
            return prop + ' = ' + hash[prop]
        else:
            return lineStr

""" Expands environment variables of the form $(var) and $(shell ...)
    in a string. Compare to os.path.expandvars, which only handles
    $var and therefore requires a separation character after
    variables."""
def expandvars(str):
    
    while '$' in str:
        i = str.find('$')
        j = str.find(')', i)
        if (i > len(str) - 3) or (str[i + 1] != '(') or (j < 0):
            raise 'Error', 'Environment variables must have the form $(var) or $(shell cmds)'

        varexpr = str[i:(j + 1)]

        varname = str[(i + 2):j]

        if varname.startswith('shell '):
            # Shell command
            cmd = varname[6:]
            value = shell(cmd, verbose)
            if verbose: print value
        else:
            # Environment variable
            value = os.getenv(varname)

        if (value == None):
            value = ''
        
        str = str.replace(varexpr, value)

    return str
        
refDict = {}
currentFile = ''

def recapLetter(match):
    return match.group(0)[1].upper()

def replaceRef(match):
    if match.group(2) in refDict:
        return 'href="' + refDict[match.group(2)] + '">' + match.group(2) + match.group(3)
    elif match.group(2) == 'Ref':
        return 'href="' + currentFile + '">' + match.group(2) + match.group(3)        
    else:
        return match.group(0)
        
    return returnStr

def parseDoxygenFilename(filename):
    refNames = []

    #ignore class names with spaces
    if re.search('301|014', filename): 
        return refNames

    #split into namespaces
    fullname = re.split('class|struct', re.split('\-', re.split('.html', filename)[0])[0], 1)[1]
    namespaces = re.split('_1_1', fullname)
    
    #reconstruct capitalization
    correctNames = []
    for name in namespaces:
        name = re.sub('_(?<=_)\w', recapLetter, name)
        correctNames.append(name)

    memberName = ''
    for str in correctNames[1:]:
        memberName += str + '::'
        
    memberName += 'Ref'
    refNames.append({memberName: filename})
    
    refNames.append({correctNames[-1] + 'Ref': filename})
    return refNames

def remapRefLinks(docDir):

    possibleRefs = {}

    docFiles = os.listdir(docDir)
    
    validFiles = []
    
    for docFile in docFiles:
        if re.search('^class_g3_d|^struct_g3_d', docFile) and not re.search('-members.html$', docFile):
            validFiles.append(docFile)
            refs = parseDoxygenFilename(docFile)
            for ref in refs:
                possibleRefs.update(ref)

    refDict.clear()
    refDict.update(possibleRefs)

    for docFile in validFiles[20:]:
        currentFile = docFile
        fileBuffer = ''
        f = open(docDir + '/' + docFile)
        try:
            for line in f:
                fileBuffer += re.sub('(href="class_g3_d_1_1_reference_counted_pointer.html">)([a-zA-Z0-9:]+)(</a>)', replaceRef, line)
        finally:
            f.close()
            
        f = open(docDir + '/' + docFile, 'w')
        try:
            f.write(fileBuffer)
        finally:
            f.close()