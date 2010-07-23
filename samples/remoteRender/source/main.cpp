#include <G3D/G3DAll.h>
#include "App.h"

G3D_START_AT_MAIN();

int main(int argc, char *argv[]) {
#   ifdef G3D_WIN32
        if (! FileSystem::exists("static")) {
            if (FileSystem::exists("G3D.sln")) {
                chdir("../samples/remoteRender/data-files");        
            } else if (FileSystem::exists("data-files")) {
                chdir("data-files");        
            } else {
                chdir("../data-files");
            }
        }
#   endif
    char x[2000];
    getcwd(x, 2000);
    alwaysAssertM(FileSystem::exists("static"), format("Cannot find runtime files. (cwd = %s)", x));

    App app;

    (void)getchar();  

    return 0;
}
