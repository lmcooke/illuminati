#ifndef DIRPHOTONSCATTER_H
#define DIRPHOTONSCATTER_H
#include "photonscatter.h"
#include <G3D/G3DAll.h>

class DirPhotonScatter
    : public PhotonScatter
{
public:
    DirPhotonScatter(World * world);
    ~DirPhotonScatter();
    void preprocess();
    std::vector<PhotonBeamette> getBeams();
private:
    std::vector<PhotonBeamette> m_beams;
};

#endif // DIRPHOTONSCATTER_H
