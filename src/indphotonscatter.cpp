#include "indphotonscatter.h"

IndPhotonScatter::IndPhotonScatter(World * world, PhotonSettings settings)
    : PhotonScatter(world, settings),
      m_beams(std::make_shared<G3D::KDTree<PhotonBeamette>>())
{
    preprocess();
}

IndPhotonScatter::~IndPhotonScatter()
{
}

void IndPhotonScatter::preprocess()
{
    // Send out a beam, recursivly bounce it around, and then store it in our beams array.
    for (int i=0; i<m_PSettings.numBeamettes; i++)
    {
        Array<PhotonBeamette> newBeams = shootRay();
        for (int i=0; i<newBeams.size(); i++)
        {
            printf("\rBuilding indirect photon beamette map ... %.2f%%", 100.f * i / m_PSettings.numBeamettes);
            m_beams->insert(newBeams[i]);
        }
    }
    m_beams->balance();
    printf("\rBuilding indirect photon beamette map ... done       \n");
}

std::shared_ptr<G3D::KDTree<PhotonBeamette>> IndPhotonScatter::getBeams()
{
    return m_beams;
}

