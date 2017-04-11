#ifndef APP_H
#define APP_H

#include <G3D/G3DAll.h>

#include "photonmap.h"
#include "world.h"

#define NUM_PHOTONS     500000      /* How many photons to gather */
#define GATHER_RADIUS   0.1         /* Max distance between intersection point and photons in map */
#define MAX_DEPTH       4           /* Recursve depth of the raytracer */
#define DIRECT_SAMPLES  64         /* Number of samples to take of direct light sources */
#define EPSILON 1e-4
#define GATHER_SAMPLES  16            /*Number of ray samples for final gather*/

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

private:
    PhotonMap              m_photons;  // Contains photons organized spatially
    World                  m_world;    // The scene being rendered
    shared_ptr<Image3>     m_canvas;   // Output buffer for raytrace()
    Random                 m_random;   // Random number generator
    shared_ptr<Thread>     m_dispatch; // Spawns rendering threads
    bool                   m_useGather; // Boolean to use final gather
};

#endif
