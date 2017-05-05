#include "photonscatter.h"

PhotonScatter::PhotonScatter(World * world, shared_ptr<PhotonSettings> settings):
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
    if (m_world->emitBeam(m_random, beam, surfel, numBeams, m_PSettings->beamSpread))
    {
        // Bounce the beam in the scene and insert the bounced beam into the map.
        if (beam.m_splineID >= 0){
            shootRayRecursiveCurve(beam, initBounceNum, 0); // We're always starting at beginnign of spline
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
            Vector3 prev = emittedBeam.m_start;
            Vector3 next = surfel->position;
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
    // Do some Russian Roulette stuff here.
    PhotonBeamette beam2 = PhotonBeamette();
    beam2.m_start = startPt;
    beam2.m_end = beam2.m_start + origDirection;

    // Attenuate over the distance
    float dist = length(beam2.m_start - beam2.m_end);
    beam2.m_power = power; // TODO: negative? ?
    shootRayRecursiveStraight(beam2, bounces );
}

/**
 * @brief scatterForwardCurve A beam that starts at startPt moves in direction origDirection and recurrs.
 * @param startPt
 * @param origDirection
 * @param power
 * @param spline ID
 * @param bounces
 * @param curveStep
 */
void PhotonScatter::scatterForwardCurve(Vector3 startPt, Vector3 nextDirection, Color3 power, int id, int bounces, int curveStep)
{
        PhotonBeamette beam2 = PhotonBeamette();
        beam2.m_start = startPt;
        beam2.m_end = beam2.m_start + nextDirection;
        beam2.m_splineID = id;

        // Attenuate over the distance
        float dist = length(beam2.m_start - beam2.m_end);
        beam2.m_power = power;
        curveStep += 1;
        shootRayRecursiveCurve(beam2, bounces, curveStep);
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

    PhotonBeamette beam2 = PhotonBeamette();
    beam2.m_start = startPt;
    beam2.m_end = beam2.m_start + wOut;
    beam2.m_power = power;
    shootRayRecursiveStraight(beam2, bounces);
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
    if (bounces > m_PSettings->maxDepthScatter) {
        return;
    }

    // A random distance to step forward along the beam.
    float marchDist = m_random.uniform()*getRayMarchDist();

    // Shoot the ray into the world and find the surfel it intersects with.
    float dist = inf();
    Vector3 direction =  emittedBeam.m_end - emittedBeam.m_start;

    // scatterOffSurf will recur if we hit a surface
    bool hitSurf = scatterOffSurf(emittedBeam, marchDist, dist, bounces);

    // If the marched distance is closer than the nearest surface along the same ray (ie, hitSurf is true), then we're in fog.
    // Store the ray with the point here. Then, scatter forward and out.
    if (!hitSurf && dist < inf())
    {
        Vector3 beamEndPt = emittedBeam.m_start + normalize(direction) * marchDist;
        Vector3 prev = emittedBeam.m_start;
        Vector3 next = beamEndPt;
        if(bounces > 0)
        {
            calculateAndStoreBeam(emittedBeam.m_start, beamEndPt, prev, next, m_radius, m_radius, emittedBeam.m_power);
        }

        float extinctionProb = getExtinctionProbability(marchDist); // 1 - (scat + trans)
        float remainingProb = 1.f - extinctionProb;
        float scatterProb = remainingProb * m_PSettings->scattering;
        float transProb = remainingProb - scatterProb;

        float fogEmission = 1.02f;

        float rng = m_random.uniform();

        if (rng < transProb) {
            // transmission
            scatterForward(beamEndPt, direction, emittedBeam.m_power * fogEmission, bounces);

        } else if (rng < transProb + scatterProb) {
            // scattering
            scatterIntoFog(beamEndPt, direction, emittedBeam.m_power * fogEmission, bounces);

        }
        // otherwise, extinction -> no recursion.
    }
}

/**
 * @brief shootRayRecursiveCurve Given a potential beam, defined by a spline, store it (as long as it's not going off into the abyss).
 * Then, scatter it off a surfel, into fog, and/or forward.
 * @param emittedBeam
 * @param bounces
 */
void PhotonScatter::shootRayRecursiveCurve(PhotonBeamette emittedBeam, int bounces, int curveStep)
{
    // Terminate recursion
    if (bounces > m_PSettings->maxDepthScatter) {
        return;
    }

    Array<Vector4> spline = m_world->splines()[emittedBeam.m_splineID];

    // if there isn't one more CV, return
    if (curveStep > spline.length()-2){
        return;
    }

    // A random distance to step forward along the beam.
    Vector3 startPoint = spline[curveStep].xyz();
    Vector3 endPoint = spline[curveStep+1].xyz();
    float startRad = spline[curveStep].w;
    float endRad = spline[curveStep+1].w;
    float marchDist = m_random.uniform() * length(endPoint - emittedBeam.m_start);
    Vector3 curveDirection =  normalize(endPoint - emittedBeam.m_start);
    curveDirection = (normalize(endPoint - emittedBeam.m_start) + normalize(spline[curveStep+2].xyz() - endPoint))/2.f;

    // Generate random next point based on radius
    float jitter = endRad * m_PSettings->beamSpread;

    // Generate random vector in xz plane about y axis
    // Then rotate to be oriented about curveDirect axis
    float randAngle = m_random.uniform()* M_2_PI;
    Matrix4 rot = CoordinateFrame::fromYAxis(curveDirection).toMatrix4();
    Vector4 perp = rot * Vector4(cos(randAngle), 0.0, sin(randAngle), 0.0);
    Vector3 beamEndPt = endPoint + jitter * normalize(perp.xyz());
    beamEndPt = emittedBeam.m_start + marchDist * normalize(beamEndPt - emittedBeam.m_start); // TODO: TESTING
    emittedBeam.m_end = beamEndPt;

    // Shoot the ray into the world and find the surfel it intersects with.
    float dist = inf();
    bool hitSurf = scatterOffSurf(emittedBeam, marchDist, dist, bounces);

    // If the marched distance is closer than the nearest surface along the same ray (ie, hitSurf is true), then we're in fog.
    // Store the ray with the point here. Then, scatter forward and out.
    if (!hitSurf && dist < inf())
    {
        Vector3 nextDirection = spline[curveStep+2].xyz() - beamEndPt;
        Vector3 prev = emittedBeam.m_start;
        if (curveStep > 0){
            prev = spline[curveStep-1].xyz() + -marchDist * curveDirection;
        }
        Vector3 next = spline[curveStep+2].xyz() + marchDist * curveDirection;

        startRad = max(startRad * m_radius, startRad*.5f);
        endRad = max(endRad * m_radius, endRad* .5f);

        calculateAndStoreBeam(emittedBeam.m_start, beamEndPt, prev, next, startRad, endRad, emittedBeam.m_power);

        float extinctionProb = getExtinctionProbability(marchDist); // 1 - (scat + trans)
        float remainingProb = 1.f - extinctionProb;
        float scatterProb = remainingProb * m_PSettings->scattering;
        float transProb = remainingProb - scatterProb;

        float rng = m_random.uniform();

        if (rng < transProb) {

            // transmission
            scatterForwardCurve(beamEndPt, nextDirection, emittedBeam.m_power, emittedBeam.m_splineID, bounces, curveStep);

        } else if (rng < transProb + scatterProb) {

            // scattering
            scatterIntoFog(beamEndPt, nextDirection, emittedBeam.m_power, bounces);

        } else {

        }
        // otherwise, extinction -> no recursion.
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
    if (prev.isNaN() || prev == startPt) { // beam is light source? will cut edge perpendicular to beam
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
    if (next.isNaN() || next == endPt) { // beam has no child? will cut edge perpendicular to beam
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


// returns sum of 1 - (scattering + transmission)
float PhotonScatter::getExtinctionProbability(float marchDist)
{
    return  1.f - (exp(-1.f * marchDist * m_PSettings->attenuation));
}
