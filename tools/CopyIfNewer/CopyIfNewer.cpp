#include <g3d/platform.h>
#include <graphics3d.h>

using namespace G3D;

void printHelp();
bool isDirectory(const std::string& filename);
/** Adds a slash to a directory, if not already there. */
std::string maybeAddSlash(const std::string& in);
int main(int argc, char** argv);
bool excluded(bool exclusions, bool superExclusions, const std::string& filename);


void copyIfNewer(
    bool exclusions, 
    bool superExclusions, 
    std::string sourcespec,
    std::string destspec) {

    if (G3D::isDirectory(sourcespec)) {
        // Copy an entire directory.  Change the arguments so that
        // we copy the *contents* of the directory.

        sourcespec = maybeAddSlash(sourcespec);
        sourcespec = sourcespec + "*";
    }

    std::string path = filenamePath(sourcespec);

    Array<std::string> fileArray;
    Array<std::string> dirArray;

    getDirs(sourcespec, dirArray);
    getFiles(sourcespec, fileArray);

    destspec = maybeAddSlash(destspec);

    if (fileExists(destspec) && ! G3D::isDirectory(destspec)) {
        printf("A file already exists named %s.  Target must be a directory.", 
            destspec.c_str());
        exit(-2);
    }
    createDirectory(destspec);

    for (int f = 0; f < fileArray.length(); ++f) {
        if (! excluded(exclusions, superExclusions, fileArray[f])) {
            std::string s = path + fileArray[f];
            std::string d = destspec + fileArray[f];
            if (fileIsNewer(s, d)) {
                printf("copy %s %s\n", s.c_str(), d.c_str());
                copyFile(s, d);
            }
        }
    }

    // Directories just get copied; we don't check their dates.
    // Recurse into the directories
    for (int d = 0; d < dirArray.length(); ++d) {
        if (! excluded(exclusions, superExclusions, dirArray[d])) {
            copyIfNewer(exclusions, superExclusions, path + dirArray[d], destspec + dirArray[d]);
        }
    }
}



int main(int argc, char** argv) {

    if (((argc == 2) && (std::string("--help") == argv[1])) || (argc < 3) || (argc > 4)) {
        printHelp();
        return -1;
    } else {
        bool exclusions = false;
        bool superExclusions = false;

        std::string s, d;
        if (std::string("--exclusions") == argv[1]) {
            exclusions = true;
            s = argv[2];
            d = argv[3];
        } else if (std::string("--super-exclusions") == argv[1]) {
            exclusions = true;
            superExclusions = true;
            s = argv[2];
            d = argv[3];
        } else {
            s = argv[1];
            d = argv[2];
        }

        copyIfNewer(exclusions, superExclusions, s, d);
    }
    
    return 0;
}


void printHelp() {
    printf("COPYIFNEWER\n\n");
    printf("SYNTAX:\n\n");
    printf(" copyifnewer [--help] [--exclusions] <source> <destdir>\n\n");
    printf("ARGUMENTS:\n\n");
    printf("  --exclusions  If specified, exclude CVS, svn, and ~ files. \n\n");
    printf("  --super-exclusions  If specified, exclude CVS, svn, ~, .ncb, .obj, .pyc, Release, Debug, build, temp files. \n\n");
    printf("  source   Filename or directory name (trailing slash not required).\n");
    printf("           May include standard Win32 wild cards in the filename.\n");
    printf("  dest     Destination directory, no wildcards allowed.\n\n");
    printf("PURPOSE:\n\n");
    printf("Copies files matching the source specification to the dest if they\n");
    printf("do not exist in dest or are out of date (according to the file system).\n\n");
    printf("Compiled: " __TIME__ " " __DATE__ "\n"); 
}


std::string maybeAddSlash(const std::string& sourcespec) {
    if (sourcespec.length() > 0) {
        char last = sourcespec[sourcespec.length() - 1];
        if ((last != '/') && (last != ':') && (last != '\\')) {
            // Add a trailing slash
            return sourcespec + "/";
        }
    }
    return sourcespec;
}

bool excluded(bool exclusions, bool superExclusions, const std::string& filename) {
    
    if (exclusions) {
        if (filename[filename.length() - 1] == '~') {
            return true;
        } else if ((filename == "CVS") || (filename == "svn") || (filename == ".cvsignore")) {
            return true;
        }
    }

    if (superExclusions) {
        std::string f = toLower(filename);
        if ((f == "release") || 
            (f == "debug") ||
            (f == "build") ||
            (f == "graveyard") ||
            (f == "temp") ||
            endsWith(f, ".pyc") ||
            endsWith(f, ".obj") ||
            endsWith(f, ".sbr") ||
            endsWith(f, ".ncb") ||
            endsWith(f, ".opt") ||
            endsWith(f, ".bsc") ||
            endsWith(f, ".pch") ||
            endsWith(f, ".ilk") ||
            endsWith(f, ".pdb")) {

            return true;
        }
    }

    return false;
}
