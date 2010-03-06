#include "G3D/G3DAll.h"
using G3D::uint8;
using G3D::uint32;
using G3D::uint64;

void testHugeBinaryIO() {
    printf("BinaryOutput Large Files\n");
	if (FileSystem::exists("huge.bin", false)) {
#if defined(G3D_WIN32)
        system("del huge.bin");
#elif defined(G3D_LINUX)
		system("rm huge.bin");
#elif defined(G3D_OSX)
		system("rm huge.bin");
#endif
    }

    size_t testSize = 1024 * 1024 * 600;

    size_t stepSize = 1024 * 1024 * 2;
    uint8* giantBuffer = new uint8[stepSize];
    debugAssert(giantBuffer);

    {
        BinaryOutput b("huge.bin", G3D_LITTLE_ENDIAN);
        for (int i = 0; i < (int)testSize / (int)stepSize; ++i) {
            b.writeBytes(giantBuffer, (int)stepSize);
        }
        b.commit();
    }

    printf("BinaryInput Large Files\n");

    {
        BinaryInput b("huge.bin", G3D_LITTLE_ENDIAN);

        for (int i = 0; i < (int)testSize / (int)stepSize; ++i) {
            b.readBytes(giantBuffer, (int)stepSize);
        }
    }

    delete giantBuffer;

	if (FileSystem::exists("huge.bin", false)) {
#if defined(G3D_WIN32)
        system("del huge.bin");
#elif defined(G3D_LINUX)
		system("rm huge.bin");
#elif defined(G3D_OSX)
		system("rm huge.bin");
#endif
    }
}

static void testBitSerialization() {
    printf("Bit Serialization\n");
    uint8 x[100];

    {
        BinaryOutput b("<memory>", G3D_LITTLE_ENDIAN);

        b.beginBits();
            b.writeBits(0, 1);
            b.writeBits(1, 1);
        b.endBits();

        b.commit(x);

        debugAssert(x[0] == 2);
    }

    {
        BinaryInput b(x, 1, G3D_LITTLE_ENDIAN);
        b.beginBits();
            
            uint8 a = b.readBits(1);
            debugAssert(a == 0);
            
            a = b.readBits(1);
            debugAssert(a == 1);
        b.endBits();
    }

    {
        BinaryOutput b("<memory>", G3D_LITTLE_ENDIAN);
        b.beginBits();
            b.writeBits(0xF1234567, 32);
        b.endBits();

        b.commit(x);

        debugAssert(x[0] == 0x67);
        debugAssert(x[1] == 0x45);
        debugAssert(x[2] == 0x23);
        debugAssert(x[3] == 0xF1);
    }

    {
        BinaryInput b(x, 4, G3D_LITTLE_ENDIAN);
        b.beginBits();
            
            uint8 a = b.readBits(8);
            debugAssert(a == 0x67);
            
            a = b.readBits(8);
            debugAssert(a == 0x45);
            
            a = b.readBits(8);
            debugAssert(a == 0x23);

            a = b.readBits(8);
            debugAssert(a == 0xF1);

        b.endBits();
    }

    {
        BinaryOutput b("<memory>", G3D_LITTLE_ENDIAN);

        b.beginBits();
            b.writeBits(0, 3);
            b.writeBits(3, 3);
            b.writeBits(4, 3);
            b.writeBits(7, 3);
        b.endBits();

        b.commit(x);
    }

    {
        BinaryInput b(x, 2, G3D_LITTLE_ENDIAN);
        b.beginBits();
            
            uint8 a = b.readBits(3);
            debugAssert(a == 0);
            
            a = b.readBits(3);
            debugAssert(a == 3);

            a = b.readBits(3);
            debugAssert(a == 4);

            a = b.readBits(3);
            debugAssert(a == 7);
        b.endBits();
    }

}


static void testCompression() {
    printf("BinaryInput & BinaryOutput\n");
    BinaryOutput f("out.t", G3D_LITTLE_ENDIAN);

    for (int i = 0; i < 100; ++i) {
        f.writeUInt32(1234);
        f.writeFloat64(1.234);
    }
    f.compress();
    f.commit();

    BinaryInput g("out.t", G3D_LITTLE_ENDIAN, true);
    for (int k = 0; k < 100; ++k) {
        uint32 i = g.readUInt32();
        debugAssert(i == 1234); (void)i;
        double j = g.readFloat64();
        debugAssert(j == 1.234); (void)j;
    }
}


static void measureSerializerPerformance() {
    Array<uint8> x;
    x.resize(1024);
    RealTime t0 = System::time();
    Matrix4 M(Matrix4::identity());
    
    for (int i = 0; i < 100; ++i) {
        BinaryOutput b("<memory>", G3D_LITTLE_ENDIAN);
        b.writeInt32(1);
        b.writeInt32(2);
        b.writeInt32(8);
        M.serialize(b);
        b.commit(x.getCArray());
    }
    RealTime reallocTime = (System::time() - t0) / 100.0;
    printf("BinaryOutput time with re-allocation: %gs\n", reallocTime);

    BinaryOutput b("<memory>", G3D_LITTLE_ENDIAN);
    t0 = System::time();    
    for (int i = 0; i < 100; ++i) {
        b.writeInt32(1);
        b.writeInt32(2);
        b.writeInt32(8);
        M.serialize(b);
        b.commit(x.getCArray());
        b.reset();
    }
    RealTime resetTime = (System::time() - t0) / 100.0;
    printf("BinaryOutput time with BinaryOutput::reset: %gs\n\n", resetTime);
    
}


void perfBinaryIO() {
    measureSerializerPerformance();
}


void testBasicSerialization() {
    Vector3 tmp(-100.0f, -10.0f, 2.0f);
    Vector3int16 tmp2(100, -10, 2);

    {

        BinaryOutput bo("outfile.bin", G3D_LITTLE_ENDIAN);
        tmp.serialize(bo);
        tmp2.serialize(bo);
        bo.commit();

    }
    {
        BinaryInput bi("outfile.bin", G3D_LITTLE_ENDIAN);
        Vector3 alpha;
        Vector3int16 alpha2;
        alpha.deserialize(bi);
        alpha2.deserialize(bi);
        debugAssertM(alpha == tmp, format("%s should be %s \n", alpha.toString().c_str(), tmp.toString().c_str()));
        debugAssertM(alpha2 == tmp2, format("%s should be %s \n", alpha2.toString().c_str(), tmp2.toString().c_str()));
    }
}

void testBinaryIO() {
    testBasicSerialization();
    testBitSerialization();
    testCompression();
}
