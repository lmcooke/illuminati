#ifndef PHOTONSCATTER_H
#define PHOTONSCATTER_H
#include <G3D/G3DAll.h>
#include "photonbeamette.h"
#include "world.h"
#include "photonsettings.h"

class PhotonScatter
{
public:
    PhotonScatter(World * world, PhotonSettings settings);
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
    PhotonSettings m_PSettings;
};

#endif // PHOTONSCATTER_H
