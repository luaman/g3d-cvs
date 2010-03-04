#ifndef G3D_FileSystem_h
#define G3D_FileSystem_h

#include "G3D/platform.h"
#include "G3D/Array.h"
#include "G3D/Table.h"

namespace G3D {

/** 
 OS-independent file system layer that optimizes the performance
 of queries by caching and prefetching.

 This class uses the following definitions:
 <ul>
   <li> "file" = document that can be opened for reading or writing
   <li> "directory" = folder containing files and other directories
   <li> "node" = file or directory
   <li> "path" = string identifying a (see the FSPath class)
   <li> "zipfile" = a compressed file storing an archive of files and directories in the zip format
 </ul>

 In G3D, Zipfiles are transparently treated as if they were directories, provided:
 <ul>
   <li> The zipfile name contains an extension (e.g., map.pk3, files.zip)
   <li> There are no nested zipfiles
 </ul>

 The extension requirement allows G3D to quickly identify whether a path could enter a
 zipfile without forcing it to open all parent directories for reading.
*/
class FileSystem {
private:

    /** Drive letters.  Only used on windows, but defined on all platforms to help
       avoid breaking the Windows build when compiling on another platform. */
    Array<std::string>          m_winDrive;

    float                       m_cacheLifetime;

    enum Type {
        /** Not yet checked */
        UNKNOWN, 
        FILE_TYPE, 
        DIR_TYPE
    };

    class Entry {
    public: 
        /** Name, not including parent path */
        std::string             name;
        Type                    type;
        Entry() : type(UNKNOWN) {}
        Entry(const char* n) : name(n), type(UNKNOWN) {}
    };

    class Dir {
    public:
        
        /** If false, this path did not exist (even inside a zipfile) when last checked, or it is not a directory. */
        bool                    exists;

        bool                    isZipfile;
        bool                    inZipfile;

        /** Files and directories */
        Array<Entry>            nodeArray;
        
        /** When this entry was last updated */
        double                  lastChecked;

        /** Compute the contents of nodeArray from this zipfile. */
        void computeZipListing(const std::string& zipfile, const std::string& pathInsideZipfile);

        Dir() : lastChecked(0), exists(false), isZipfile(false), inZipfile(false) {}
    };

    /** Maps path names (without trailing slashes, except for the file system root) to contents.
        On Windows, all paths are lowercase */
    Table<std::string, Dir>     m_cache;

    /** Update the cache entry for path if it is not already present.
     \param forceUpdate If true, always override the current cache value.*/
    const Dir& getContents(const std::string& path, bool forceUpdate);

    /** Don't allow public construction. */
    FileSystem();

    /** Returns true if some sub-path of this one is a zipfile. 
       If the path itself is a zipfile, returns false.*/
    bool inZipfile(const std::string& path, std::string& zipsubpath);

public:

#   ifdef G3D_WIN32
    /** On Windows, the drive letters that form the file system roots.*/
    const Array<std::string>& drives();
#   endif

    /** Flushes the cache */
    void flushCache();

    /** Returns true if \a path is a file that is a zipfile. Note that G3D requires zipfiles to have
        some extension, although it is not required to be "zip" */
    bool isZipfile(const std::string& path);

    /** Set the cacheLifetime().
       \param t in seconds */
    void setCacheLifetime(float t);

    /** A cache is used to optimize repeated calls.  A cache entry is considered
        valid for this many seconds after it has been checked. */
    float cacheLifetime() const {
        return m_cacheLifetime;
    }

    /** Creates the directory named, including any subdirectories 
        that do not already exist.

        The directory must not be inside a zipfile.

        Flushes the cache.
     */
    void createDirectory(const std::string& path);

    /** The current working directory (cwd).  Only ends in a slash if this is the root of the file system. */
    std::string currentDirectory();

    /**
    \param srcPath Must name a file.
    \param dstPath Must not contain a zipfile.

    Flushes the cache.
    */
    void copyFile(const std::string& srcPath, const std::string& dstPath);

    /** Returns true if a node named \a f exists.

        \param trustCache If true, uses the cache for optimizing repeated calls 
        in the same parent directory. 
     */
    bool exists(const std::string& f, bool trustCache = true);

    /** Known bug: does not work inside zipfiles */
    bool isDirectory(const std::string& path);

    /** Known bug: does not work inside zipfiles */
    bool isFile(const std::string& path) {
        return ! isDirectory(path);
    }
    
    /** Fully qualifies a filename.

        The filename may contain wildcards, in which case the wildcards will be preserved in the returned value.
    */
    std::string resolve(const std::string& path);

    /** Returns true if \param dst does not exist or \param src is newer than \param dst,
       according to their time stamps.
       
       Known bug: does not work inside zipfiles.
       */
    bool isNewer(const std::string& src, const std::string& dst);

    /** Returns the length of the file in bytes, or -1 if the file could not be opened. */
    int64 size(const std::string& path);

    /** Appends all nodes matching \a spec to the \a result array.

      Wildcards can only appear to the right of the last slash in \a spec.

      The names will not contain parent paths unless \a includePath == true. 
      These may be relative to the current directory unless \a spec 
      is fully qualified (can be done with resolveFilename). 
      
     */
    void list(const std::string& spec, Array<std::string>& result,
        bool files = true, bool directories = true, bool includeParentPath = false);

    /** list() files */
    void getFiles(const std::string& spec, Array<std::string>& result, bool includeParentPath = false) {
        list(spec, result, true, false, includeParentPath);
    }

    /** list() directories */
    void getDirectories(const std::string& spec, Array<std::string>& result, bool includeParentPath = false) {
        list(spec, result, false, true, includeParentPath);
    }

    static FileSystem& instance();

    /** Create the common instance. */
    static void init();

    /** Destroy the common instance. */
    static void cleanup();
};


/** \brief Parsing of file system paths.  

    None of these routines touch the disk--they are purely string manipulation.

    In "/a/b/base.ext",

    <ul>
      <li> base = "base"
      <li> ext = "ext"
      <li> parentPath = "/a/b"
      <li> baseExt = "base.ext"
    </ul>

*/
class FilePath {
public:

    /** Appends file onto dirname, ensuring a / if needed. */
    static std::string concat(const std::string& a, const std::string& b);

    static bool isRoot(const std::string& f);

    /** Removes the trailing slash unless \a f is a filesystem root */
    static std::string removeTrailingSlash(const std::string& f);

    /** Returns everything to the right of the last '.' */
    static std::string ext(const std::string& path);

    /** Returns everything to the right of the last slash (or, on Windows, the last ':') */
    static std::string baseExt(const std::string& path);

    /** Returns everything between the right-most slash and the following '.' */
    static std::string base(const std::string& path);

    /** Returns everything to the left of the right-most slash */
    static std::string parentPath(const std::string& path);

    /** Returns true if '*' or '?' appear in the filename */
    static bool containsWildcards(const std::string& p);

    /**
      Parses a filename into four useful pieces.

      Examples:

      c:\\a\\b\\d.e   
        root  = "c:\\"
        path  = "a" "b"
        base  = "d"
        ext   = "e"
     
      /a/b/d.e
        root = "/"
        path  = "a" "b"
        base  = "d"
        ext   = "e"

      /a/b
        root  = "/"
        path  = "a"
        base  = "b"
        ext   = "e"

     */
    static void parse
    (const std::string&  filename,
     std::string&        drive,    
     Array<std::string>& path,
     std::string&        base,
     std::string&        ext);
};

} // namespace G3D
#endif

