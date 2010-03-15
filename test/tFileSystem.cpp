#include "G3D/G3DAll.h"

void testFileSystem() {
    printf("FileSystem...");

    debugAssert(g3dfnmatch("*.zip", "hello.not", FNM_PERIOD | FNM_NOESCAPE | FNM_PATHNAME) == FNM_NOMATCH);
    debugAssert(g3dfnmatch("*.zip", "hello.zip", FNM_PERIOD | FNM_NOESCAPE | FNM_PATHNAME) == 0);

    debugAssert(FilePath::matches("hello", "*", false));
    debugAssert(FilePath::matches("hello", "*", true));

    chdir("TestDir");
    std::string cwd = FileSystem::currentDirectory();
    debugAssert(endsWith(cwd, "TestDir"));
    chdir("..");

    // Directory listing
    Array<std::string> files;
    FileSystem::getFiles("*", files);
    debugAssert(files.contains("Any-load.txt"));
    debugAssert(files.contains("apiTest.zip"));

    // Directory listing
    files.clear();
    FileSystem::getFiles("*.zip", files);
    debugAssert(files.contains("apiTest.zip"));
    debugAssert(files.size() == 1);

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
