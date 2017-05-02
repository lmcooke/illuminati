#include "indphotonscatter.h"

IndPhotonScatter::IndPhotonScatter(World * world, PhotonSettings settings)
    : PhotonScatter(world, settings),
      m_KDTreeBeams(std::make_shared<G3D::KDTree<PhotonBeamette>>())
{
    preprocess();
}

IndPhotonScatter::~IndPhotonScatter()
{
}

void IndPhotonScatter::preprocess()
{
    Array<PhotonBeamette> tempBeamettes = Array<PhotonBeamette>();
    Array<PhotonBeamette> newBeams;
    // Send out a beam, recursivly bounce it around, and then store it in our beams array.
    for (int i=0; i<m_PSettings.numBeamettesInDir; i++)
    {
        shootRay(newBeams, m_PSettings.numBeamettesInDir, 0);
        tempBeamettes.append(newBeams);
        printf("\rBuilding indirect photon beamette map ... %.2f%%", 100.f * i / m_PSettings.numBeamettesInDir);
    }

    printf("\rBuilding indirect photon beamette map ... done       \n");
    std::cout << "started making KD tree" <<std::endl;
    m_KDTreeBeams->insert(tempBeamettes);
//    m_KDTreeBeams->balance();
    std::cout << "finished making KD tree" <<std::endl;
}

void IndPhotonScatter::phaseFxn(Vector3 wi, Vector3 &wo)
{
    float a = m_random.uniform();
    float b = m_random.uniform();
    wo.x = b*wi.y - a*wi.z;
    wo.y = -a*wi.x;
    wo.z = b*wi.x;
}

float IndPhotonScatter::getRayMarchDist()
{
    return m_PSettings.dist;
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


