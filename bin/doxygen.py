# doxygen.py
#
# Doxygen Management

from mpyutils import *

##############################################################################
#                            Doxygen Management                              #
##############################################################################

"""
 Called from buildDocumentation.
"""
def createDoxyfile():
    # Create the template, surpressing Doxygen's usual output
    shell("doxygen -g Doxyfile > /dev/null")

    # Edit it
    f = open("Doxyfile", "r+")
    text = f.read()

    propertyMapping = {
    "PROJECT_NAME"            : '"' + projectName.capitalize() + '"',
    "OUTPUT_DIRECTORY"        : '"' + BUILDDIR + '/doc"',
    "EXTRACT_ALL"             : "YES",
    "STRIP_FROM_PATH"         : '"' + rootDir + '"',
    "TAB_SIZE"                : "4",
    "HTML_OUTPUT"             : '"./"',
    "GENERATE_LATEX"          : "NO",
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
    if (line == ""): # it's a blank line
        return lineStr
    elif (line[0] == "#"): # it's a comment line
        return lineStr
    else : # here we know it's a property assignment
        prop = string.strip(line[0:string.find(line, "=")])
        if hash.has_key(prop):
            return prop + " = " + hash[prop]
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

        if beginswith(varname, 'shell '):
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
        
