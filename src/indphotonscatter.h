#ifndef INDPHOTONSCATTER_H
#define INDPHOTONSCATTER_H
#include "photonscatter.h"

class IndPhotonScatter
    :public PhotonScatter
{
public:
    IndPhotonScatter(World * world);
    ~IndPhotonScatter();
    void preprocess();
};

#endif // INDPHOTONSCATTER_H
