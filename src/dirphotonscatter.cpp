#include "dirphotonscatter.h"

DirPhotonScatter::DirPhotonScatter(World * world)
    : PhotonScatter(world),
      m_beams()
{
    preprocess();
}

DirPhotonScatter::~DirPhotonScatter()
{
}

void DirPhotonScatter::preprocess()
{
    // Send out a beam, recursively bounce it around, and then store it in our beams array.
    for (int i=0; i<NUM_BEAMETTES; i++)
    {
        Array<PhotonBeamette> newBeams = shootRay();
        m_beams.append(newBeams);
    }
}

Array<PhotonBeamette> DirPhotonScatter::getBeams()
{
    return m_beams;
}


