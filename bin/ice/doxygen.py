# doxygen.py
#
# Doxygen Management

from utils import *
import glob

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



class DoxygenRefLinkRemapper:

    # Given the output of a Doxygen directory, rewrites the output .html
    # files so that G3D::ReferenceCountedPointer<X> instances link to X
    # instead of ReferenceCountedPointer.
    #
    # The current implementation only works for the G3D build itself.
    # It is intended to be expanded in the future to support projects
    # built against G3D.
    def remap(self, sourcePath, remapPath):
        self.__buildValidRefs(sourcePath)
        self.__remapRefLinks(remapPath)

    def __buildValidRefs(self, sourcePath):
        # initialize the ref name mapping
        self.validRefs = {}
        
        # build list of valid source files
        sources = os.listdir(sourcePath)
        
        # discard non-class/struct documentation files
        sources = filter(lambda filename: re.search('^class|^struct', filename), sources)
        sources = filter(lambda filename: not re.search('-members.html$', filename), sources)
        
        # discard filenames with encoded spaces (implies templates) for now
        sources = filter(lambda filename: not re.search('_01', filename), sources)
        
        # build the dictionary mapping valid ref names to their documentation
        for filename in sources:
            memberRefName, nonmemberRefName = self.__buildRefNames(filename)
            self.validRefs.update({memberRefName:filename, nonmemberRefName:filename})
        
        
    def __buildRefNames(self, filename):
        # build list of qualified scopes from filename
        capitalizedScopes = self.__buildScopes(filename)
        
        # build the qualified name used as prefix
        qualifiedPrefix = ''
        for scope in capitalizedScopes:
            qualifiedPrefix += scope + '::'

        # build the member typedef ref name (e.g., G3D::Class::Ref)
        memberRefName = qualifiedPrefix + 'Ref'
                    
        # build the non-member ref name (e.g., G3D::ClassRef)
        nonmemberRefName = qualifiedPrefix[:-2] + 'Ref'
        
        return memberRefName, nonmemberRefName

    def __buildScopes(self, filename):
        # remove the file type ('class', 'struct', '.html') and separate the scopes ('::')
        sansType = re.split('class|struct', filename)[1]
        sansType = re.split('.html', sansType)[0]
        
        rawScopes = re.split('_1_1', sansType)
        
        # re-capitalize letters
        capitalizedScopes = []
        for scope in rawScopes:
            scope = re.sub('_(?<=_)\w', lambda match: match.group(0)[1].upper(), scope)
            capitalizedScopes.append(scope)

        return capitalizedScopes        

    def __remapRefLinks(self, remapPath):
        # initialize the current remapping filename
        self.currentRemapFilename = ''
        
        # loop through all valid html/documentation files in the remap path
        for filename in glob.glob(os.path.join(os.path.normcase(remapPath), '*.html')):
            self.currentRemapFilename = filename
            
            # will hold updated file contents
            remappedBuffer = ''
            
            # read each line in file and replace any matched ref links
            f = open(filename)
            try:
                for line in f:
                    remappedBuffer += re.sub('(href="class_g3_d_1_1_reference_counted_pointer.html">)([a-zA-Z0-9:]+)(</a>)', self.__linkMatchCallack, line)
            finally:
                f.close()
            
            #assume lines were read and remapped correctly, write new documentation
            writeFile(filename, remappedBuffer)
            
    def __linkMatchCallack(self, match):
        # if ref search fails, build the fully qualified ref name that we can search the dictionary for
        # e.g., SuperShader::Pass::Ref would be qualified as G3D::SuperShader::Pass::Ref
        # ref links found in non-struct/class files will be matched as-is
        
        # note: this would have to be redone if multiple source directories is implemented
        
        if match.group(2) in self.validRefs:
            return 'href="' + self.validRefs[match.group(2)] + '">' + match.group(2) + match.group(3)
        elif re.search('class|struct', self.currentRemapFilename):
            # get list of scopes from current filename
            qualifiedScopes = self.__buildScopes(self.currentRemapFilename)
            
            # build a prefix including all of the scopes except for current one (should be class/struct)
            for numScopes in range(0, len(qualifiedScopes)):
                qualifiedPrefix = ''
                for scope in qualifiedScopes[:-numScopes]:
                    qualifiedPrefix += scope + '::'
                
                qualifiedRef = qualifiedPrefix + match.group(2)
                
                if qualifiedRef in self.validRefs:
                    return 'href="' + self.validRefs[qualifiedRef] + '">' + match.group(2) + match.group(3)

        return match.group(0)
