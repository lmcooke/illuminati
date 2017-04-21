#ifndef INDPHOTONSCATTER_H
#define INDPHOTONSCATTER_H
#include "photonscatter.h"

class IndPhotonScatter
    :public PhotonScatter
{
public:
    IndPhotonScatter(World * world);
    ~IndPhotonScatter();
    void preprocess();
    G3D::KDTree<PhotonBeamette> getBeams();

private:
    G3D::KDTree<PhotonBeamette> m_beams;
};

#endif // INDPHOTONSCATTER_H
