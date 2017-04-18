#include <G3D/G3DAll.h>
#include "app.h"

#ifndef G3D_PATH
#define G3D_PATH "/contrib/projects/g3d10/G3D10"
#endif

G3D_START_AT_MAIN();

int main(int argc, const char *argv[])
{
    GApp::Settings s;
    s.window.caption = "Photon Beams";
    s.dataDir = G3D_PATH "/../data10/common";

    return App(s).run();
}
