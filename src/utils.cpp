#include "utils.h"

Utils::Utils()
{
}

/** Bumps a position */
Vector3 Utils::bump(Vector3 pos, Vector3 dir, Vector3 normal)
{
    return pos + sign(dir.dot(normal)) * EPSILON * normal;
}

/** Bumps a ray in place */
void Utils::bump(Ray &ray, shared_ptr<Surfel> surf)
{
    ray.set(bump(ray.origin(), ray.direction(), surf->shadingNormal),
            ray.direction());
}

/** Computes ray.origin + t * ray.direction, bumps the point according to the
  * normal vector, and returns it.
  */
Vector3 Utils::bump(Ray &ray, float t, Vector3 normal)
{
    return bump(ray.origin() + t * ray.direction(), ray.direction(), normal);
}


/** Used for attenuation
 */
Radiance3 Utils::exp( float d, const Radiance3 &tau )
{
    return Radiance3( ::exp(-d*tau.r),
                      ::exp(-d*tau.g),
                      ::exp(-d*tau.b) );
}
