#if 0
#include "../include/G3D/G3DAll.h"
using G3D::uint8;
using G3D::uint32;
using G3D::uint64;
#include "../contrib/GChunk/GChunk.h"

void testGChunk() {
    printf("GChunk ");

    enum {HEADER, NAME, BODY, NUM, DATA};
    {
        BinaryOutput b("file.dat", G3D_LITTLE_ENDIAN);

        {
            GChunk c(b, HEADER);

            {
                GChunk c(b, NAME, STRING_BINFMT);
                b.writeString("abcdefg");
                c.finish(b);
            }
            c.finish(b);
        }

        {
            GChunk c(b, BODY);

            {
                GChunk c(b, NUM, INT32_BINFMT);
                b.writeInt32(10);
                c.finish(b);
            }

            {
                GChunk c(b, DATA, FLOAT32_BINFMT);
                for (int i = 0; i < 10; ++i) {
                    b.writeFloat32(sqrt((double)i));
                }
                c.finish(b);
            }
            c.finish(b);
        }

        b.commit();
    }

    {

        BinaryInput b("file.dat", G3D_LITTLE_ENDIAN);

        {
            GChunk c(b, HEADER);

            {
                GChunk c(b, NAME, STRING_BINFMT);
                debugAssert(c.count == -1);
                alwaysAssertM(b.readString() == "abcdefg", "");
                c.finish(b);
            }
            c.finish(b);
        }

        {
            GChunk c(b, BODY);

            {
                GChunk c(b, NUM, INT32_BINFMT);
                debugAssert(c.count == 1);
                alwaysAssertM(b.readInt32() == 10, "");
                c.finish(b);
            }

            {
                GChunk c(b, DATA, FLOAT32_BINFMT);
                debugAssert(c.count == 10);
                for (int i = 0; i < 10; ++i) {
                    alwaysAssertM(fuzzyEq(b.readFloat32(), sqrt((double)i)),
                        "Data in chunk corrupted");
                }
                c.finish(b);
            }
            c.finish(b);
        }
    }

    printf("passed\n");
}

#endif
