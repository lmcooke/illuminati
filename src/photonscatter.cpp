#include "photonscatter.h"

PhotonScatter::PhotonScatter(World * world):
    m_world(world)
{
}

PhotonScatter::~PhotonScatter()
{
}


Array<PhotonBeamette> PhotonScatter::shootRay()
{
    // Array to store photon beams.
    Array<PhotonBeamette> beams;

    // Emit a photon.
    PhotonBeamette beam;
    shared_ptr<Surfel> surfel;
    m_world->emitBeam(m_random, beam, surfel);
    // Bounce the beam in the scene and insert the bounced beam into the map.
    shootRayRecursive(beam, beams, 0);
    return beams;
}

Array<PhotonBeamette> PhotonScatter::shootRayRecursive(PhotonBeamette emitBeam, Array<PhotonBeamette> &beamettes, int bounces)
{
    // Terminate recursion
    if (bounces > MAX_DEPTH) {
        return beamettes;
    }

    shared_ptr<Surfel> surfel;
    float dist = inf();

    Vector3 direction =  emitBeam.m_end - emitBeam.m_start;
    Ray ray = Ray(emitBeam.m_start, direction);
    m_world->intersect(ray, dist, surfel);

    // If intersection
    if (surfel){

        // Don't store direct light contribution
        PhotonBeamette beam = PhotonBeamette();
        beam.m_start =  emitBeam.m_start;
        beam.m_end = surfel->position + EPSILON * surfel->shadingNormal;
        beam.m_power = emitBeam.m_power;
        beamettes.push_back(beam);

        // recursive rays
        Vector3 wIn = -ray.direction();
        Vector3 wOut;
        float probabilityHint = 1.0;
        Color3 weight = Color3(1.0);
        surfel->scatter(PathDirection::SOURCE_TO_EYE, wIn, false, m_random, weight, wOut, probabilityHint);
        Color3 probability = surfel->probabilityOfScattering(PathDirection::SOURCE_TO_EYE, wIn, m_random);

        // Russian roulette termination
        float rand = m_random.uniform();
        float prob = (probability.r + probability.g + probability.b) / 3.f;
        prob = weight.average();
        if (rand < prob){
            Vector3 surfelPosOffset = Utils::bump(surfel->position, wOut, surfel->shadingNormal);
            Ray offsetRay = Ray(surfelPosOffset, wOut);
            PhotonBeamette beam2 = PhotonBeamette();
            beam2.m_start = surfel->position;
            beam2.m_end = emitBeam.m_start + offsetRay.direction();
            beam2.m_power = emitBeam.m_power * weight * probability/prob;
            shootRayRecursive(beam2, beamettes, bounces + 1);
        }
    }
    return beamettes;
}
