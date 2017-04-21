#ifndef UTILS_H
#define UTILS_H
#include <G3D/G3DAll.h>
#define EPSILON 1e-4

class Utils
{
public:
    Utils();
    static void bump(Ray &ray, shared_ptr<Surfel> surf);
    static Vector3 bump(Ray &ray, float t, Vector3 normal);
    static Vector3 bump(Vector3 pos, Vector3 dir, Vector3 normal);
    static Radiance3 exp( float d, const Radiance3 &tau );
};

#endif // UTILS_H
