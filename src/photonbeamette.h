#ifndef PHOTONBEAM_H
#define PHOTONBEAM_H
#include <G3D/G3DAll.h>

class PhotonBeamette
        : public G3D::Box
{
public:
    PhotonBeamette();
    Vector3 m_start;
    Vector3 m_end;
    Vector3 m_diff1;
    Vector3 m_diff2;
    Power3 m_power;
};

#endif // PHOTONBEAM_H
