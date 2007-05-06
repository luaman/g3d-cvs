# templateHello.py
#

from utils import *
from variables import *

defaultMainCppContents = """
/** @file main.cpp
 */
#include <stdio.h>

int main(int argc, const char** argv) {
    printf("Hello World!\\n");
    return 0;
}
"""


""" Generates an empty project. """
def generateStarterFiles(state):
    mkdir('build')
    mkdir('source')
    mkdir('doc-files')

    if not isLibrary(state.binaryType):
        mkdir('data-files')
        writeFile('source/main.cpp', defaultMainCppContents)
    else:
        mkdir('include')
        incDir = pathConcat('include', state.projectName)
        mkdir(incDir)
        writeFile(pathConcat(incDir, state.projectName + '.h'), _headerContents(state))


""" Generates the contents of the master header file for a library """
def _headerContents(state):
    guard = state.projectName.upper() + '_H'
    return """
/** @file """ + state.projectName + """.h
 */

#ifndef """ + guard + """
#define """ + guard + """

#endif
"""
