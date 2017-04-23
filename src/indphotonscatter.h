#ifndef INDPHOTONSCATTER_H
#define INDPHOTONSCATTER_H
#include "photonscatter.h"

class IndPhotonScatter
    :public PhotonScatter
{
public:
    IndPhotonScatter(World * world, PhotonSettings settings);
    ~IndPhotonScatter();
    void preprocess();
    std::shared_ptr<G3D::KDTree<PhotonBeamette>> getBeams();

private:
    std::shared_ptr<G3D::KDTree<PhotonBeamette>> m_beams;
};

#endif // INDPHOTONSCATTER_H
