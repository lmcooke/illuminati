#ifndef PHOTONSCATTER_H
#define PHOTONSCATTER_H
#include <G3D/G3DAll.h>
#include "photonbeam.h"

class PhotonScatter
{
public:
    PhotonScatter();
    ~PhotonScatter();
    /** Initializes datastructure, loops over photon beams to create and shoot them*/

protected:
    virtual void preprocess()= 0;

    /**
     * Actually shoots a single ray into the scene, accumulates an array of photons
     * to be added to the KD tree or array.
     * Returns an array of PhotonBeams
     */
    std::vector<PhotonBeam> shootRay();
    std::vector<PhotonBeam> shootRayRecursive();
};

#endif // PHOTONSCATTER_H
