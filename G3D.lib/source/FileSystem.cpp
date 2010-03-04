/**
 @file FileSystem.cpp
 
 @author Morgan McGuire, http://graphics.cs.williams.edu
 
 @author  2002-06-06
 @edited  2010-03-05
 */
#include "G3D/FileSystem.h"
#include "G3D/System.h"
#include "G3D/stringutils.h"
#include "G3D/fileutils.h"
#include <sys/stat.h>
#include <sys/types.h>
#include "zip.h"
#include "G3D/g3dfnmatch.h"
#include "G3D/BinaryInput.h"
#include "G3D/BinaryOutput.h"

#ifdef G3D_WIN32
    // Needed for _getcwd
#   include <direct.h>

    // Needed for _findfirst
#   include <io.h>

#define stat64 _stat64
#else
#   include <dirent.h>
#   include <fnmatch.h>
#   include <unistd.h>
#   define _getcwd getcwd
#   define _stat stat
#endif

namespace G3D {

static FileSystem* common = NULL;

FileSystem& FileSystem::instance() {
    init();
    return *common;
}


void FileSystem::init() {
    if (common == NULL) {
        common = new FileSystem();
    }
}


void FileSystem::cleanup() {
    if (common != NULL) {
        delete common;
        common = NULL;
    }
}

FileSystem::FileSystem() : m_cacheLifetime(10) {}

/////////////////////////////////////////////////////////////

void FileSystem::Dir::computeZipListing(const std::string& zipfile, const std::string& pathInsideZipfile) {
    struct zip* z = zip_open( zipfile.c_str(), ZIP_CHECKCONS, NULL );
    debugAssert(z);

    int count = zip_get_num_files( z );
    for (int i = 0; i < count; ++i) {
        struct zip_stat info;
        zip_stat_init( &info );    // TODO: Docs unclear if zip_stat_init is required.
        zip_stat_index( z, i, ZIP_FL_NOCASE, &info );
        
        // Fully-qualified name of a file inside zipfile
        std::string name = info.name;

        Set<std::string> alreadyAdded;
        if (beginsWith(name, pathInsideZipfile)) {
            // We found something inside the directory we were looking for,
            // so the directory itself must exist                        
            exists = true;

            // For building the cached directory listing, extract only elements that do not contain 
            // additional subdirectories.

            // TODO: check that the +1 character offset is correct in this code.
            int start = pathInsideZipfile.size() + 1;
            int end = findSlash(name, start);
            if (end == -1) {
                // There are no more slashes; add this name
                name = name.substr(start);
                if (alreadyAdded.insert(name)) {
                    Entry& e = nodeArray.next();
                    e.name = name;
                    e.type = FILE_TYPE;
                }
            } else {
                // There are more slashes, indicating that this is a directory
                name = name.substr(start, end);
                if (alreadyAdded.insert(name)) {
                    Entry& e = nodeArray.next();
                    e.name = name;
                    e.type = DIR_TYPE;
                }
            }
        }
    }
    
    zip_close(z);
    z = NULL;
}


const FileSystem::Dir& FileSystem::getContents(const std::string& path, bool forceUpdate) {
    const std::string& key = 
#   if defined(G3D_WIN32)
        toLower(path);
#   else
        path;
#   endif
    
    RealTime now = System::time();
    Dir& dir = m_cache.getCreate(key);

    if ((now > dir.lastChecked + cacheLifetime()) || forceUpdate) {
        dir = Dir();

        // Out of date: update
        dir.lastChecked = now;

        struct _stat st;
        const bool exists = _stat(path.c_str(), &st) != -1;
        const bool isDirectory = (st.st_mode & S_IFDIR) != 0;

        // Does this path exist on the real filesystem?
        if (exists && isDirectory) {

            // Is this path actually a directory?
            if (isDirectory) {
                // Is this path to a zipfile?
                dir.exists = true;

                // Update contents
#               ifdef G3D_WIN32
                    const std::string& filespec = FilePath::concat(path, "*");
                    struct _finddata_t fileinfo;
                    intptr_t handle = _findfirst(filespec.c_str(), &fileinfo);
                    debugAssert(handle != -1);
                    int result = 0;
                    do {
                        Entry& e = dir.nodeArray.next();
                        e.name = fileinfo.name;
                        if ((fileinfo.attrib & _A_SUBDIR) != 0) {
                            e.type = DIR_TYPE;
                        } else {
                            e.type = FILE_TYPE;
                        }

                        result = _findnext(handle, &fileinfo);
                    } while (result == 0);
                    _findclose(handle);

#               else
                    DIR* listing = opendir(path.c_str());
                    struct dirent* entry = readdir(listing);
                    while (entry != NULL) {
                        if (! strcmp(entry->d_name, "..") &&
                            ! strcmp(entry->d_name, ".")) {

                            dir.nodeArray.append(toLower(entry->d_name));
                        }
                        entry = readdir(listing);
                    }
                    closedir(listing);
                    listing = NULL;
                    entry = NULL;
#               endif
            }

        } else {
            std::string zip;

            if (exists && isZipfile(path)) {
                // This is a zipfile; get its root
                dir.isZipfile = true;                
                dir.computeZipListing(path, "");

            } else if (inZipfile(path, zip)) {

                // There is a zipfile somewhere in the path.  Does
                // the rest of the path exist inside the zipfile?
                dir.inZipfile = true;

                dir.computeZipListing(zip, path.substr(zip.length() + 1));
            }
        }        
    }

    return dir;
}


bool FileSystem::inZipfile(const std::string& path, std::string& z) {
    // Reject trivial cases before parsing
    if (path.find('.') == std::string::npos) {
        // There is no zipfile possible, since G3D requires
        // an extension on zipfiles.
        return false;
    }

    // Look at all sub-paths containing periods.
    // For each, ask if it is a zipfile.
    int current = 0;
    current = path.find('.', current);

    while (current != -1) {
        // xxxxx/foo.zip/yyyyy
        current = path.find('.', current);

        // Look forward for the next slash
        int s = findSlash(path, current);

        z = path.substr(0, s); 
        if (isZipfile(z)) {
            return true;
        }
        
        current = s + 1;
    }

    z = "";
    return false;

}


bool FileSystem::isZipfile(const std::string& filename) {
    if (FilePath::ext(filename).empty()) {
        return false;
    }
    
	FILE* f = fopen(filename.c_str(), "r");
	if (f == NULL) {
		return false;
	}
	uint8 header[4];
	fread(header, 4, 1, f);
	
	const uint8 zipHeader[4] = {0x50, 0x4b, 0x03, 0x04};
	for (int i = 0; i < 4; ++i) {
		if (header[i] != zipHeader[i]) {
			fclose(f);
			return false;
		}
	}

	fclose(f);
	return true;
}



void FileSystem::flushCache() {
    m_cache.clear();
}


void FileSystem::setCacheLifetime(float t) {
    m_cacheLifetime = t;
}


void FileSystem::createDirectory(const std::string& dir) {
    
	if (dir == "") {
		return;
	}

    std::string d;

    // Add a trailing / if there isn't one.
    switch (dir[dir.size() - 1]) {
    case '/':
    case '\\':
        d = dir;
        break;

    default:
        d = dir + "/";
    }

    // If it already exists, do nothing
    if (exists(d.substr(0, d.size() - 1))) {
        return;
    }

    // Parse the name apart
    std::string root, base, ext;
    Array<std::string> path;

    std::string lead;
    FilePath::parse(d, root, path, base, ext);
    debugAssert(base == "");
    debugAssert(ext == "");

    // Begin with an extra period so "c:\" becomes "c:\.\" after
    // appending a path and "c:" becomes "c:.\", not root: "c:\"
    std::string p = root + ".";

    // Create any intermediate that doesn't exist
    for (int i = 0; i < path.size(); ++i) {
        p += "/" + path[i];
        if (! exists(p)) {
            // Windows only requires one argument to mkdir,
            // where as unix also requires the permissions.
#           ifndef G3D_WIN32
                mkdir(p.c_str(), 0777);
#	        else
                _mkdir(p.c_str());
#	        endif
        }
    }

    flushCache();
}


void FileSystem::copyFile(const std::string& source, const std::string& dest) {
#   ifdef G3D_WIN32
        // TODO: handle case where srcPath is in a zipfile
        CopyFileA(source.c_str(), dest.c_str(), FALSE);
#   else
        // Read it all in, then dump it out
        BinaryInput  in(source, G3D_LITTLE_ENDIAN);
        BinaryOutput out(dest, G3D_LITTLE_ENDIAN);
        out.writeBytes(in.getCArray(), in.size());
        out.commit(false);
#   endif
        
    flushCache();
}


bool FileSystem::exists(const std::string& f, bool trustCache) {

    std::string path = FilePath::removeTrailingSlash(f);
    std::string parentPath = FilePath::parentPath(path);

    const Dir& entry = getContents(parentPath, ! trustCache);

    if (entry.exists) {
        // See if the entry contains f.
        // TODO
    }

    return false;
}


bool FileSystem::isDirectory(const std::string& filename) {
    // TODO: work with zipfiles and cache
    struct _stat st;
    const bool exists = _stat(FilePath::removeTrailingSlash(filename).c_str(), &st) != -1;
    return exists && ((st.st_mode & S_IFDIR) != 0);
}


std::string FileSystem::resolve(const std::string& filename) {
    if (filename.size() >= 1) {
        if (isSlash(filename[0])) {
            // Already resolved
            return filename;
        } else {

            #ifdef G3D_WIN32
                if ((filename.size() >= 2) && (filename[1] == ':')) {
                    // There is a drive spec on the front.
                    if ((filename.size() >= 3) && isSlash(filename[2])) {
                        // Already fully qualified
                        return filename;
                    } else {
                        // The drive spec is relative to the
                        // working directory on that drive.
                        debugAssertM(false, "Files of the form d:path are"
                                     " not supported (use a fully qualified"
                                     " name).");
                        return filename;
                    }
                }
            #endif
        }
    }

    // Prepend the working directory.
    return FilePath::concat(currentDirectory(), filename);
}


std::string FileSystem::currentDirectory() {
    static const int N = 2048;
    char buffer[N];

    _getcwd(buffer, N);
    return std::string(buffer);
}


bool FileSystem::isNewer(const std::string& src, const std::string& dst) {
    // TODO: work with cache and zipfiles
    struct _stat sts;
    bool sexists = _stat(src.c_str(), &sts) != -1;

    struct _stat dts;
    bool dexists = _stat(dst.c_str(), &dts) != -1;

    return sexists && ((! dexists) || (sts.st_mtime > dts.st_mtime));
}


int64 FileSystem::size(const std::string& filename) {
    struct stat64 st;
    int result = stat64(filename.c_str(), &st);
    
    if (result == -1) {
		std::string zip, contents;
		if (zipfileExists(filename, zip, contents)) {
			int64 requiredMem;

            struct zip *z = zip_open( zip.c_str(), ZIP_CHECKCONS, NULL );
            debugAssertM(z != NULL, zip + ": zip open failed.");
			{
                struct zip_stat info;
                zip_stat_init( &info );    // Docs unclear if zip_stat_init is required.
                int success = zip_stat( z, contents.c_str(), ZIP_FL_NOCASE, &info );
                debugAssertM(success == 0, zip + ": " + contents + ": zip stat failed.");
                requiredMem = info.size;
			}
            zip_close(z);
			return requiredMem;
		} else {
            return -1;
		}
    }

    return st.st_size;
}


void FileSystem::list(const std::string& spec, Array<std::string>& result, bool files, bool directories, bool includeParentPath) {
    // TODO
}



#ifdef G3D_WIN32
const Array<std::string>& FileSystem::drives() {
    if (m_winDrive.length() == 0) {
        // See http://msdn.microsoft.com/en-us/library/aa364975(VS.85).aspx
        static const size_t bufSize = 5000;
        char bufData[bufSize];
        GetLogicalDriveStringsA(bufSize, bufData);

        // Drive list is a series of NULL-terminated strings, itself terminated with a NULL.
        for (int i = 0; bufData[i] != '\0'; ++i) {
            const char* thisString = bufData + i;
            m_winDrive.append(thisString);
            i += strlen(thisString) + 1;
        }
    }

    return m_winDrive;
}
#endif

/////////////////////////////////////////////////////////////////////

bool FilePath::isRoot(const std::string& f) {
#   ifdef G3D_WIN32
        if (f.length() < 2) {
            return false;
        }

        if (f[1] == ':') {
            if (f.length() == 2) {
                // e.g.,   "x:"
                return true;
            } else if ((f.length() == 3) && isSlash(f[2])) {
                // e.g.,   "x:\"
                return true;
            }
        }

        if (isSlash(f[0]) && isSlash(f[1])) {        
            // e.g., "\\foo\"
            return true;
        }
#   else
        if (f == "/") {
            return true;
        }
#   endif

    return false;
}


std::string FilePath::removeTrailingSlash(const std::string& f) {
    bool removeSlash = ((endsWith(f, "/") || endsWith(f, "\\"))) && ! isRoot(f);

    return removeSlash ? f.substr(0, f.length() - 1) : f;
}


std::string FilePath::concat(const std::string& dirname, const std::string& file) {
    // Ensure that the directory ends in a slash
    if (! dirname.empty() && 
        ! isSlash(dirname[dirname.size() - 1]) &&
        (dirname[dirname.size() - 1] != ':')) {
        return dirname + '/' + file;
    } else {
        return dirname + file;
    }
}


std::string FilePath::ext(const std::string& filename) {
    int i = filename.rfind(".");
    if (i >= 0) {
        return filename.substr(i + 1, filename.length() - i);
    } else {
        return "";
    }
}


std::string FilePath::baseExt(const std::string& filename) {
    int i = findLastSlash(filename);

#   ifdef G3D_WIN32
        int j = filename.rfind(":");
        if ((i == -1) && (j >= 0)) {
            i = j;
        }
#   endif

    if (i == -1) {
        return filename;
    } else {
        return filename.substr(i + 1, filename.length() - i);
    }
}


std::string FilePath::base(const std::string& path) {
    std::string filename = baseExt(path);
    int i = filename.rfind(".");
    if (i == -1) {
        // No extension
        return filename;
    } else {
        return filename.substr(0, i - 1);
    }
}


std::string FilePath::parentPath(const std::string& path) {
    int i = findLastSlash(path);

#   ifdef G3D_WIN32
        int j = path.rfind(":");
        if ((i == -1) && (j >= 0)) {
            i = j;
        }
#   endif

    if (i == -1) {
        return "";
    } else {
        return path.substr(0, i + 1);
    }
}


bool FilePath::containsWildcards(const std::string& filename) {
    return (filename.find('*') != std::string::npos) || (filename.find('?') != std::string::npos);
}


bool FilePath::matches(const std::string& path, const std::string& pattern, int flags) {
    return g3dfnmatch(path.c_str(), pattern.c_str(), flags) == 0;
}


void FilePath::parse
(const std::string&     filename,
 std::string&           root,
 Array<std::string>&    path,
 std::string&           base,
 std::string&           ext) {

    std::string f = filename;

    root = "";
    path.clear();
    base = "";
    ext = "";

    if (f == "") {
        // Empty filename
        return;
    }

    // See if there is a root/drive spec.
    if ((f.size() >= 2) && (f[1] == ':')) {
        
        if ((f.size() > 2) && isSlash(f[2])) {
        
            // e.g.  c:\foo
            root = f.substr(0, 3);
            f = f.substr(3, f.size() - 3);
        
        } else {
        
            // e.g.  c:foo
            root = f.substr(2);
            f = f.substr(2, f.size() - 2);

        }

    } else if ((f.size() >= 2) & isSlash(f[0]) && isSlash(f[1])) {
        
        // e.g. //foo
        root = f.substr(0, 2);
        f = f.substr(2, f.size() - 2);

    } else if (isSlash(f[0])) {
        
        root = f.substr(0, 1);
        f = f.substr(1, f.size() - 1);

    }

    // Pull the extension off
    {
        // Find the period
        size_t i = f.rfind('.');

        if (i != std::string::npos) {
            // Make sure it is after the last slash!
            size_t j = iMax(f.rfind('/'), f.rfind('\\'));
            if ((j == std::string::npos) || (i > j)) {
                ext = f.substr(i + 1, f.size() - i - 1);
                f = f.substr(0, i);
            }
        }
    }

    // Pull the basename off
    {
        // Find the last slash
        size_t i = iMax(f.rfind('/'), f.rfind('\\'));
        
        if (i == std::string::npos) {
            
            // There is no slash; the basename is the whole thing
            base = f;
            f    = "";

        } else if ((i != std::string::npos) && (i < f.size() - 1)) {
            
            base = f.substr(i + 1, f.size() - i - 1);
            f    = f.substr(0, i);

        }
    }

    // Parse what remains into path.
    size_t prev, cur = 0;

    while (cur < f.size()) {
        prev = cur;
        
        // Allow either slash
        size_t i = f.find('/', prev + 1);
        size_t j = f.find('\\', prev + 1);
        if (i == std::string::npos) {
            i = f.size();
        }

        if (j == std::string::npos) {
            j = f.size();
        }

        cur = iMin(i, j);

        if (cur == std::string::npos) {
            cur = f.size();
        }

        path.append(f.substr(prev, cur - prev));
        ++cur;
    }
}

}
