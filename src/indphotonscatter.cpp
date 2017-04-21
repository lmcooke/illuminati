#include "indphotonscatter.h"

IndPhotonScatter::IndPhotonScatter(World * world)
    : PhotonScatter(world)
{
}

IndPhotonScatter::~IndPhotonScatter()
{
}

void IndPhotonScatter::preprocess()
{
    // Send out a beam, recursivly bounce it around, and then store it in our beams array.
    for (int i=0; i<NUM_BEAMETTES; i++)
    {
        Array<PhotonBeamette> newBeams = shootRay();
        m_beams.insert(newBeams);
    }
}

G3D::KDTree<PhotonBeamette> IndPhotonScatter::getBeams()
{
    return m_beams;
}

