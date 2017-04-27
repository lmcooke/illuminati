#include "photonscatter.h"

PhotonScatter::PhotonScatter(World * world, PhotonSettings settings):
    m_world(world),
    m_PSettings(settings)
{
}

PhotonScatter::~PhotonScatter()
{
}

void PhotonScatter::shootRay(Array<PhotonBeamette> &beams)
{
    m_beams.clear();
    // Emit a photon.
    PhotonBeamette beam;
    shared_ptr<Surfel> surfel;
    if (m_world->emitBeam(m_random, beam, surfel, m_PSettings.numBeamettes))
    {
        // Bounce the beam in the scene and insert the bounced beam into the map.
        shootRayRecursive(beam, 0);
    }
    beams = m_beams;
}

/**
 * @brief scatterOffSurf Standard old scattering function. Scatter and see what you hit! Only catch is
 * that if the raymarch distance is closer than what you hit, don't continue.
 * @param emitBeam
 * @param marchDist
 * @param dist
 * @param bounces
 * @return
 */
bool PhotonScatter::scatterOffSurf(PhotonBeamette &emitBeam, float marchDist, float &dist, int bounces)
{
    shared_ptr<Surfel> surfel;
    Vector3 direction =  emitBeam.m_end - emitBeam.m_start;
    Ray ray = Ray(emitBeam.m_start, direction);
    m_world->intersect(ray, dist, surfel);

    // If the surfel intersected with an object and is closer than our step size,
    if (marchDist > dist)
    {
        // Store the photon!
        if(bounces > 0)
        {
            calculateAndStoreBeam(emitBeam.m_start, surfel->position, emitBeam.m_power);
        }

        // Choose a direction to shoot the beam based on the surfel's BSDF
        Vector3 wIn = -ray.direction();
        Vector3 wOut;
        float probabilityHint = 1.0;
        Color3 weight = Color3(1.0);
        surfel->scatter(PathDirection::SOURCE_TO_EYE, wIn, false, m_random, weight, wOut, probabilityHint);
        Color3 probability = surfel->probabilityOfScattering(PathDirection::SOURCE_TO_EYE, wIn, m_random);

        // Russian roulette termination
        float rand = m_random.uniform();
        float prob = weight.average();
        if (rand < prob){
            Vector3 surfelPosOffset = Utils::bump(surfel->position, wOut, surfel->shadingNormal);
            PhotonBeamette beam2 = PhotonBeamette();
            beam2.m_start = surfelPosOffset;
            beam2.m_end = beam2.m_start + wOut;
            beam2.m_power = emitBeam.m_power * weight * probability/probability.average();
            shootRayRecursive(beam2, bounces + 1);
        }
        return true;
    }
    return false;
}

/**
 * @brief scatterForward A beam that starts at startPt moves in direction origDirection and recurrs.
 * @param startPt
 * @param origDirection
 * @param power
 * @param bounces
 */
void PhotonScatter::scatterForward(Vector3 startPt, Vector3 origDirection, Color3 power, int bounces)
{
    float rand = m_random.uniform();
    if (rand < (1 - m_PSettings.attenuation))
    {
        // Do some Russian Roulette stuff here.
        PhotonBeamette beam2 = PhotonBeamette();
        beam2.m_start = startPt;
        beam2.m_end = beam2.m_start + origDirection;
        beam2.m_power = power/(1 - m_PSettings.attenuation);
        shootRayRecursive(beam2, bounces);
    }
}

/**
 * @brief scatterIntoFog A beam that starts at startPt and, if it were to continue forward, would go in direction
 * origDirection. But it doesn't! Instead, it scatters some other direction based on the phase function.
 * @param startPt
 * @param origDirection
 * @param power
 * @param bounces
 */
