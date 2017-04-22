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
        G3D::Box box = G3D::Box(b.m_start, b.m_end);
        box.getBounds(out);
    }
};
/** Define HashTrait for the photon beamettes so we can use them in the KDTree */
template<> struct HashTrait<class PhotonBeamette> {
    static size_t hashCode(const PhotonBeamette b) {
        return b.m_end.x + b.m_end.y*2 + b.m_end.x;
    }
};
/** Define equals so that it works too?*/
template<> struct EqualsTrait<class PhotonBeamette> {
    static bool equals(const PhotonBeamette b, const PhotonBeamette b1) {
            return (b.m_start == b1.m_start) && (b.m_end == b1.m_end);
    }
};
inline std::ostream & operator<<(std::ostream & Str, PhotonBeamette const & v) {
    Str << v.m_start.toString() << " -> " << v.m_end.toString();
    return Str;
}

#endif // PHOTONBEAM_H
