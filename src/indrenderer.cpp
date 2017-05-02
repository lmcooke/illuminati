#include "indrenderer.h"

IndRenderer::IndRenderer(World* world, PhotonSettings settings):
    m_world(world),
    m_PSettings(settings)
{
    m_gatherRadius = m_PSettings.gatherRadius;
}

IndRenderer::~IndRenderer()
{
}

Radiance3 IndRenderer::direct(std::shared_ptr<Surfel> surf, Vector3 wo)
{
    Radiance3 rad;

    std::shared_ptr<Surfel> light;
    float P_light;
    float area;
    int id;

    for (int i = 0; i < m_PSettings.directSamples; ++i)
    {
        m_world->emissivePoint(m_random, light, P_light, area, id);

        Vector3 wi = light->position - surf->position;
        float dist = wi.length();
        if (dist < EPSILON)
            continue;
        wi /= dist;

        if (m_world->lineOfSight(Utils::bump(surf->position, wi, surf->geometricNormal), light->position))
        {
            rad += light->emittedRadiance(-wi) / (pif() * area)
                 * surf->finiteScatteringDensity(wi, wo)
                 * max(0.f, wi.dot(surf->shadingNormal))
                 * max(0.f, -wi.dot(light->shadingNormal))
                 / (dist * dist)
                 / P_light;
        }
    }
    return rad / m_PSettings.directSamples;
}

Radiance3 IndRenderer::impulse(std::shared_ptr<Surfel> surf, Vector3 wo, int depth)
{
    if (!--depth)
        return Radiance3::zero();

    Surfel::ImpulseArray imp;
    surf->getImpulses(PathDirection::EYE_TO_SOURCE, wo, imp);

    Radiance3 rad;
    for (int i = 0; i < imp.size(); ++i)
    {
        Ray ray(surf->position, imp[i].direction);
        Utils::bump(ray, surf);

        rad += imp[i].magnitude * trace(ray, depth);
    }

    return rad;
}

Radiance3 IndRenderer::diffuse(std::shared_ptr<Surfel> surf, Vector3 wo, int depth)
{
    // In line with the path demo, ignore diffuse interrfelection for specular
    // surfaces.
    Surfel::ImpulseArray imp;
    surf->getImpulses(PathDirection::EYE_TO_SOURCE, wo, imp);
    if (imp.size() > 0)
        return Radiance3::zero();

    Radiance3 rad;
    // If first bounce, final gather

    if (depth == m_PSettings.maxDepthScatter && m_PSettings.useFinalGather){

        for (int i=0; i < m_PSettings.gatherSamples; i++){
            // get a random sample direction from this sample point
            Vector3 wInGather = wo;
            Vector3 wOutGather = Vector3(0.f, 0.f, 0.f);
            float probabilityHint = 0.f;
            Color3 weight = Color3(1.0);
            surf->scatter(PathDirection::SOURCE_TO_EYE, wInGather, false, m_random, weight, wOutGather, probabilityHint);
            Vector3 offsetPos = Utils::bump(surf->position, wOutGather, surf->shadingNormal);
            Ray gatherRay = Ray(offsetPos, wOutGather);
            int newDepth = depth - 1;
            Radiance3 gatherColor = trace(gatherRay, newDepth).clamp(0.f, 1.f);
            Radiance3 currColor = pif() * gatherColor * weight;
            rad += currColor;
        }
        rad /= m_PSettings.gatherSamples;

    // Else, do normal diffuse calcualation
    }else{
        // Iterate through photon beams in a sphere of radius GATHER_RADIUS
        // Using cone() as kernel
        Array<PhotonBeamette> beamettes;

        m_beams->getIntersectingMembers(Sphere(surf->position, m_gatherRadius), beamettes);
        for (int i=0; i<beamettes.size(); i++){
            PhotonBeamette beam = beamettes[i];
            Vector3 closestPt = Utils::closestPointOnLine(surf->position, beam.m_start, beam.m_end);
            float dist = Vector3(surf->position - closestPt).length();
            Vector3 wi =  beam.m_end - beam.m_start;
            Radiance3 scatter = surf->finiteScatteringDensity(wi, wo.direction());
            float c = std::fmax(Utils::cone(dist, m_gatherRadius), 0.0);
            rad += beam.m_power * c * scatter/fmin(m_PSettings.numBeamettesInDir, m_beams->size());
        }
    }
    return rad;
}

Radiance3 IndRenderer::trace(const Ray &ray, int depth)
{
    Radiance3 final;

    float dist = 0;
    shared_ptr<Surfel> surf;
    m_world->intersect(ray, dist, surf);

    if (surf)
    {
        Point3 loc = ray.origin() + ray.direction() * dist;
        Point3 eye = ray.origin();
        Vector3 wo = -ray.direction();

        Radiance3 surf_radiance = surf->emittedRadiance(wo)
               + direct(surf, wo)
               + diffuse(surf, wo, depth)
               + impulse(surf, wo, depth);
        Radiance3 fogCooef = Utils::exp(dist, Radiance3(m_PSettings.attenuation));
        surf_radiance = surf_radiance*fogCooef.r + (1. - fogCooef.r)*Color3::white()*0.2;

        final += surf_radiance;
    }
    return final;
}


void IndRenderer::setBeams(std::shared_ptr<G3D::KDTree<PhotonBeamette>> beams)
{
    m_beams = beams;
}

void IndRenderer::setGatherRadius(float rad)
{
    m_gatherRadius = rad;
}

