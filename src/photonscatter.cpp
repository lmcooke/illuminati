#include "photonscatter.h"

PhotonScatter::PhotonScatter(World * world, PhotonSettings settings):
    m_world(world),
    m_PSettings(settings),
    m_radius(1)
{
}

PhotonScatter::~PhotonScatter()
{
}

void PhotonScatter::shootRay(Array<PhotonBeamette> &beams, int numBeams, int initBounceNum)
{
    m_beams.clear();
    // Emit a photon.
    PhotonBeamette beam;
    shared_ptr<Surfel> surfel;
    if (m_world->emitBeam(m_random, beam, surfel, numBeams, m_PSettings.beamSpread))
    {
        // Bounce the beam in the scene and insert the bounced beam into the map.
        if (beam.m_splineID > 0){
            shootRayRecursiveCurve(beam, initBounceNum);
        }else{
            shootRayRecursiveStraight(beam, initBounceNum);
        }
    }
    beams = m_beams;
}

/**
 * @brief scatterOffSurf Standard old scattering function. Scatter and see what you hit! Only catch is
 * that if the raymarch distance is closer than what you hit, don't continue.
 * @param emittedBeam
 * @param marchDist
 * @param dist
 * @param bounces
 * @return
 */
bool PhotonScatter::scatterOffSurf(PhotonBeamette &emittedBeam, float marchDist, float &dist, int bounces)
{
    shared_ptr<Surfel> surfel;
    Vector3 direction =  emittedBeam.m_end - emittedBeam.m_start;
    Ray ray = Ray(emittedBeam.m_start, direction);
    m_world->intersect(ray, dist, surfel);

    // If the surfel intersected with an object and is closer than our step size,
    if (marchDist > dist)
    {
        // Store the photon!
        if(bounces > 0)
        {
            Vector3 prev = -(emittedBeam.m_start - surfel->position) * 1.1;
            Vector3 next = (emittedBeam.m_start - surfel->position) * 1.1;
            calculateAndStoreBeam(emittedBeam.m_start,  surfel->position, prev, next, m_radius, m_radius, emittedBeam.m_power);
        }

        // Choose a direction to shoot the beam based on the surfel's BSDF
        Vector3 wIn = -ray.direction();
        Vector3 wOut;
        float probabilityHint = 1.0;
        Color3 weight = Color3(1.0);
        surfel->scatter(PathDirection::SOURCE_TO_EYE, wIn, false, m_random, weight, wOut, probabilityHint);
        Color3 probability = surfel->probabilityOfScattering(PathDirection::SOURCE_TO_EYE, wIn, m_random);
        weight = weight.clamp(0.0, 1.0);

        // Russian roulette termination
        float rand = m_random.uniform();
        float prob = weight.average();
        if (rand < prob){
            Vector3 surfelPosOffset = Utils::bump(surfel->position, wOut, surfel->shadingNormal);
            PhotonBeamette beam2 = PhotonBeamette();
            beam2.m_start = surfelPosOffset;
            beam2.m_end = beam2.m_start + wOut;
            beam2.m_power = emittedBeam.m_power * weight;
            shootRayRecursiveStraight(beam2, bounces + 1);
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
    if (rand < (1 - fmax(m_PSettings.attenuation, 0.01)))
    {
        // Do some Russian Roulette stuff here.
        PhotonBeamette beam2 = PhotonBeamette();
        beam2.m_start = startPt;
        beam2.m_end = beam2.m_start + origDirection;

        // Attenuate over the distance
        float dist = length(beam2.m_start - beam2.m_end);
        beam2.m_power = power/(1 - fmax(m_PSettings.attenuation, 0.01));
        shootRayRecursiveStraight(beam2, bounces );
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
        beam2.m_power = power/fmax(m_PSettings.scattering, 0.001);
        shootRayRecursiveStraight(beam2, bounces + 1);
    }
}

/**
 * @brief shootRayRecursiveStraight Given a potential beam, store it (as long as it's not going off into the abyss).
 * Then, scatter it off a surfel, into fog, and/or forward.
 * @param emittedBeam
 * @param bounces
 */
void PhotonScatter::shootRayRecursiveStraight(PhotonBeamette emittedBeam, int bounces)
{
    // Terminate recursion
    if (bounces > m_PSettings.maxDepthScatter) {
        return;
    }

    // A random distance to step forward along the beam.
    float marchDist = m_random.uniform()*getRayMarchDist();

    // Shoot the ray into the world and find the surfel it intersects with.
    float dist = inf();
    Vector3 direction =  emittedBeam.m_end - emittedBeam.m_start;
    bool hitSurf = scatterOffSurf(emittedBeam, marchDist, dist, bounces);

    // If the marched distance is closer than the nearest surface along the same ray (ie, hitSurf is true), then we're in fog.
    // Store the ray with the point here. Then, scatter forward and out.
    if (!hitSurf && dist < inf())
    {

        Vector3 beamEndPt = emittedBeam.m_start + direction * marchDist;
        Vector3 prev = -(emittedBeam.m_start - beamEndPt) * 1.1;
        Vector3 next = (emittedBeam.m_start - beamEndPt) * 1.1;
        if(bounces > 0)
        {
            calculateAndStoreBeam(emittedBeam.m_start, beamEndPt, prev, next, m_radius, m_radius, emittedBeam.m_power);
        }

        scatterIntoFog(beamEndPt, direction, emittedBeam.m_power, bounces);
        scatterForward(beamEndPt, direction, emittedBeam.m_power, bounces);
    }
}

/**
 * @brief shootRayRecursiveCurve Given a potential beam, defined by a spline, store it (as long as it's not going off into the abyss).
 * Then, scatter it off a surfel, into fog, and/or forward.
 * @param emittedBeam
 * @param bounces
 */
void PhotonScatter::shootRayRecursiveCurve(PhotonBeamette emittedBeam, int bounces)
{
    // Terminate recursion
    if (bounces > m_PSettings.maxDepthScatter) {
        return;
    }

    // A random distance to step forward along the beam.
    float marchDist = m_random.uniform()*getRayMarchDist();

    // Shoot the ray into the world and find the surfel it intersects with.
    float dist = inf();
    Vector3 direction =  emittedBeam.m_end - emittedBeam.m_start;
    bool hitSurf = scatterOffSurf(emittedBeam, marchDist, dist, bounces);

    // If the marched distance is closer than the nearest surface along the same ray (ie, hitSurf is true), then we're in fog.
    // Store the ray with the point here. Then, scatter forward and out.
    if (!hitSurf && dist < inf())
    {

        Vector3 beamEndPt = emittedBeam.m_start + direction * marchDist;
        Vector3 prev = -(emittedBeam.m_start - beamEndPt) * 1.1;
        Vector3 next = (emittedBeam.m_start - beamEndPt) * 1.1;
        if(bounces > 0)
        {
            calculateAndStoreBeam(emittedBeam.m_start, beamEndPt, prev, next, m_radius, m_radius, emittedBeam.m_power);
        }

        scatterIntoFog(beamEndPt, direction, emittedBeam.m_power, bounces);
        scatterForward(beamEndPt, direction, emittedBeam.m_power, bounces);
    }
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

    // We want the total light contribution to the screen from a single beam to be constant
    // regardless of the width of the beam.
    beam.m_power = power/(startRad + endRad)/2;
    Vector3 vbeam = normalize(endPt - startPt);

    //start
    if (prev.isNaN()) { // beam is light source? will cut edge perpendicular to beam
        Vector3 perp = (!vbeam.x && !vbeam.y) ? Vector3(0, 1, 0) : Vector3(0, 0, 1); // any nonparallel vector
        beam.m_start_major = startRad * normalize(cross(perp, vbeam));
        beam.m_start_minor = startRad * normalize(cross(vbeam, beam.m_start_major));
    } else {
        Vector3 beam_prev = normalize(startPt - prev);
        Vector3 majdir = normalize((vbeam - beam_prev) / 2.0);
        float startr = startRad;
        if (dot(vbeam, majdir) != 0) {
            startr = startr / dot(vbeam, majdir);
        }
        beam.m_start_major = startr * majdir;
        beam.m_start_minor = startRad * normalize(cross(vbeam, beam_prev));
    }

    // end
    if (next.isNaN()) { // beam has no child? will cut edge perpendicular to beam
        Vector3 perp = (!vbeam.x && !vbeam.y) ? Vector3(0, 1, 0) : Vector3(0, 0, 1); // any nonparallel vector
        beam.m_end_major = endRad * normalize(cross(perp, vbeam));
        beam.m_end_minor = endRad * normalize(cross(vbeam, beam.m_end_major));
    } else {
        Vector3 beam_next = normalize(next - endPt);
        Vector3 majdir = normalize((-vbeam + beam_next) / 2.0);
        float endr = endRad;
        if (dot(vbeam, majdir) != 0) {
            endr = endr / dot(vbeam, majdir);
        }
        beam.m_end_major = endr * majdir;
        beam.m_end_minor = endRad * normalize(cross(vbeam, beam_next));
    }
    m_beams.push_back(beam);
}

void PhotonScatter::setRadius(float radius)
{
    m_radius = radius;
}
