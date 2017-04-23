#include "photonscatter.h"

PhotonScatter::PhotonScatter(World * world, PhotonSettings settings):
    m_world(world),
    m_PSettings(settings)
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
    if (m_world->emitBeam(m_random, beam, surfel, m_PSettings.numBeamettes))
    {
        // Bounce the beam in the scene and insert the bounced beam into the map.
        shootRayRecursive(beam, beams, 0);
    }
    return beams;
}

Array<PhotonBeamette> PhotonScatter::shootRayRecursive(PhotonBeamette emitBeam, Array<PhotonBeamette> &beamettes, int bounces)
{
    // Terminate recursion
    if (bounces > m_PSettings.maxDepth) {
        return beamettes;
    }

    shared_ptr<Surfel> surfel;
    float dist = inf();

    Vector3 direction =  emitBeam.m_end - emitBeam.m_start;
    Ray ray = Ray(emitBeam.m_start, direction);
    m_world->intersect(ray, dist, surfel);

    // If intersection
    if (surfel){
        // Store the photon!
        if(bounces > 0)
        {
            PhotonBeamette beam = PhotonBeamette();
            beam.m_start =  emitBeam.m_start;
            beam.m_end = surfel->position + m_PSettings.epsilon * surfel->shadingNormal;
            beam.m_power = emitBeam.m_power;
            beamettes.push_back(beam);
        }

        // Recursive rays
        Vector3 wIn = -ray.direction();
        Vector3 wOut;
        float probabilityHint = 1.0;
        Color3 weight = Color3(1.0);
        surfel->scatter(PathDirection::SOURCE_TO_EYE, wIn, false, m_random, weight, wOut, probabilityHint);
        Color3 probability = surfel->probabilityOfScattering(PathDirection::SOURCE_TO_EYE, wIn, m_random);

        // Russian roulette termination
        float rand = m_random.uniform();
        float prob = probability.average();
        prob = weight.average();
        if (rand < prob){
            Vector3 surfelPosOffset = Utils::bump(surfel->position, wOut, surfel->shadingNormal);
            PhotonBeamette beam2 = PhotonBeamette();
            beam2.m_start = surfelPosOffset;
            beam2.m_end = beam2.m_start + wOut;
            beam2.m_power = emitBeam.m_power * weight * probability/prob;
            shootRayRecursive(beam2, beamettes, bounces + 1);
        }
    }
    return beamettes;
}
