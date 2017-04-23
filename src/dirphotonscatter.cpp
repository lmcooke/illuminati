#include "dirphotonscatter.h"

DirPhotonScatter::DirPhotonScatter(World * world, PhotonSettings settings)
    : PhotonScatter(world, settings),
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
    for (int i=0; i<m_PSettings.numBeamettes; i++)
    {
        printf("\rBuilding direct photon beamette map ... %.2f%%", 100.f * i / m_PSettings.numBeamettes);
        Array<PhotonBeamette> newBeams = shootRay();
        m_beams.append(newBeams);
    }
    printf("\rBuilding direct photon beamette map ... done       \n");
}

Array<PhotonBeamette> DirPhotonScatter::getBeams()
{
    return m_beams;
}


