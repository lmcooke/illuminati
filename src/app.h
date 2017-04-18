#ifndef APP_H
#define APP_H

#include <G3D/G3DAll.h>

#include "photonmap.h"
#include "world.h"

#include "dirphotonscatter.h"

#define NUM_PHOTONS     500000      /* How many photons to gather */
#define GATHER_RADIUS   0.1         /* Max distance between intersection point and photons in map */
#define MAX_DEPTH       4           /* Recursve depth of the raytracer */
#define DIRECT_SAMPLES  64         /* Number of samples to take of direct light sources */
#define EPSILON 1e-4
#define GATHER_SAMPLES  16          /*Number of ray samples for final gather*/

// you can extend this if you want
class PTSettings
{
public:

    // all light contributions assumed to be area lights
    bool useDirectDiffuse;
    bool useDirectSpecular;
    bool useIndirect;
    bool useEmitted;
    bool useSkyMap;

    int superSamples; // for say, stratified sampling
    bool attenuation; // refracted path absorption through non-vacuum spaces
    bool useMedium; // enable volumetric mediums

    bool dofEnabled;
    float dofFocus;
    float dofLens;
    int dofSamples;

};

/** The entry point and main window manager */
class App : public GApp
{
public:
    enum Stage { IDLE, SCATTERING, GATHERING };
    enum View { DEFAULT, PHOTONMAP, RENDITION };

    App(const GApp::Settings &settings = GApp::Settings());
    virtual ~App();

    /**
      *
      * Shoot a single photon from a single light source, and add each of its
      * bounces to the photon map
      */
    void scatter();

    /**
     * Traces a photon into the scene
     */
    void photonTrace(const Photon& emitPhoton, Array<Photon>& incidentPhotons);

    /**
     * Helper for photon trace recursion
     */
    void photonTraceHelper(const Photon& emitPhoton, Array<Photon>& incidentPhotons, int bounces);

    /** Gathers emissive, direct, impulse and diffuse (photon map) illumination
      * from the point under the given ray
      */
    Radiance3 trace(const Ray &ray, int depth);

    /** Computes the direct illumination approaching the given surface point
      *
      * @param surf The surface point receiving illumination
      * @param wo   Points towards the viewer viewing the surface point
      */
    Radiance3 direct(shared_ptr<Surfel> surf, Vector3 wo);

    /** Computes the indirect illumination approaching the given surface point
      * via the impulse directions of the surface's BSDF
      *
      * @param surf The surface point receiving illumination
      * @param wo   Points towards the viewer viewing the surface point
      */
    Radiance3 impulse(shared_ptr<Surfel> surf, Vector3 wo, int depth);

    /**
      *
      * Computes the indirect illumination approaching the given surface point
      * via diffuse inter-object reflection, using the photon map.
      *
      * @param surf The surface point receiving illumination
      * @param wo   Points towards the viewer viewing the surface point
      */
    Radiance3 diffuse(shared_ptr<Surfel> surf, Vector3 wo, int depth);

    /** Calls the shoot() callback until the minumum photon count is met */
    void buildPhotonMap();

    /** Multithreaded callback for tracing gather rays */
    void traceCallback(int x, int y);

    /** Called once at application startup */
    virtual void onInit();

    /** Called once at application shutdown */
    virtual void onCleanup();

    /** Called once per frame to render the scene */
    virtual void onGraphics(RenderDevice *dev,
                            Array<shared_ptr<Surface> >& posed3D,
                            Array<shared_ptr<Surface2D> >& posed2D);

    /** Processes user input */
    virtual void onUserInput(UserInput *input);

    Stage stage;
    View view;

    /** Called from onInit() */
    void makeGUI();

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

//    static RenderMethod m_currRenderMethod;
    static bool m_kill;
    void changeRenderMethod();
    void toggleWindowPath();

    int             pass; // how many passes we have taken for a given pixel
    int             num_passes;
    bool            continueRender;

private:

    PTSettings          m_ptsettings;

    PhotonMap              m_photons;  // Contains photons organized spatially
    World                  m_world;    // The scene being rendered
    shared_ptr<Image3>     m_canvas;   // Output buffer for raytrace()
    Random                 m_random;   // Random number generator
    shared_ptr<Thread>     m_dispatch; // Spawns rendering threads
    bool                   m_useGather; // Boolean to use final gather

    shared_ptr<GuiWindow> m_windowRendering;
    shared_ptr<GuiWindow> m_windowScenes;
    shared_ptr<GuiWindow> m_windowPath;

    static String              m_defaultScene;

    static String         m_scenePath; // path to scene folder

    float                m_scaleFactor; // how much to scale down images by.

    // GUI stuff
    GuiDropDownList*    m_ddl;
    GuiDropDownList*    m_renderdl;
    GuiDropDownList*    m_lightdl;
    GuiLabel*           m_warningLabel;
    GuiLabel*           m_scenePathLabel;
    String              m_dirName;
    void updateScenePathLabel();

    DirPhotonScatter m_dirPhotonScatter;


};

#endif
