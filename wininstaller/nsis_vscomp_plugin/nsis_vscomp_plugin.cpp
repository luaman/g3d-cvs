#include <windows.h>
#include "exdll.h"

HWND g_hwndParent;

extern "C" void __declspec(dllexport) InsertVCComponentDirectories(HWND hwndParent, int string_size, 
                                                        char *variables, stack_t **stacktop,
                                                        extra_parameters *extra)
{
    g_hwndParent=hwndParent;

    EXDLL_INIT();


    // note if you want parameters from the stack, pop them off in order.
    // i.e. if you are called via exdll::myFunction file.dat poop.dat
    // calling popstring() the first time would give you file.dat,
    // and the second time would give you poop.dat. 
    // you should empty the stack of your parameters, and ONLY your
    // parameters.

    char vccompPath[1024] = {0};
    char includeDir[1024] = {0};
    char libraryDir[1024] = {0};

    // pop path to vccomponent.dat
    int popSuccess = popstring(vccompPath);

    if (popSuccess == 0)
    {
        // pop include dir
         popSuccess = popstring(includeDir);
    }

    if (popSuccess == 0)
    {
        // pop library dir
        popSuccess = popstring(libraryDir);
    }

    if (popSuccess == 0)
    {
        // open vccomponent.dat
        HANDLE fileHandle = CreateFile(vccompPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (fileHandle != INVALID_HANDLE_VALUE)
        {
            DWORD fileSize = GetFileSize(fileHandle, NULL);

            wchar_t* oldFileBuffer = (WCHAR*)GlobalAlloc(GPTR, fileSize + 1);
            DWORD bytesRead = 0;

            // Read in unicode vccomponent.dat
            ReadFile(fileHandle, oldFileBuffer, fileSize, &bytesRead, NULL);
            CloseHandle(fileHandle);

            // Create unicode versions of include and library directories
            wchar_t unicodeInclude[1024] = {0};
            wchar_t unicodeLibrary[1024] = {0};

            size_t ansiIncludeLen = strlen(includeDir);
            size_t ansiLibraryLen = strlen(libraryDir);

            MultiByteToWideChar(CP_ACP, 0, includeDir, ansiIncludeLen, unicodeInclude, ansiIncludeLen);
            MultiByteToWideChar(CP_ACP, 0, libraryDir, ansiLibraryLen, unicodeLibrary, ansiLibraryLen);

            // Create new buffer that can contain old file and new entries
            wchar_t* newFileBuffer = (wchar_t*)GlobalAlloc(GPTR, fileSize + 2048);

            // check for existence of exactly include path, if not found add it
            wchar_t* unicodeIncludeLoc = wcsstr(oldFileBuffer, unicodeInclude);
            wchar_t* unicodeLibraryLoc = wcsstr(oldFileBuffer, unicodeLibrary);

            wchar_t* oldBufferCopyLoc = oldFileBuffer;
            size_t oldBufferCopyLen = bytesRead;

            wchar_t* newBufferCopyLoc = newFileBuffer;

            if ((unicodeIncludeLoc == NULL) && (unicodeLibraryLoc == NULL))
            {
                // Fine the lines containing lists we want to modify
                wchar_t* includeDirsLineLoc = wcsstr(oldFileBuffer, L"Include Dirs=");
                wchar_t* libraryDirsLineLoc = wcsstr(oldFileBuffer, L"Library Dirs=");

                ptrdiff_t diffBetweenLines = (includeDirsLineLoc - libraryDirsLineLoc);

                // include line should be before library line
                if (diffBetweenLines < 0)
                {
                    // Insert include path
                    size_t sizeToIncludeLine = includeDirsLineLoc - oldBufferCopyLoc;
                    wcsncpy(newBufferCopyLoc, oldBufferCopyLoc, sizeToIncludeLine);
                    newBufferCopyLoc += sizeToIncludeLine;
                    oldBufferCopyLoc += sizeToIncludeLine;

                    wcsncpy(newBufferCopyLoc, L"Include Dirs=", 13);
                    newBufferCopyLoc += 13;
                    oldBufferCopyLoc += 13;

                    wcsncpy(newBufferCopyLoc, unicodeInclude, wcslen(unicodeInclude));
                    newBufferCopyLoc += wcslen(unicodeInclude);

                    wcsncpy(newBufferCopyLoc, L";", 1);
                    newBufferCopyLoc += 1;

                    // insert library path
                    size_t sizeToLibraryLine = libraryDirsLineLoc - oldBufferCopyLoc;
                    wcsncpy(newBufferCopyLoc, oldBufferCopyLoc, sizeToLibraryLine);
                    newBufferCopyLoc += sizeToLibraryLine;
                    oldBufferCopyLoc += sizeToLibraryLine;

                    wcsncpy(newBufferCopyLoc, L"Library Dirs=", 13);
                    newBufferCopyLoc += 13;
                    oldBufferCopyLoc += 13;

                    wcsncpy(newBufferCopyLoc, unicodeLibrary, wcslen(unicodeLibrary));
                    newBufferCopyLoc += wcslen(unicodeLibrary);

                    wcsncpy(newBufferCopyLoc, L";", 1);
                    newBufferCopyLoc += 1;

                    // copy rest of old file
                    wcscpy(newBufferCopyLoc, oldBufferCopyLoc);

                    // Write out new file

                    // open vccomponent.dat
                    fileHandle = CreateFile(vccompPath, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

                    if (fileHandle != INVALID_HANDLE_VALUE)
                    {
                        DWORD bytesWritten = 0;

                        WriteFile(fileHandle, newFileBuffer, wcslen(newFileBuffer) * 2, &bytesWritten, NULL);
                        CloseHandle(fileHandle);
                    }
                }
            }

            GlobalFree((HGLOBAL)oldFileBuffer);
            GlobalFree((HGLOBAL)newFileBuffer);
        }
    }
}



BOOL WINAPI DllMain(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
    return TRUE;
}
