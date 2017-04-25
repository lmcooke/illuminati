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

protected:
    /** Initializes datastructure, loops over photon beams to create and shoot them*/
    virtual void preprocess()= 0;

    /**
     * @brief phaseFxn
     * @param wi
     */
    virtual void phaseFxn(Vector3 wi, Vector3 &wo)= 0;

    /**
     * Actually shoots a single ray into the scene, accumulates an array of photons
     * to be added to the KD tree or array.
     * Returns an array of PhotonBeams
     */
    void shootRay(Array<PhotonBeamette> &beams);

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

    /* Temporary, use other calculateAndStoreBeam instead. */
    void calculateAndStoreBeam(Vector3 startPt, Vector3 endPt, Color3 power);

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
     * @brief shootRayRecursive Given a potential beam, store it (as long as it's not going off into the abyss).
     * Then, scatter it off a surfel, into fog, and/or forward.
     * @param emitBeam
     * @param bounces
     */
    void shootRayRecursive(PhotonBeamette emitBeam, int bounces);

    /**
     * @brief scatterOffSurf Standard old scattering function. Scatter and see what you hit! Only catch is
     * that if the raymarch distance is closer than what you hit, don't continue.
     * @param emitBeam
     * @param marchDist
     * @param dist
     * @param bounces
     * @return
     */
    bool scatterOffSurf(PhotonBeamette &emitBeam, float marchDist, float &dist, int bounces);
    World* m_world;
    Random m_random;   // Random number generator
    PhotonSettings m_PSettings;
    Array<PhotonBeamette> m_beams;
};

#endif // PHOTONSCATTER_H
