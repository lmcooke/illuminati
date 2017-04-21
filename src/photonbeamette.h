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
    Vector3 m_diff1; //TODO needs position and direction for differentials?
    Vector3 m_dir1;
    Vector3 m_diff2;
    Vector3 m_dir2;
    Power3 m_power;
};

inline std::ostream & operator<<(std::ostream & Str, PhotonBeamette const & v) {
    Str << v.m_start.toString() << " -> " << v.m_end.toString();
    return Str;
}

#endif // PHOTONBEAM_H