void PhotonScatter::scatterIntoFog(Vector3 startPt, Vector3 origDirection, Color3 power, int bounces)
{
    Vector3 wIn = origDirection;
    Vector3 wOut;
    phaseFxn(wIn, wOut);
    float rand = m_random.uniform();
    if (rand < m_PSettings.scattering)
    {
        // Do some Russian Roulette stuff here.
        PhotonBeamette beam2 = PhotonBeamette();
        beam2.m_start = startPt;
        beam2.m_end = beam2.m_start + wOut;
        beam2.m_power = power/m_PSettings.scattering;
        shootRayRecursive(beam2, bounces);
    }
}

/**
 * @brief shootRayRecursive Given a potential beam, store it (as long as it's not going off into the abyss).
 * Then, scatter it off a surfel, into fog, and/or forward.
 * @param emitBeam
 * @param bounces
 */
void PhotonScatter::shootRayRecursive(PhotonBeamette emitBeam, int bounces)
{
    // Terminate recursion
    if (bounces > m_PSettings.maxDepthScatter) {
        return;
    }

    // A random distance to step forward along the beam.
    float marchDist = m_random.uniform()*m_PSettings.dist;

    // Shoot the ray into the world and find the surfel it intersects with.
    float dist = inf();
    Vector3 direction =  emitBeam.m_end - emitBeam.m_start;
    bool hitSurf = scatterOffSurf(emitBeam, marchDist, dist, bounces);

    // If the marched distance is closer than the nearest surface along the same ray (ie, hitSurf is true), then we're in fog.
    // Store the ray with the point here. Then, scatter forward and out.
    if (!hitSurf && dist < inf())
    {
        Vector3 beamEndPt = emitBeam.m_start + direction * marchDist;
        calculateAndStoreBeam(emitBeam.m_start, beamEndPt, emitBeam.m_power);
        scatterForward(beamEndPt, direction, emitBeam.m_power, bounces);
        scatterIntoFog(beamEndPt, direction, emitBeam.m_power, bounces);
    }
}

// Temp, later use startRad, endRad version
void PhotonScatter::calculateAndStoreBeam(Vector3 startPt, Vector3 endPt, Color3 power)
{
    PhotonBeamette beam = PhotonBeamette();
    beam.m_start =  startPt;
    beam.m_end = endPt;
    beam.m_power = power;
    m_beams.push_back(beam);
}

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
void PhotonScatter::calculateAndStoreBeam(Vector3 startPt, Vector3 endPt, Vector3 prev,
                                          Vector3 next, float startRad, float endRad, Color3 power)
{
    PhotonBeamette beam = PhotonBeamette();
    beam.m_start =  startPt;
    beam.m_end = endPt;
    beam.m_power = power;

    Vector3 vbeam = normalize(endPt - startPt);

    //start
    if (prev.isNaN()) { // beam is light source? will cut edge perpendicular to beam
        Vector3 perp = (!vbeam.x && !vbeam.y) ? Vector3(0, 1, 0) : Vector3(0, 0, 1); // any nonparallel vector
        beam.m_start_major = startRad * cross(perp, vbeam);
        beam.m_start_minor = startRad * perp;
    } else {
        Vector3 beam_prev = normalize(startPt - prev);
        beam.m_start_major = ((vbeam + beam_prev) / 2.0) * (startRad / dot(vbeam, beam_prev));
        beam.m_start_minor = startRad * beam_prev;
    }

    // end
    if (next.isNaN()) { // beam has no child? will cut edge perpendicular to beam
        Vector3 perp = (!vbeam.x && !vbeam.y) ? Vector3(0, 1, 0) : Vector3(0, 0, 1); // any nonparallel vector
        beam.m_end_minor = endRad * perp;
        beam.m_end_major = endRad * cross(perp, vbeam);
    } else {
        Vector3 beam_next = normalize(next - endPt);
        beam.m_end_major = ((vbeam + beam_next) / 2.0) * (endRad / dot(vbeam, beam_next));
        beam.m_end_minor = endRad * beam_next;
    }

    m_beams.push_back(beam);
}

