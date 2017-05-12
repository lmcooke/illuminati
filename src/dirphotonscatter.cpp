#include "dirphotonscatter.h"

#include <math.h>

DirPhotonScatter::DirPhotonScatter(World * world, shared_ptr<PhotonSettings> settings)
    : PhotonScatter(world, settings),
      m_beams()
{
}

DirPhotonScatter::~DirPhotonScatter()
{
}

void DirPhotonScatter::preprocess()
{
    Array<PhotonBeamette> newBeams;
    // Send out a beam, recursively bounce it around, and then store it in our beams array.
    for (int i=0; i<m_PSettings->numBeamettesDir; i++)
    {
        // Stores from first bounce
        shootRay(newBeams, m_PSettings->numBeamettesDir, 1);
        m_beams.append(newBeams);
    }
}

void DirPhotonScatter::phaseFxn(Vector3 wi, Vector3 &wo)
{
    float power = 1.f;
    wo = Vector3::cosPowHemiRandom(-wi, power, m_random);
}

Array<PhotonBeamette> DirPhotonScatter::getBeams()
{
    return m_beams;
}

void DirPhotonScatter::makeBeams()
{
    m_beams.clear();
    preprocess();
}


float DirPhotonScatter::getRayMarchDist()
{
//    return 5;
//    return .1f;
    return m_PSettings->dist;
}

