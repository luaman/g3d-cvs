/**
  \file IconSet.h

  \maintainer Morgan McGuire, http://graphics.cs.williams.edu
  \created 2010-01-04
  \edited  2010-01-04
*/
#ifndef GLG3D_IconSet_h
#define GLG3D_IconSet_h

#include "G3D/platform.h"
#include "G3D/Array.h"
#include "G3D/Table.h"
#include "G3D/Rect2D.h"
#include "G3D/ReferenceCount.h"
#include "GLG3D/Icon.h"
#include "GLG3D/Texture.h"

namespace G3D {

/**
 \brief A set of small image files packed into a single G3D::Texture
 for efficiency.

 Examples:
 <pre>
 IconSet::Ref icons = IconSet::fromFile("tango.icn");
 debugPane->addButton(icons->get("16x16/actions/document-open.png"));

 int index = icons->getIndex("16x16/actions/edit-clear.png");
 debugPane->addButton(icons->get(index));
 </pre>
*/
class IconSet {
public:

    typedef ReferenceCountedPointer<IconSet> Ref;

private:

    class Source {
    public:
        std::string filename;
        int         width;
        int         height;
        int         channels;
    };
   
    class Entry {
    public:
        std::string filename;
        Rect2D      rect;
    };

    /** Recursively find images.
        \param baseDir Not included in the returned filenames.*/
    static void findImages(const std::string& baseDir, const std::string& sourceDir, 
                           Array<Source>& sourceArray);

    Texture::Ref             m_texture;
    Table<std::string, int>  m_index;
    Array<Entry>             m_icon;

public:
    
    /** Load an existing icon set from a file. */
    static IconSet::Ref fromFile(const std::string& filename);

    /** Load all of the image files (see G3D::GImage::supportedFormat)
        from \a sourceDir and its subdirectories and pack them into a
        single G3D::IconSet named \a outFile.

        The packing algorithm is not optimal.  Future versions of G3D
        may provide improved packing, and you can also create icon
        sets with your own packing algorithm--the indexing scheme
        allows arbitrary packing algorithms within the same file
        format.

        Ignores .svn and CVS directories.
    */
    static void makeIconSet(const std::string& sourceDir, const std::string& outFile);

    const Texture::Ref& texture() const {
        return m_texture;
    }

    /** Number of icons */
    int size() const {
        return m_icon.size();
    }

    /** Returns the index of the icon named \a s */
    int getIndex(const std::string& s) const {
        return m_index[s];
    }

    int getIndex(const char* s) const {
        return getIndex(std::string(s));
    }
    
    Icon get(int index) const;

    Icon get(const std::string& s) const {
        return get(getIndex(s));
    }

    Icon get(const char* s) const {
        return get(getIndex(s));
    }

    /** Returns the filename of the icon with the given \a index */
    const std::string& filename(int index) const {
        return m_icon[index].filename;
    }

    /** Texture coordinates */
    const Rect2D& rect(int index) const {
        return m_icon[index].rect;
    }
};

}
#endif
