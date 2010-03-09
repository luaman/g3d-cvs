#include "G3D/G3DAll.h"

void testFileSystem() {
    printf("FileSystem...");


    debugAssert(FilePath::matches("hello", "*"));
    debugAssert(FilePath::matches("hello", "*", FNM_CASEFOLD));

    chdir("TestDir");
    std::string cwd = FileSystem::currentDirectory();
    debugAssert(endsWith(cwd, "TestDir"));
    chdir("..");

    // Directory listing
    Array<std::string> files;
    FileSystem::getFiles("*", files);
    debugAssert(files.contains("Any-load.txt"));
    debugAssert(files.contains("apiTest.zip"));

    // Directory listing inside zipfile
    files.clear();
    debugAssert(FileSystem::exists("apiTest.zip"));
    debugAssert(FileSystem::isZipfile("apiTest.zip"));
    FileSystem::getFiles("apiTest.zip/*", files);
    debugAssert(files.size() == 1);
    debugAssert(files.contains("Test.txt"));

    files.clear();
    FileSystem::getDirectories("apiTest.zip/*", files);
    debugAssert(files.size() == 1);
    debugAssert(files.contains("zipTest"));

    debugAssert(! FileSystem::exists("nothere"));
    debugAssert(FileSystem::exists("apiTest.zip/Test.txt"));
    debugAssert(! FileSystem::exists("apiTest.zip/no.txt"));

    debugAssert(FileSystem::size("apiTest.zip") == 488);

    printf("passed\n");
}
