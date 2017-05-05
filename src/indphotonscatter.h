#ifndef INDPHOTONSCATTER_H
#define INDPHOTONSCATTER_H
#include "photonscatter.h"

class IndPhotonScatter
    :public PhotonScatter
{
public:
    IndPhotonScatter(World * world, shared_ptr<PhotonSettings> settings);
    ~IndPhotonScatter();

    /** Scatters photon beams and stores them in the KdTree */
    void preprocess();

    /** Given input direction wi, generates output direction
     *  according to cosHemiRandom distribution. */
    void phaseFxn(Vector3 wi, Vector3 &wo);

    /** Distance to ray march. */
    float getRayMarchDist();

    /** Returns beams from KdTree. */
    std::shared_ptr<G3D::KDTree<PhotonBeamette>> getBeams();

    /** Clears KdTree, scatters beams, and stores them in the KdTree. */
    void makeBeams();

protected:

    std::shared_ptr<G3D::KDTree<PhotonBeamette>> m_KDTreeBeams;

private:
};

#endif // INDPHOTONSCATTER_H
