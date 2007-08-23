#
# ice/deploy.py
#
# Creates distributable files from the build
#

import utils
import math
from variables import *
from utils import *
from copyifnewer import copyIfNewer
from library import libraryTable, FRAMEWORK

def _makePList(appname, binaryName):
    return """<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleDevelopmentRegion</key>
	<string>English</string>
	<key>CFBundleExecutable</key>
	<string>""" + appname + """</string>
	<key>CFBundleGetInfoString</key>
	<string>1.0</string>
	<key>CFBundleIconFile</key>
	<string>icon.icns</string>
	<key>CFBundleIdentifier</key>
	<string>""" + appname + """</string>
	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>
	<key>CFBundleName</key>
	<string>""" + appname + """</string>
	<key>CFBundlePackageType</key>
	<string>APPL</string>
	<key>CFBundleShortVersionString</key>
	<string>1.0.0</string>
	<key>CFBundleSignature</key>
        <string>""" + appname[0:4] + """</string>
	<key>CFBundleVersion</key>
	<string>1</string>
</dict>
</plist>"""


def _createApp(tmpDir, appDir, srcDir, state):
    contents   = appDir + 'Contents/'
    frameworks = contents + 'Frameworks/'
    resources  = contents + 'Resources/'
    macos      = contents + 'MacOS/'

    # Create directories
    mkdir(appDir,     utils.verbosity >= VERBOSE)
    mkdir(contents,   utils.verbosity >= VERBOSE)
    mkdir(frameworks, utils.verbosity >= VERBOSE)
    mkdir(resources,  utils.verbosity >= VERBOSE)
    mkdir(macos,      utils.verbosity >= VERBOSE)

    # Create Info.plist
    if utils.verbosity >= NORMAL: colorPrint('\nWriting Info.plist and PkgInfo', SECTION_COLOR)
    writeFile(contents + 'Info.plist', _makePList(state.projectName, state.binaryName))
    writeFile(contents + 'PkgInfo', 'APPL' + state.binaryName[0:4] + '\n') 

    # Copy binary
    if utils.verbosity >= NORMAL: colorPrint('\nCopying executable', SECTION_COLOR)
    shell('cp ' + state.binaryDir + state.binaryName + ' ' + macos + state.binaryName, utils.verbosity >= VERBOSE) 

    # Copy data-files to Resources
    if utils.verbosity >= NORMAL: colorPrint('\nCopying data files', SECTION_COLOR)
    copyIfNewer('data-files', resources, utils.verbosity >= VERBOSE, utils.verbosity == NORMAL)
    if utils.verbosity >= VERBOSE: colorPrint('Done copying data files', SECTION_COLOR)

    # Copy frameworks
    for libName in state.libList():
        if libraryTable.has_key(libName):
            lib = libraryTable[libName]
            if lib.deploy and (lib.type == FRAMEWORK):
                print 'Copying ' + lib.name + ' Framework'
                fwk = lib.releaseFramework + '.framework'
                src = '/Library/Frameworks/' + fwk
                copyIfNewer(src, frameworks + fwk, utils.verbosity >= VERBOSE, utils.verbosity == NORMAL)


def _createDmg(tmpDir, dmgName, projectName):
    shell('hdiutil create -fs HFS+ -srcfolder ' + tmpDir + ' ' + dmgName + ' -volname "' + projectName + '"', utils.verbosity >= VERBOSE)

def _deployOSX(state):
    print
    srcDir = state.binaryDir
    tmpDir = state.tempDir + 'deploy'
    appDir = tmpDir + '/' + state.projectName + '.app/'
    dmgName = state.buildDir + state.projectName + '.dmg'

    rmdir(tmpDir)
    _createApp(tmpDir + '/', appDir, srcDir, state)
    _createDmg(tmpDir, dmgName, state.projectName)
    
    if utils.verbosity >= VERBOSE:
        print
    print 'Deployable archive written to ' + dmgName


def _deployUnix(state):
    print
    tarfile = state.buildDir + state.projectName + '.tar'
    shell('tar -cf ' + tarfile +' ' + state.installDir + '*', utils.verbosity >= VERBOSE)
    shell('gzip -f ' + tarfile, utils.verbosity >= VERBOSE)
    rm(tarfile, utils.verbosity >= VERBOSE)
    if utils.verbosity >= VERBOSE:
        print
    print 'Deployable archive written to ' + state.buildDir + tarfile + '.gz'

#
# Create a deployment file
#
def deploy(state):
    maybeColorPrint('Building deployment', SECTION_COLOR)
    if (os.uname()[0] == "Darwin"):
        _deployOSX(state)
    else:
        _deployUnix(state)
        
    
