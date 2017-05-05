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
#include "threadpool.h"

/** The entry point and main window manager */
class App : public GApp
{
public:
    enum Stage { IDLE, SCATTERING, GATHERING };
    enum View { DEFAULT, DIRBEAMS, INDBEAMS, SPLAT, RENDITION };

    App(const GApp::Settings &settings = GApp::Settings());
    virtual ~App();

    /** Calls the shoot() callback until the minumum photon count is met */
    void buildPhotonMap(bool createRngGen);

    /** Multithreaded callback for tracing gather rays */
    void traceCallback(int x, int y);

    /** Called once per pixel for raytracing */
    void threadCallback(int x, int y);

    /** Called once at application startup */
    virtual void onInit();

    virtual bool onEvent(const GEvent& e) override;

    /** Called once at application shutdown */
    virtual void onCleanup();

    /** Called once per frame to render the scene */
    virtual void onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface>>& surface3D) override;

    /** Called from onInit() */
    void makeGUI();

    Stage stage;
    View view;

    void loadSceneDirectory(String directory = m_scenePath);

    void onRender();
    void setScenePath(const char *path);
    void saveCanvas();
    FilmSettings getFilmSettings();

    void toggleWindowRendering();
    void toggleWindowScenes();
    void toggleWindowPath();

    void setGatherRadius();

    int                 indRenderCount;
    int                 prevIndRenderCount;
    int                 m_maxPasses;
    bool                continueRender;


private:

    void gpuProcess(RenderDevice *rd);

    /** Makes the verts to visualize the indirection lighting */
    void makeLinesIndirBeams(SlowMesh &mesh);

    /** Makes the verts to visualize the direct lighting */
    void makeLinesDirBeams(SlowMesh &mesh);

    /** Resets render counts, called when settings change */
    void clearParams();


    World                           m_world;    // The scene being rendered
    shared_ptr<ArticulatedModel>    m_model;
    Array<shared_ptr<Surface>>      m_sceneGeometry;
    static String                   m_scenePath; // path to scene folder
    static String                   m_defaultScene;

    shared_ptr<Texture> m_dirLight;
    shared_ptr<Texture> m_zBuffer;
    shared_ptr<Texture> m_totalDirLight1;
    shared_ptr<Texture> m_currentComposite1;
    shared_ptr<Texture> m_totalDirLight2;
    shared_ptr<Texture> m_currentComposite2;

    shared_ptr<Framebuffer> m_dirFBO;
    shared_ptr<Framebuffer> m_FBO1;
    shared_ptr<Framebuffer> m_FBO2;
    shared_ptr<Framebuffer> m_ZFBO;

    shared_ptr<PhotonSettings>         m_PSettings;

    Random                 m_random;   // Random number generator

    std::unique_ptr<DirPhotonScatter> m_dirBeams;
    std::unique_ptr<IndPhotonScatter> m_inDirBeams;
    std::unique_ptr<IndRenderer> m_indRenderer;

    int                 m_passType;
    shared_ptr<Image3>  m_canvas;   // Output buffer for raytrace()
    shared_ptr<Thread>  m_dispatch; // Spawns rendering threads
    float               m_radius; // Current radius of the beams to be rendered
    int                 m_passes;
    int                 num_passes;
    bool                m_updating;
    float               m_scaleFactor; // how much to scale down images by.

    // GUI stuff
    shared_ptr<GuiWindow> m_windowRendering;
    shared_ptr<GuiWindow> m_windowScenes;
    shared_ptr<GuiWindow> m_windowPath;
    GuiDropDownList*    m_ddl;
    GuiDropDownList*    m_renderdl;
    GuiDropDownList*    m_lightdl;
    GuiLabel*           m_scenePathLabel;
    String              m_dirName;
    void updateScenePathLabel();
    void renderBeams(RenderDevice *dev, World *world);
};

#endif
