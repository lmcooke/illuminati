#ifndef PHOTONSCATTER_H
#define PHOTONSCATTER_H
#include <G3D/G3DAll.h>
#include "photonbeamette.h"
#include "world.h"

#define MAX_DEPTH 4
#define EPSILON 1e-4
#define NUM_BEAMETTES 500 /* How many beams to scatter into the scene */

class PhotonScatter
{
public:
    PhotonScatter(World * world);
    ~PhotonScatter();

protected:
    /** Initializes datastructure, loops over photon beams to create and shoot them*/
    virtual void preprocess()= 0;

    /**
     * Actually shoots a single ray into the scene, accumulates an array of photons
     * to be added to the KD tree or array.
     * Returns an array of PhotonBeams
     */
    Array<PhotonBeamette> shootRay();
    Array<PhotonBeamette> shootRayRecursive(PhotonBeamette emitBeam, Array<PhotonBeamette> &beamettes, int bounces);
    World* m_world;
    Random m_random;   // Random number generator

};

#endif // PHOTONSCATTER_H
