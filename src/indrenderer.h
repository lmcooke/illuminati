#ifndef INDRENDERER_H
#define INDRENDERER_H
#include <G3D/G3DAll.h>
#include "world.h"
#include "photonscatter.h"

#define GATHER_RADIUS   0.1         /* Max distance between intersection point and photons in map */
#define MAX_DEPTH       4           /* Recursve depth of the raytracer */
#define DIRECT_SAMPLES  64         /* Number of samples to take of direct light sources */
#define EPSILON 1e-4
#define GATHER_SAMPLES  16          /*Number of ray samples for final gather*/

/**
 * @brief The renderer class. Takes in a BBH type and a World type.
 */

class IndRenderer
{
public:
    IndRenderer(World *world);
    ~IndRenderer();

    /** Computes the direct illumination approaching the given surface point
      *
      * @param surf The surface point receiving illumination
      * @param wo   Points towards the viewer viewing the surface point
      */
    Radiance3 direct(std::shared_ptr<Surfel> surf, Vector3 wo);

    /** Computes the indirect illumination approaching the given surface point
      * via the impulse directions of the surface's BSDF
      *
      * @param surf The surface point receiving illumination
      * @param wo   Points towards the viewer viewing the surface point
      */
    Radiance3 impulse(std::shared_ptr<Surfel> surf, Vector3 wo, int depth);

    /**
      *
      * Computes the indirect illumination approaching the given surface point
      * via diffuse inter-object reflection, using the photon map.
      *
      * @param surf The surface point receiving illumination
      * @param wo   Points towards the viewer viewing the surface point
      */
    Radiance3 diffuse(std::shared_ptr<Surfel> surf, Vector3 wo, int depth);
    /** Gathers emissive, direct, impulse and diffuse (photon map) illumination
      * from the point under the given ray
      */
    Radiance3 trace(const Ray &ray, int depth);

      /**
      Sets the photon beam array that will be used to render the scene.
      */
    void setBeams(G3D::KDTree<PhotonBeamette> beams);
private:
    World*  m_world;
    Random  m_random;   // Random number generator
    bool    m_useGather; // Boolean to use final gather
    G3D::KDTree<PhotonBeamette> m_beams;


};

#endif // INDRENDERER_H
