#ifndef APP_H
#define APP_H

#include <ctime>
#include <G3D/G3DAll.h>

#include "world.h"
#include "photonscatter.h"
#include "indphotonscatter.h"
#include "dirphotonscatter.h"
#include "indrenderer.h"
#include "photonsettings.h"

//enum RenderMethod { RAY, PATH, PHOTON };

//#include "dirphotonscatter.h"

/** The entry point and main window manager */
class App : public GApp
{
public:
    enum Stage { IDLE, SCATTERING, GATHERING };
    enum View { DEFAULT, PHOTONMAP, SPLAT, RENDITION };

    App(const GApp::Settings &settings = GApp::Settings());
    virtual ~App();

    /** Calls the shoot() callback until the minumum photon count is met */
    void buildPhotonMap();

    /** Multithreaded callback for tracing gather rays */
    void traceCallback(int x, int y);

    /** Called once per pixel for raytracing */
    void threadCallback(int x, int y);

    /** Called once at application startup */
    virtual void onInit();

    /** Called once at application shutdown */
    virtual void onCleanup();

    /** Called once per frame to render the scene */
    virtual void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface>>& surface3D) override;

    /** Called from onInit() */
    void makeGUI();

//    /** Processes user input */
//    virtual void onUserInput(UserInput *input);

    Stage stage;
    View view;

    void loadSceneDirectory(String directory = m_scenePath);
    void changeDataDirectory();

    void onRender();
    void setScenePath(const char *path);
    void loadDefaultScene();
    void loadCustomScene();
    void loadCS244Scene();
    void saveCanvas();
    FilmSettings getFilmSettings();
    void toggleWindowRendering();
    void toggleWindowScenes();

    static bool m_kill;
    void toggleWindowPath();

    int             pass; // how many passes we have taken for a given pixel
    int             num_passes;
    bool            continueRender;

private:

    void gpuProcess(RenderDevice *rd);

    /** Makes the verts to visualize the indirection lighting */
    void makeLinesIndirBeams(SlowMesh &mesh);

    /** Makes the verts to visualize the direct lighting */
    void makeLinesDirBeams(SlowMesh &mesh);

    // TODO : temp
    shared_ptr<ArticulatedModel> m_model;
    Array<shared_ptr<Surface>> m_sceneGeometry;

    shared_ptr<Texture> m_dirLight;
    shared_ptr<Framebuffer> m_dirFBO;

    // path flags
    PhotonSettings         m_PSettings;
//    shared_ptr<PathTracer> m_renderer;

    Random                 m_random;   // Random number generator
    int                    m_passType; // Pass type to render

    shared_ptr<GuiWindow> m_windowRendering;
    shared_ptr<GuiWindow> m_windowScenes;
    shared_ptr<GuiWindow> m_windowPath;
    std::unique_ptr<DirPhotonScatter> m_dirBeams;
    std::unique_ptr<IndPhotonScatter> m_inDirBeams;
    std::unique_ptr<IndRenderer> m_indRenderer;

    static String           m_scenePath; // path to scene folder
    static String           m_defaultScene;
    float                   m_scaleFactor; // how much to scale down images by.

    World               m_world;    // The scene being rendered
    shared_ptr<Image3>  m_canvas;   // Output buffer for raytrace()
    shared_ptr<Thread>  m_dispatch; // Spawns rendering threads

#if 0
    bool                m_pointLights; // true if these are turned on
    bool                m_areaLights;
    bool				m_direct;      // show direct lighting of first intersection point?
    bool				m_direct_s;      // show SPECULAR reflections of direct lighting of first intersection point?
    bool				m_indirect;    // show indirect lighting at first intersection point?
    bool				m_emit;        // show emitted light

    bool                m_fresnelEnabled;
    bool                m_attenuation;
#endif

    // GUI stuff
    GuiDropDownList*    m_ddl;
    GuiDropDownList*    m_renderdl;
    GuiDropDownList*    m_lightdl;
    GuiLabel*           m_warningLabel;
    GuiLabel*           m_scenePathLabel;
    String              m_dirName;
    void updateScenePathLabel();
    void renderBeams(RenderDevice *dev, World *world);
};

#endif
