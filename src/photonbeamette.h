#ifndef PHOTONBEAM_H
#define PHOTONBEAM_H
#include <G3D/G3DAll.h>
#include <iomanip>


class PhotonBeamette
        : public G3D::Box
{
public:
    PhotonBeamette();
    Vector3 m_start;
    Vector3 m_end;
    Vector3 m_start_major;
    Vector3 m_start_minor;
    Vector3 m_end_major;
    Vector3 m_end_minor;
    bool m_isCurve;
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

inline void printvec(std::ostream & Str, const Vector3& v) {
    Str << std::setprecision(2) << "(" << v.x << ", " << v.y << ", " << v.z << ")";
}

inline std::ostream & operator<<(std::ostream & Str, PhotonBeamette const & v) {
    Str << v.m_start.toString() << " -> " << v.m_end.toString() << "\t";
//    Str << v.m_start_major.toString() << " .. " << v.m_end_major.toString() << "\t";
//    Str << v.m_start_minor.toString() << " .. " << v.m_end_minor.toString() << "\t";
    printvec(Str, v.m_start_major);
    Str << " .. ";
    printvec(Str, v.m_start_minor);
    Str << "\t";
    printvec(Str, v.m_end_major);
    Str << " .. ";
    printvec(Str, v.m_end_minor);
    Str << "\t";
    return Str;
}

#endif // PHOTONBEAM_H
