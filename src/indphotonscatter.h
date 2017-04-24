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
    void phaseFxn(Vector3 wi, Vector3 &wo);
    std::shared_ptr<G3D::KDTree<PhotonBeamette>> getBeams();
protected:
    std::shared_ptr<G3D::KDTree<PhotonBeamette>> m_KDTreeBeams;
private:
};

#endif // INDPHOTONSCATTER_H
