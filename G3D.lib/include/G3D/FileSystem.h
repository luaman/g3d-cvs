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

 \sa FilePath
 TODO: make threadsafe!
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

        /** Case-independent comparison on Windows */
        bool contains(const std::string& child) const;

        /** Compute the contents of nodeArray from this zipfile. */
        void computeZipListing(const std::string& zipfile, const std::string& pathInsideZipfile);

        Dir() : exists(false), isZipfile(false), inZipfile(false), lastChecked(0) {}
    };

    /** Maps path names (without trailing slashes, except for the file system root) to contents.
        On Windows, all paths are lowercase */
    Table<std::string, Dir>     m_cache;

    /** Update the cache entry for path if it is not already present.
     \param forceUpdate If true, always override the current cache value.*/
    Dir& getContents(const std::string& path, bool forceUpdate);

    /** Don't allow public construction. */
    FileSystem();

    /** Returns true if some sub-path of this one is a zipfile. 
       If the path itself is a zipfile, returns false.*/
    bool inZipfile(const std::string& path, std::string& zipsubpath);

public:
    
    static FileSystem& instance();

    /** Create the common instance. */
    static void init();

    /** Destroy the common instance. */
    static void cleanup();

#   ifdef G3D_WIN32
    /** On Windows, the drive letters that form the file system roots.*/
    const Array<std::string>& _drives();
    /** \copydoc _drives */
    static const Array<std::string>& drives() {
        return instance()._drives();
    }
