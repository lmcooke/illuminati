#include "app.h"
#include "world.h"

World::World() { }

World::~World() { }

void World::load(const String &path )
{

    printf("Loading scene %s...\n", path.c_str());

    Any scene;
    scene.load(path);

    // Read the model table
    debugAssert( scene.containsKey("models") );
    const Table<String, Any> models = scene["models"].table();

    // Dump it to stdout
    printf("%d model(s)\n", (int)models.size());
    for (int i = 0; i < (int)models.size(); ++i)
        printf("    %s\n", models.getKeys()[i].c_str());

    // Read the entity table
    debugAssert(scene.containsKey("entities"));
    const Table<String, Any> &entities = scene["entities"].table();

    // Parse entities
    printf("%d entities(s)\n", (int)entities.size());
    for (int i = 0; i < (int)entities.size(); ++i)
    {
        String key = entities.getKeys()[i];
        Any e = entities[key];
        String type = e.name();

        printf("    %s (%s) ... ", key.c_str(), type.c_str());

        if (type == "Camera")
        {
            AnyTableReader props(e);
            m_camera = dynamic_pointer_cast<Camera>(Camera::create(type, NULL, props));

            printf("done\n");
        }
        else if (type == "Light")
        {
            printf("ignored (only emitters are used as lights in path)\n");
        }
        else if (type == "VisibleEntity")
        {
            const Table<String, Any> &props = e.table();

            // Read the model from disk
            shared_ptr<ArticulatedModel> model =
                ArticulatedModel::create(models[props["model"]]);

            // Pose it in world space
            Vector3 pos = Vector3::zero();
            if (props.containsKey("position"))
                pos = Vector3(props["position"]);

            Array<shared_ptr<Surface>> posed;
            model->pose(posed, CFrame(pos));

            // Add it to the scene
            for (int i = 0; i < posed.size(); ++i)
                m_geometry.append(posed[i]);

            printf("done\n");
        }
        else
        {
            printf("ignored (unknown entity type)\n");
        }
    }

    // Build bounding interval hierarchy for scene geometry
    Array<Tri> triArray;

    Surface::getTris( m_geometry, m_verts, triArray );
    for (int i = 0; i < triArray.size(); ++i)
    {
        triArray[i].material()->setStorage(COPY_TO_CPU);

        // Check if this triangle emits light
        shared_ptr<Material> m = triArray[i].material();
        if (m)
        {
            shared_ptr<UniversalMaterial> mtl =
                dynamic_pointer_cast<UniversalMaterial>(m);

            if ( mtl->emissive().notBlack() ) {
                m_emit.append(triArray[i]);
            }
        }
    }

    m_tris.setContents(triArray, m_verts);

    printf( "%d light-emitting triangle(s) in scene.\n", (int) m_emit.size() );
    fflush( stdout );

}

void World::unload()
{
    m_emit.clear();
    m_geometry.clear();
    m_tris.clear();
}

shared_ptr<Camera> World::camera()
{
    return m_camera;
}

void World::emissivePoint(Random &random, shared_ptr<Surfel> &surf, float &prob, float &area)
{
    // Pick an emissive triangle uniformly at random
    int i = random.integer(0, m_emit.size() - 1);
    const Tri& tri = m_emit[i];

    // Pick a point in that triangle uniformly at random
    // http://books.google.com/books?id=fvA7zLEFWZgC&pg=PA24#v=onepage&q&f=false
    float s = random.uniform(),
          t = random.uniform(),
          sqrtT = sqrt(t),
          a = (1.f - sqrtT),
          b = (1.f - s) * sqrtT,
          c = s * sqrtT;

    Vector3 point = tri.position(m_verts, 0) * a
                  + tri.position(m_verts, 1) * b
                  + tri.position(m_verts, 2) * c;

    Vector3 normal = tri.normal(m_verts, 0) * a
                   + tri.normal(m_verts, 1) * b
                   + tri.normal(m_verts, 2) * c;
    normal = normal.direction();

    // XXX semi-hacky, slow way to get a surfel for this point
    //     there has to be a better way ...
    Ray ray(point + normal * 1e-4, -normal);
    float dist = 0;
    intersect(ray, dist, surf);

    prob = 1.f / m_emit.size() / tri.area();

    area = tri.area();
}

bool World::emit(Random &random, Photon &photon, shared_ptr<Surfel> &surf)
{
    // Select the point of emission
    shared_ptr<Surfel> light;
    float prob;
    float area;
    emissivePoint(random, light, prob, area);

    // Shoot the photon somewhere into the scene
    Vector3 dir;
    float dist;

    dir = Vector3::cosHemiRandom(light->shadingNormal, random);
    intersect(Ray(light->position + light->geometricNormal * 1e-4, dir), dist, surf);

    if (!surf) return false;

    // Store the photon
    photon.position = surf->position;
    photon.wi = -dir;
    photon.power = light->emittedRadiance(dir)
                 / NUM_PHOTONS
                 * m_emit.size();

    return true;
}

bool World::emitBeam(Random &random, PhotonBeamette &beam, shared_ptr<Surfel> &surf)
{

    // Select the point of emission
    shared_ptr<Surfel> light;
    float prob;
    float area;

    emissivePoint(random, light, prob, area);

    // Shoot the photon somewhere into the scene
    Vector3 dir;
    float dist;

    dir = Vector3::cosHemiRandom(light->shadingNormal, random);
    intersect(Ray(light->position + light->geometricNormal * 1e-4, dir), dist, surf);

    if (!surf) return false;

    // Store the beam
    beam.m_end = surf->position;
    beam.m_start = light->position;
    beam.m_power = light->emittedRadiance(dir)
                 / NUM_PHOTONS
                 * m_emit.size();
    return true;
}

void World::intersect(const Ray &ray, float &dist, shared_ptr<Surfel> &surf)
{
    TriTree::Hit hit;
    if (m_tris.intersectRay(ray, hit)) {
        dist = hit.distance;
        m_tris.sample(hit, surf);
    }
}

bool World::lineOfSight(const Vector3 &beg, const Vector3 &end)
{
    Vector3 d = end - beg;
    float dist = d.length();
    if (dist < 2e-4)
        return false;

    Ray ray = Ray::fromOriginAndDirection(beg, d / dist, 1e-4, dist - 1e-4);

    // exitOnAnyHit -- Find any intersection, not the first (early exit)
    // twoSidedTest -- Don't cull back-facing triangles from intersection test
    // static const bool exitOnAnyHit = true, twoSidedTest = true;

    TriTreeBase::Hit hit;

    return !m_tris.intersectRay(ray, hit, TriTree::DO_NOT_CULL_BACKFACES | TriTree::OCCLUSION_TEST_ONLY);
}

void World::setMatrices(RenderDevice *dev)
{
    dev->setProjectionAndCameraMatrix(m_camera->projection(), m_camera->frame());
}

void World::renderWireframe(RenderDevice *dev)
{
    dev->pushState();
    setMatrices(dev);
    Surface::renderWireframe(dev, m_geometry, Color3::white());
    dev->popState();
}
