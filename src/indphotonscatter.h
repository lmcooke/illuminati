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
    float getRayMarchDist();
    std::shared_ptr<G3D::KDTree<PhotonBeamette>> getBeams();
    void makeBeams();
protected:
    std::shared_ptr<G3D::KDTree<PhotonBeamette>> m_KDTreeBeams;
private:
};

#endif // INDPHOTONSCATTER_H
