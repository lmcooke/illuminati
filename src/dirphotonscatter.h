#ifndef DIRPHOTONSCATTER_H
#define DIRPHOTONSCATTER_H
#include "photonscatter.h"

class DirPhotonScatter
    : public PhotonScatter
{
public:
    DirPhotonScatter();
    ~DirPhotonScatter();
    void preprocess();
};

#endif // DIRPHOTONSCATTER_H
