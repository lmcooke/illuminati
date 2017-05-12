#include "indphotonscatter.h"

IndPhotonScatter::IndPhotonScatter(World * world, shared_ptr<PhotonSettings> settings)
    : PhotonScatter(world, settings),
      m_KDTreeBeams(std::make_shared<G3D::KDTree<PhotonBeamette>>())
{
}

IndPhotonScatter::~IndPhotonScatter()
{
}

void IndPhotonScatter::preprocess()
{
    Array<PhotonBeamette> tempBeamettes = Array<PhotonBeamette>();
    Array<PhotonBeamette> newBeams;
    // Send out a beam, recursively bounce it around, and then store it in our beams array.
    for (int i=0; i<m_PSettings->numBeamettesInDir; i++)
    {
        // we won't start storing rays until after initial bounce - initial bounce = 0
        shootRay(newBeams, m_PSettings->numBeamettesInDir, 0);
        tempBeamettes.append(newBeams);
//        printf("\rBuilding indirect photon beamette map ... %.2f%%", 100.f * i / m_PSettings->numBeamettesInDir);
    }

//    printf("\rBuilding indirect photon beamette map ... done       \n");
    m_KDTreeBeams->insert(tempBeamettes);
}

void IndPhotonScatter::phaseFxn(Vector3 wi, Vector3 &wo)
{
    wo = Vector3::cosHemiRandom(-wi);
}

float IndPhotonScatter::getRayMarchDist()
{
    return m_PSettings->dist;
}

std::shared_ptr<G3D::KDTree<PhotonBeamette>> IndPhotonScatter::getBeams()
{
    return m_KDTreeBeams;
}

void IndPhotonScatter::makeBeams()
{
    m_KDTreeBeams->clear();
    preprocess();
}


