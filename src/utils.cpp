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

/**
 * Computes ray.origin + t * ray.direction, bumps the point according to the
 * normal vector, and returns it.
 */
Vector3 Utils::bump(Ray &ray, float t, Vector3 normal)
{
    return bump(ray.origin() + t * ray.direction(), ray.direction(), normal);
}

/**
 * Used for attenuation
 */
Radiance3 Utils::exp( float d, const Radiance3 &tau )
{
    return Radiance3( ::exp(-d*tau.r),
                      ::exp(-d*tau.g),
                      ::exp(-d*tau.b) );
}

/** The photon map kernel (a cone filter).
  * @param dist The distance between the point being sampled and a photon
  * @return     The kernel weight for this photon
  */
float Utils::cone(float dist, float gatherRadius)
{
    static const float volume = pif() * square(gatherRadius) / 3;
    static const float normalize = 1.f / volume;

    float height = 1.f - dist / gatherRadius;
    return height * normalize;
}

/**
 * @brief Utils::closestPointOnLine
 * @param point
 * @param lineS
 * @param lineE
 * @return
 */
Vector3 Utils::closestPointOnLine(Vector3 point, Vector3 lineS, Vector3 lineE)
{
    float t = dot(point - lineS, lineS - lineE);
    return lineS + lineE*t;
}

/**
 * @brief Utils:interpolate - Catmull Rom interpolation between points p1 and p2, using neightbors p0, p3, and t position
 * @param p0
 * @param p1
 * @param p2
 * @param p3
 * @param t
 */
Vector3 Utils::interpolate(Vector3 p0, Vector3 p1, Vector3 p2, Vector3 p3, float t){

    // Using 4 points and t, find coefficents for the polynomial that defines these points

    Vector3 A = 2.f * p1;
    Vector3 B = p2 - p0;
    Vector3 C = 2.f * p0 - 5.f * p1 + 4.f * p2 - p3;
    Vector3 D = -p0 + 3.f * p1 - 3.f * p2 + p3;

    // Cubic polynomial
    Vector3 pos = A + (B * t) + (C * t * t) + (D * t * t * t);

    return pos;
}

/**
 * @brief catmullRomSpline
 * @param splinePoints
 * @param numPoints
 * @param alpha - alpha value (ranges from 0 to 1) for knot parameterization
 */
void Utils::catmullRomSpline(Array<Vector3> &splinePoints, int numPoints, float alpha){
    // TODO: don't hardcode alpha, allow 0, 0.5, or 1

    splinePoints.resize(0, false);
    if (splinePoints.length() != numPoints){
        splinePoints.resize(numPoints);
    }

    int bound = floor(1.f/numPoints);

    return;
}

/**
 * @brief calculateT
 * @param prevT
 * @param point1
 * @param point2
 */
float Utils::calculateT(float prevT, Vector3 point1, Vector3 point2, float alpha){
    float a = pow(point2.x - point1.x, 2.f) + pow(point2.y - point1.y, 2.f) + pow(point2.z - point1.z, 2.f);
    float b = pow(a, 0.5f);
    float c = pow(b, alpha);
    return (c + prevT);
}
















