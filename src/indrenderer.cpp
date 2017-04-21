#include "indrenderer.h"

IndRenderer::IndRenderer(World* world):
    m_world(world),
    m_useGather(false)
{
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

    for (int i = 0; i < DIRECT_SAMPLES; ++i)
    {
        m_world->emissivePoint(m_random, light, P_light, area);

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
    return rad / DIRECT_SAMPLES;
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

/** The photon map kernel (a cone filter).
  * @param dist The distance between the point being sampled and a photon
  * @return     The kernel weight for this photon
  */
static float cone(float dist)
{
    static const float volume = pi() * square(GATHER_RADIUS) / 3;
    static const float normalize = 1.f / volume;

    float height = 1.f - dist / GATHER_RADIUS;
    return height * normalize;
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
    if (depth == MAX_DEPTH && m_useGather){
        for (int i=0; i < GATHER_SAMPLES; i++){
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
        rad /= GATHER_SAMPLES;

    // Else, do normal diffuse calcualation
    }else{
        // Iterate through photon beams in a sphere of radius GATHER_RADIUS
        // Using cone() as kernel
        Array<PhotonBeamette> beamettes;
        m_beams.getIntersectingMembers(Sphere(surf->position, GATHER_RADIUS), beamettes);

        for (int i=0; i<beamettes.size(); i++){
            PhotonBeamette beam = beamettes[i];
            float dist = Vector3(surf->position - beam.m_start).length();
            Vector3 wi =  beam.m_end - beam.m_start;
            Radiance3 scatter = surf->finiteScatteringDensity(wi, wo.direction());

            rad += beam.m_power * cone(dist) * scatter;
        }
        if (rad.average() > 0)
        {
            std::cout << rad.average() <<std::endl;
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
//               + direct(surf, wo)
               + diffuse(surf, wo, depth)
               + impulse(surf, wo, depth);
//        surf_radiance *= exp(dist, Radiance3(m_PSettings.attenuation));
//        surf_radiance *= Utils::exp(dist, Radiance3(0.0));
        final += surf_radiance;
    }

    return final;
}


void IndRenderer::setBeams(G3D::KDTree<PhotonBeamette> beams)
{
    m_beams = beams;
}