#   endif

    /** Flushes the cache */
    void _flushCache();

    /** \copydoc _flushCache */
    static void flushCache() {
        instance()._flushCache();
    }

    /** Returns true if \a path is a file that is a zipfile. Note that G3D requires zipfiles to have
        some extension, although it is not required to be "zip" */
    bool _isZipfile(const std::string& path);

    /** \copydoc isZipfile */
    static bool isZipfile(const std::string& path) {
        return instance()._isZipfile(path);
    }

    /** Set the cacheLifetime().
       \param t in seconds */
    void _setCacheLifetime(float t);

    /** \copydoc _setCacheLifetime */
    void setCacheLifetime(float t) {
        instance()._setCacheLifetime(t);
    }

    /** A cache is used to optimize repeated calls.  A cache entry is considered
        valid for this many seconds after it has been checked. */
    float _cacheLifetime() const {
        return m_cacheLifetime;
    }

    /** \copydoc _cacheLifetime */
    static float cacheLifetime() {
        return instance()._cacheLifetime();
    }

    /** Creates the directory named, including any subdirectories 
        that do not already exist.

        The directory must not be inside a zipfile.

        Flushes the cache.
     */
    void _createDirectory(const std::string& path);

    /** \copydoc _createDirectory */
    static void createDirectory(const std::string& path) {
        instance()._createDirectory(path);
    }

    /** The current working directory (cwd).  Only ends in a slash if this is the root of the file system. */
    std::string _currentDirectory();

    /** \copydoc _currentDirectory */
    static std::string currentDirectory() {
        return instance()._currentDirectory();
    }

    /**
    \param srcPath Must name a file.
    \param dstPath Must not contain a zipfile.

    Flushes the cache.
    */
    void _copyFile(const std::string& srcPath, const std::string& dstPath);

    /** \copydoc _copyFile */
    static void copyFile(const std::string& srcPath, const std::string& dstPath) {
        instance()._copyFile(srcPath, dstPath);
    }

    /** Returns true if a node named \a f exists.

        \param trustCache If true, uses the cache for optimizing repeated calls 
        in the same parent directory. 
     */
    bool _exists(const std::string& f, bool trustCache = true);

    /** \copydoc _exists */
    static bool exists(const std::string& f, bool trustCache = true) {
        return instance()._exists(f, trustCache);
    }

    /** Known bug: does not work inside zipfiles */
    bool _isDirectory(const std::string& path);
    
    /** \copydoc _isDirectory */
    static bool isDirectory(const std::string& path) {
        return instance()._isDirectory(path);
    }

    /** Known bug: does not work inside zipfiles */
    bool _isFile(const std::string& path) {
        return ! isDirectory(path);
    }
    
    /** \copydoc _isFile */
    static bool isFile(const std::string& path) {
        return instance()._isFile(path);
    }

    /** Fully qualifies a filename.

        The filename may contain wildcards, in which case the wildcards will be preserved in the returned value.
    */
    std::string _resolve(const std::string& path);

    /** \copydoc _resolve */
    static std::string resolve(const std::string& path) {
        return instance()._resolve(path);
    }

    /** Returns true if \param dst does not exist or \param src is newer than \param dst,
       according to their time stamps.
       
       Known bug: does not work inside zipfiles.
       */
    bool _isNewer(const std::string& src, const std::string& dst);

    /** \copydoc _isNewer */
    static bool isNewer(const std::string& src, const std::string& dst) {
        return instance()._isNewer(src, dst);
    }

    /** Returns the length of the file in bytes, or -1 if the file could not be opened. */
    int64 _size(const std::string& path);

    /** \copydoc _size */
    static int64 size(const std::string& path) {
        return instance()._size(path);
    }

    /** Appends all nodes matching \a spec to the \a result array.

      Wildcards can only appear to the right of the last slash in \a spec.

      The names will not contain parent paths unless \a includePath == true. 
      These may be relative to the current directory unless \a spec 
      is fully qualified (can be done with resolveFilename). 
      
     */
    void _list(const std::string& spec, Array<std::string>& result,
        bool files = true, bool directories = true, bool includeParentPath = false);

    /** \copydoc _list */
    static void list(const std::string& spec, Array<std::string>& result,
        bool files = true, bool directories = true, bool includeParentPath = false) {
        return instance()._list(spec, result, files, directories, includeParentPath);
    }

    /** list() files */
    void _getFiles(const std::string& spec, Array<std::string>& result, bool includeParentPath = false) {
        _list(spec, result, true, false, includeParentPath);
    }

    /** \copydoc _getFiles */
    static void getFiles(const std::string& spec, Array<std::string>& result, bool includeParentPath = false) {
        return instance()._getFiles(spec, result, includeParentPath);
    }

    /** list() directories */
    void _getDirectories(const std::string& spec, Array<std::string>& result, bool includeParentPath = false) {
        _list(spec, result, false, true, includeParentPath);
    }

    /** \copydoc getDirectories */
    static void getDirectories(const std::string& spec, Array<std::string>& result, bool includeParentPath = false) {
        return instance()._getDirectories(spec, result, includeParentPath);
    }
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


    /**
      std::string version of function fnmatch() as specified in POSIX 1003.2-1992, section B.6.
      Compares a filename or pathname to a pattern.

    The fnmatch() function checks whether the string argument matches the pattern argument, which is a shell wildcard pattern.
    The flags argument modifies the behaviour; it is the bitwise OR of zero or more of the following flags:

    - FNM_NOESCAPE If this flag is set, treat backslash as an ordinary character, instead of an escape character.
    - FNM_PATHNAME If this flag is set, match a slash in string only with a slash in pattern and not by an asterisk (*) or a question mark (?) metacharacter, nor by a bracket expression ([]) containing a slash.
    - FNM_PERIOD If this flag is set, a leading period in string has to be matched exactly by a period in pattern. A period is considered to be leading if it is the first character in string, or if both FNM_PATHNAME is set and the period immediately follows a slash.
    - FNM_FILE_NAME This is a GNU synonym for FNM_PATHNAME.
    - FNM_LEADING_DIR If this flag (a GNU extension) is set, the pattern is considered to be matched if it matches an initial segment of string which is followed by a slash. This flag is mainly for the internal use of glibc and is only implemented in certain cases.
    - FNM_CASEFOLD If this flag (a GNU extension) is set, the pattern is matched case-insensitively.

    \return Zero if \a string matches \a pattern, FNM_NOMATCH if there is no match or another non-zero value if there is an error
     */
    static bool matches(const std::string& path, const std::string& pattern, int flags = 0);
};

} // namespace G3D
#endif

