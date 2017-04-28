#ifndef DIRPHOTONSCATTER_H
#define DIRPHOTONSCATTER_H
#include "photonscatter.h"
#include <G3D/G3DAll.h>

class DirPhotonScatter
    : public PhotonScatter
{
public:
    DirPhotonScatter(World * world, PhotonSettings settings);
    ~DirPhotonScatter();
    void preprocess();
    void phaseFxn(Vector3 wi, Vector3 &wo);
    Array<PhotonBeamette> getBeams();
    void makeBeams();
private:
    Array<PhotonBeamette> m_beams;
};

#endif // DIRPHOTONSCATTER_H
