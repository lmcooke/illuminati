#ifndef UTILS_H
#define UTILS_H
#include <G3D/G3DAll.h>
#define EPSILON 1e-4

class Utils
{
public:
    Utils();
    /** Bumps a ray in place */
    static void bump(Ray &ray, shared_ptr<Surfel> surf);

    /**
     * Computes ray.origin + t * ray.direction, bumps the point according to the
     * normal vector, and returns it.
     */
    static Vector3 bump(Ray &ray, float t, Vector3 normal);

    /** Bumps a position */
    static Vector3 bump(Vector3 pos, Vector3 dir, Vector3 normal);

    /**
     * Used for attenuation
     */
    static Radiance3 exp( float d, const Radiance3 &tau );

    /** The photon map kernel (a cone filter).
      * @param dist The distance between the point being sampled and a photon
      * @return     The kernel weight for this photon
      */
    static float cone(float dist, float gatherRadius);

    /**
     * @brief Utils::closestPointOnLine
     * @param point
     * @param lineS
     * @param lineE
     * @return
     */
    static Vector3 closestPointOnLine(Vector3 point, Vector3 lineS, Vector3 lineE);

};

#endif // UTILS_H
