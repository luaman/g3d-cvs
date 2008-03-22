#include "G3D/G3DAll.h"
using G3D::uint8;
using G3D::uint32;
using G3D::uint64;


static bool isZipfileTest(const std::string& filename) {
	return isZipfile(filename);
}


static bool zipfileExistsTest(const std::string& filename) {
	std::string path, contents;
	return zipfileExists(filename, path, contents);
}


void testZip() {
	
	printf("zip API ");

	int x; // for loop iterator

	// isZipfile()
	bool isZipTest = isZipfileTest("apiTest.zip");
	debugAssertM(isZipTest, "isZipfile failed.");

	// zipfileExists()
	bool zipExistsTest = zipfileExistsTest("apiTest.zip/Test.txt");
	debugAssertM(zipExistsTest, "zipfileExists failed.");

	// getFiles() - normal
	bool normalFiles = true;
	Array<std::string> files;
	getFiles("TestDir/*", files);

	if (files.length() != 1) {
		normalFiles = false;
	}
	for (x = 0; x < files.length(); ++x) {
		if(files[x] != "Test.txt") {
			normalFiles = false;
		}
	}
	debugAssertM(normalFiles, "Normal getFiles failed.");

	// getDirs() - normal
	Array<std::string> dirs;
	getDirs("TestDir/*", dirs);

	bool normalDirs = 	(dirs.length() == 1 && dirs[0] == "Folder") ||
		(dirs.length() == 2 && (dirs[0] == "Folder" || dirs[1] == "Folder"));

	debugAssertM(normalDirs, "Normal getDirs failed.");

	// getFiles() + getDirs() - invalid
	Array<std::string> emptyTest;
	getFiles("nothing", emptyTest);
	getDirs("nothing", emptyTest);
	bool noFile = emptyTest.length() == 0;
	debugAssertM(noFile, "Improper response to a file that does not exist.");

	// getFiles() - zip
	bool zipFiles = true;
	std::string zipDir = "apiTest.zip/*";
	Array<std::string> zFiles;

	getFiles(zipDir, zFiles);

	if (zFiles.length() != 1) {
		zipFiles = false;
	}
	for (x = 0; x < zFiles.length(); ++x) {
		if(zFiles[x] != "Test.txt") {
			zipFiles = false;
		}
	}
	debugAssertM(zipFiles, "Zip getFiles failed.");

	// getDirs() - zip
	bool zipDirs = true;
	Array<std::string> zDirs;

	getDirs(zipDir, zDirs);

	if (zDirs.length() != 1) {
		zipDirs = false;
	}
	for (x = 0; x < zDirs.length(); ++x) {
		if (zDirs[x] != "zipTest") {
			zipDirs = false;
		}
	}
	debugAssertM(zipDirs, "Zip getDirs failed.");

	// fileLength() - normal
	bool normalLength = false;
	if (fileLength("TestDir/Test.txt") == 69) {
		normalLength = true;
	}
	debugAssertM(normalLength, "Normal fileLength failed.");

	// fileLength() - nonexistent
	bool noLength = false;
	if (fileLength("Grawk") == -1) {
		noLength = true;
	}
	debugAssertM(noLength, "Nonexistent fileLength failed.");

	// fileLength() - zip
	bool zipLength = false;
	if(fileLength("apiTest.zip/Test.txt") == 69) {
		zipLength = true;
	}
	debugAssertM(zipLength, "Zip fileLength failed.");


	// Contents of files
	void* test = NULL;
	size_t zLength, length;

	zipRead("apiTest.zip/Test.txt", test, zLength);

	// Read correct results
	const char* filename = "TestDir/Test.txt";
	FILE* file = fopen(filename, "r");
	length = fileLength(filename);
	debugAssert(file);
	void* correct = System::alignedMalloc(length, 16);
	fread(correct, 1, length, file);
    fclose(file);

	// If the lengths aren't the same, the files can't be the same
	debugAssertM(length == zLength, "After zipRead, files are not the same length");

	debugAssertM(memcmp(correct, test, length) == 0, "After zipRead, files are not the same.");

	System::alignedFree(correct);
	zipClose(test);

	printf("passed\n");
}
