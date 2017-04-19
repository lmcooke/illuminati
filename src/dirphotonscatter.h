#ifndef DIRPHOTONSCATTER_H
#define DIRPHOTONSCATTER_H
#include "photonscatter.h"

class DirPhotonScatter
    : public PhotonScatter
{
public:
    DirPhotonScatter(World * world);
    ~DirPhotonScatter();
    void preprocess();
    std::vector<PhotonBeamette> getBeams();
private:
    std::vector<PhotonBeamette> m_beams;
};

#endif // DIRPHOTONSCATTER_H
