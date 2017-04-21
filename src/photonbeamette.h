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

/** Define BoundsTrait for the photon beamettes so we can use them in the KDTree */
template<> struct BoundsTrait<class PhotonBeamette> {
    static void getBounds(const PhotonBeamette b, G3D::AABox& out) {
        b.getBounds(out);
    }
};
/** Define HashTrait for the photon beamettes so we can use them in the KDTree */
template<> struct HashTrait<class PhotonBeamette> {
    static size_t hashCode(const PhotonBeamette b) {
        return (b.m_start + b.m_end).average();
    }
};

inline std::ostream & operator<<(std::ostream & Str, PhotonBeamette const & v) {
    Str << v.m_start.toString() << " -> " << v.m_end.toString();
    return Str;
}

#endif // PHOTONBEAM_H
