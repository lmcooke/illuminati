#include "dirphotonscatter.h"
#define NUM_BEAMETTES 500 /* How many beams to scatter into the scene */

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
    // Send out a beam, recursivly bounce it around, and then store it in our beams array.
    for (int i=0; i<NUM_BEAMETTES; i++)
    {
        std::vector <PhotonBeamette> newBeams = shootRay();
        m_beams.insert(std::end(m_beams), std::begin(newBeams), std::end(newBeams));
    }
    std::cout << m_beams.size() <<std::endl;
}

std::vector<PhotonBeamette> DirPhotonScatter::getBeams()
{
    return m_beams;
}


