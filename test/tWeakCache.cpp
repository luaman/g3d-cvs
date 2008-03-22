#include "G3D/G3DAll.h"
using G3D::uint8;
using G3D::uint32;
using G3D::uint64;

class CacheTest : public ReferenceCountedObject {
public:
    static int count;
    int x;
    CacheTest() {
        ++count;
    }
    ~CacheTest() {
        --count;
    }
};
int CacheTest::count = 0;
typedef ReferenceCountedPointer<CacheTest> CacheTestRef;

void testWeakCache() {
    WeakCache<std::string, CacheTestRef> cache;

    debugAssert(CacheTest::count == 0);
    CacheTestRef x = new CacheTest();
    debugAssert(CacheTest::count == 1);

    cache.set("x", x);
    debugAssert(CacheTest::count == 1);
    CacheTestRef y = new CacheTest();
    CacheTestRef z = new CacheTest();
    debugAssert(CacheTest::count == 3);
    
    cache.set("y", y);
    
    debugAssert(cache["x"] == x);
    debugAssert(cache["y"] == y);
    debugAssert(cache["q"].isNull());
    
    x = NULL;
    debugAssert(CacheTest::count == 2);
    debugAssert(cache["x"].isNull());

    cache.set("y", z);
    y = NULL;
    debugAssert(cache["y"] == z);

    cache.remove("y");
}
