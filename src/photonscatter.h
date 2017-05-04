#ifndef PHOTONSCATTER_H
#define PHOTONSCATTER_H
#include <G3D/G3DAll.h>
#include "photonbeamette.h"
#include "world.h"
#include "photonsettings.h"
class PhotonScatter
{
public:
    PhotonScatter(World * world, PhotonSettings settings);
    ~PhotonScatter();
    void setRadius(float radius);

protected:
    /** Initializes datastructure, loops over photon beams to create and shoot them*/
    virtual void preprocess()= 0;

    /**
     * @brief phaseFxn
     * @param wi
     */
    virtual void phaseFxn(Vector3 wi, Vector3 &wo)= 0;


    /**
    * @brief getRayMarchDist gets the distance to march into the fog. Will match the settings for indirect scattering.
    */
    virtual float getRayMarchDist() = 0;

    /**
     * Actually shoots a single ray into the scene, accumulates an array of photons
     * to be added to the KD tree or array.
     * Returns an array of PhotonBeams
     * @param beams the array that will be populated with the beams in the scene
     * @param numBeams the total number of beams to be initially shot out
     * @param initBounceNum the initial bounce number (we want to store the first bounce for direct photon beams, but not for indirect)
     * TODO:: initBounceNum is a stupid hack. Fix it.
     */
    void shootRay(Array<PhotonBeamette> &beams, int numBeams, int initBounceNum);

    /**
     * @brief calculateAndStoreBeam calculates the power, direction, etc of the beam, and stores it in the array.
     * @param startPt   start point
     * @param endPt     end point
     * @param prev      start point of the beam that emitted this beam
     * @param next      end point of the beam following from this beam
     * @param startRad  radius at start of beam
     * @param endRad    radius at end of beam
     * @param power     power
     */
    void calculateAndStoreBeam(Vector3 startPt, Vector3 endPt, Vector3 prev, Vector3 next,
                               float startRad, float endRad, Color3 power);
    /**
     * @brief scatterIntoFog A beam that starts at startPt and, if it were to continue forward, would go in direction
     * origDirection. But it doesn't! Instead, it scatters some other direction based on the phase function.
     * @param startPt
     * @param origDirection
     * @param power
     * @param bounces
     */
    void scatterIntoFog(Vector3 startPt, Vector3 origDirection, Color3 power, int bounces);

    /**
     * @brief scatterForward A beam that starts at startPt moves in direction origDirection and recurrs.
     * @param startPt
     * @param origDirection
     * @param power
     * @param bounces
     */
    void scatterForward(Vector3 startPt, Vector3 origDirection, Color3 power, int bounces);

    /**
     * @brief scatterForwardCurve A beam that starts at startPt moves in direction origDirection and recurrs.
     * @param startPt
     * @param origDirection
     * @param power
     * @param id
     * @param bounces
     * @param curveStep
     */
    void scatterForwardCurve(Vector3 startPt, Vector3 nextDirection, Color3 power, int id, int bounces, int curveStep);

    /**
     * @brief shootRayRecursiveStraight Given a potential beam, store it (as long as it's not going off into the abyss).
     * Then, scatter it off a surfel, into fog, and/or forward.
     * @param emittedBeam
     * @param bounces
     */
    void shootRayRecursiveStraight(PhotonBeamette emittedBeam, int bounces);

    /**
     * @brief shootRayRecursiveCurve Given a potential beam, defined by a spline, store it (as long as it's not going off into the abyss).
     * Then, scatter it off a surfel, into fog, and/or forward.
     * @param emittedBeam
     * @param bounces (off surfels)
     * @param step (steps along the spline)
     */
    void shootRayRecursiveCurve(PhotonBeamette emittedBeam, int bounces, int curveStep);

    /**
     * @brief scatterOffSurf Standard old scattering function. Scatter and see what you hit! Only catch is
     * that if the raymarch distance is closer than what you hit, don't continue.
     * @param emittedBeam
     * @param marchDist
     * @param dist
     * @param bounces
     * @return
     */

    bool scatterOffSurf(PhotonBeamette &emittedBeam, float marchDist, float &dist, int bounces);

    float getExtinctionProbability(float marchDist);

    World* m_world;
    Random m_random;   // Random number generator
    PhotonSettings m_PSettings;
    Array<PhotonBeamette> m_beams;
    float m_radius;
};

#endif // PHOTONSCATTER_H
