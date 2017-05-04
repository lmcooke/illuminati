#include "app.h"
#include "world.h"

World::World()
    : m_splines(Array<Array<Vector4>>())
{ }

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

            // TESTING camera movement
//            const CFrame& cframe = m_camera->frame();

//            float x;
//            float y;
//            float z;
//            float yaw;
//            float pitch;
//            float roll;

//            cframe.getXYZYPRDegrees(x, y, z, yaw, pitch, roll);

//            std::cout << "x: " << x << std::endl;
//            std::cout << "y: " << y << std::endl;
//            std::cout << "z : " << z << std::endl;
//            std::cout << "yaw : " << yaw << std::endl;
//            std::cout << "pitch : " << pitch << std::endl;
//            std::cout << "roll : " << roll << std::endl;

//            const CFrame newCframe = CFrame::fromXYZYPRDegrees(.5, 0.75, 3, 20, 0, 0 );
//            m_camera->setFrame(newCframe);

            printf("done\n");
        }
        else if (type == "Light")
        {
//            printf("ignored (only emitters are used as lights in path)\n");
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
        else if (type == "SplineLight")
        {
            const Table<String, Any> &props = e.table();
            const ArticulatedModel::Specification& spec = models[props["model"]];
            String filename = FileSystem::resolve(spec.filename);
            printf("%s ... ", spec.filename.c_str());

            Array<shared_ptr<ArticulatedModel>> splineArray= createSplineModel(filename);
            shared_ptr<ArticulatedModel> spline = splineArray[0];
            shared_ptr<ArticulatedModel> emitter = splineArray[1];

            // Pose it in world space
            Vector3 pos = Vector3::zero();

            if (props.containsKey("position"))
                pos = Vector3(props["position"]);

            Array<shared_ptr<Surface>> posedGeo;
            Array<shared_ptr<Surface>> posedSpline;
            spline->pose(posedSpline, CFrame(pos));
            emitter->pose(posedGeo, CFrame(pos));

            // Add it to the scene
           /* for (int i = 0; i < posed.size(); ++i)
                m_spline_geometry.append(posed[i]); // TODO keep separate spline list
                //m_geometry.append(posed[i]); // just for testing spline accuracy
                        */
            if (m_PSettings.renderSplines){
                m_geometry.append(posedSpline);
            }

            m_geometry.append(posedGeo);

            printf("done\n");
        }
        else
        {
            printf("ignored (unknown entity type)\n");
        }
    }

    // Build bounding interval hierarchy for scene geometry
    Array<Tri> triArray;

    Surface::getTris(m_geometry, m_verts, triArray);
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

                std::string name = triArray[i].surface()->name().c_str();
                int id = -1;
                if (name.find("spline") != std::string::npos) {
                    name = name[0];
                    id = std::atoi(name.c_str());
                }
                Emitter emitter = Emitter(id, triArray[i]);
                m_emit.append(emitter);
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
    m_splines.clear();
}

Array<shared_ptr<Surface>> World::geometry()
{
    return m_geometry;
}


shared_ptr<Camera> World::camera()
{

    return m_camera;
}

void World::emissivePoint(Random &random, shared_ptr<Surfel> &surf, float &prob, float &area, int &id)
{
    // Pick an emissive triangle uniformly at random
    int i = random.integer(0, m_emit.size() - 1);
    const Tri& tri = m_emit[i].tri();
    id = m_emit[i].index();

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

bool World::emitBeam(Random &random, PhotonBeamette &beam, shared_ptr<Surfel> &surf, int totalPhotons, float beamSpread)
{
    // Select the point of emission
    shared_ptr<Surfel> light;
    float prob;
    float area;
    int id;

    World::emissivePoint(random, light, prob, area, id);
    // Shoot the photon beamette somewhere into the scene
    Vector3 dir;
    float dist;

    dir = Vector3::cosPowHemiRandom(light->shadingNormal, 1./beamSpread, random);
    intersect(Ray(light->position + light->geometricNormal * 1e-4, dir), dist, surf);

    if (!surf) return false;

    // Store the beam information
    beam.m_end = surf->position;
    beam.m_start = light->position;
    beam.m_power = light->emittedRadiance(dir)* m_emit.size();
    beam.m_splineID = id;
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

Array<shared_ptr<ArticulatedModel>> World::createSplineModel(const String& str) {
    const shared_ptr<ArticulatedModel>& modelBody = ArticulatedModel::createEmpty("splineModel");
    std::string                 nameRoot      = std::to_string(m_splines.length()) + std::string("spline");
    String                      name          = String(nameRoot.c_str());

    ArticulatedModel::Part*     partBody      = modelBody->addPart(name + "_rootBody");
    ArticulatedModel::Geometry* geometryBody  = modelBody->addGeometry(name + "_geomBody");
    ArticulatedModel::Mesh*     meshBody      = modelBody->addMesh(name + "_meshBody", partBody, geometryBody);

    const shared_ptr<ArticulatedModel> &modelEmitter = ArticulatedModel::createEmpty("splineModel");

    ArticulatedModel::Part*     partEmitter      = modelEmitter->addPart(name + "_rootEmitter");
    ArticulatedModel::Geometry* geometryEmitter  = modelEmitter->addGeometry(name + "_geomEmitter");
    ArticulatedModel::Mesh*     meshEmitter      = modelEmitter->addMesh(name + "_meshEmitter", partEmitter, geometryEmitter);

    int npts = 0;
    int slices = 8;
    float arc = 2.0 * pif() / slices;

    Array<CPUVertexArray::Vertex>& vertexArray = geometryBody->cpuVertexArray.vertex;
    Array<int>& indexArray = meshBody->cpuIndexArray;

    Array<CPUVertexArray::Vertex>& vertexArrayEmitter = geometryEmitter->cpuVertexArray.vertex;
    Array<int>& indexArrayEmitter = meshEmitter->cpuIndexArray;

    Array<Vector4> raw_spline = Array<Vector4>();

    /* text parsing and vertex construction */

    const std::string st = str.c_str();
    std::ifstream infile(st);
    std::string line;
    Vector3 pt1 = Vector3(0, 0, 0);
    Vector3 pt2 = Vector3(0, 0, 0);
    Vector3 pt3 = Vector3(0, 0, 0);
    float w2;
    float w3;
    Vector3 diff;
    Vector3 prev_diff = Vector3(0, 0, 1);
    bool comment;
    bool has_color;

    Color3 c = Color3(Vector3(1.0, 0.0, 0.0));//Color3::one();

    while (std::getline(infile, line)) {
        pt1 = pt2;
        pt2 = pt3;
        w2 = w3;
        std::istringstream iss(line);
        comment = iss.peek() == '#';
        has_color = iss.peek() == '*';
        if (has_color) {
            iss.ignore(1, ' ');
            if (!(iss >> c[0] >> c[1] >> c[2])) {
                printf("spline file has invalid color, using default color... ");
            }
            printf("color %s \n", c.toString().c_str());
        } else {
            if (!comment) {
                if (!(iss >> pt3[0] >> pt3[1] >> pt3[2] >> w3)) {
                    throw std::invalid_argument( "spline file must consist of four values: x y z radius" );
                }
                raw_spline.append(Vector4(pt3[0], pt3[1], pt3[2], w3));
            }
            if (npts > 0) {
                prev_diff = diff;
                if (npts == 1) { // first control point
                    diff = normalize(pt3 - pt2);
                } else if (comment) { // last control point
                    diff = normalize(pt2 - pt1);
                } else {
                    diff = normalize (normalize(pt2 - pt1) + normalize(pt3 - pt2) );
                }

//                CFrame yax = CoordinateFrame::fromYAxis(diff);
                Matrix4 trans = Matrix4::translation(pt2);
                Matrix4 rot = CoordinateFrame::fromYAxis(diff).toMatrix4();
                //Matrix4 yaw = Matrix4::yawDegrees();
                Matrix4 transrot = trans * rot;
                for (int a = 0; a < slices; a++) {
                    CPUVertexArray::Vertex& v = vertexArray.next();
                    Vector4 tmp = transrot *
                            Vector4(w2 * cos(a * arc),
                                    0.0,
                                    w2 * sin(a * arc),
                                    1.0);
                    v.position = Vector3(tmp.x, tmp.y, tmp.z);
                    v.normal  = Vector3::nan();
                    v.tangent = Vector4::nan();

                    if (npts == 1){ // If first control point
                        // Center point
                        if (a == 0){
                            CPUVertexArray::Vertex& v = vertexArrayEmitter.next();
                            Vector4 tmp = Vector4(pt2.x, pt2.y, pt2.z, 1.0);
                            v.position = Vector3(tmp.x, tmp.y, tmp.z);
                            v.normal = Vector3::nan();
                            v.tangent = Vector4::nan();
                        }

                        CPUVertexArray::Vertex& v = vertexArrayEmitter.next();
                        Vector4 tmp = transrot * Vector4(w2 * cos(a * arc),
                                                         0.0,
                                                         w2 * sin(a * arc),
                                                         1.0);
                        v.position = Vector3(tmp.x, tmp.y, tmp.z);
                        v.normal  = Vector3::nan();
                        v.tangent = Vector4::nan();
                    }
                }
            }
            npts++;
        }
    }
    npts--;

    assert(npts == raw_spline.size());

    // Assign a material
    UniversalMaterial::Specification specBody = UniversalMaterial::Specification();
    specBody.setLambertian(Texture::Specification(Color4(c, 1.0)));
    meshBody->material = UniversalMaterial::create(specBody);

    UniversalMaterial::Specification specEmissive = UniversalMaterial::Specification();
    specEmissive.setEmissive(Texture::Specification(Color4(c, 1.0)));
    meshEmitter->material = UniversalMaterial::create(specEmissive);

    /* face construction */

    float f[4] = {0,0,0,0};
    for (int j = 0; j < npts - 1; j++) {
        for (int i = 0; i < slices; i++) {
            f[0] = j * slices + (i % slices);
            f[3] = j * slices + ((i + 1) % slices);
            f[1] = f[0] + slices;
            f[2] = f[3] + slices;
            debugAssert(f[4] < slices * npts);
            indexArray.append(
                f[0], f[1], f[2],
                f[0], f[2], f[3]);
        }
    }

    /* emitter face construction*/
    float fEmit[3] = {0, 0, 0};
    for (int i = 0; i < slices; i++){
        fEmit[0] = 0;
        fEmit[1] = i % slices + 1;
        fEmit[2] = ((i+1) % slices) + 1;
        indexArrayEmitter.append(fEmit[2], fEmit[1], fEmit[0]);
    }

    // Tell the ArticulatedModel to generate bounding boxes, GPU vertex arrays,
    // normals and tangents automatically. We already ensured correct
    // topology, so avoid the vertex merging optimization.
    ArticulatedModel::CleanGeometrySettings geometrySettings;
    geometrySettings.allowVertexMerging = false;
    modelBody->cleanGeometry(geometrySettings);

    ArticulatedModel::CleanGeometrySettings geometrySettingsEmitter;
    geometrySettingsEmitter.allowVertexMerging = false;
    geometrySettingsEmitter.forceComputeNormals = true;
    geometrySettingsEmitter.forceComputeTangents = true;
    modelEmitter->cleanGeometry(geometrySettingsEmitter);

    m_splines.append(raw_spline);

    Array<shared_ptr<ArticulatedModel>> out = Array<shared_ptr<ArticulatedModel>>(modelBody, modelEmitter);
    return out;
}


Array<PhotonBeamette> World::visualizeSplines() {
    Array<PhotonBeamette> beams = Array<PhotonBeamette>();
    for (Array<Vector4> spline : m_splines) {
        Vector3 prev_major = Vector3::nan();
        Vector3 prev_minor = Vector3::nan();
        Array<Vector4>::iterator it;
        Vector4 v;
        int i = 1;
        int n = spline.size();
        for (it = std::next(spline.begin()); it != spline.end(); ++it, ++i ) {
            v = *it;
            Vector4 prev = *std::prev(it);
            Vector4 next = *std::next(it);
            float startRad = prev.w;
            float endRad = v.w;

            PhotonBeamette pb = PhotonBeamette();
            pb.m_end = v.xyz();
            pb.m_start = prev.xyz();

            Vector3 vbeam = normalize(pb.m_end - pb.m_start);

            //start
            if (i == 1) { // beam is light source? will cut edge perpendicular to beam
                Vector3 perp = (!vbeam.x && !vbeam.y) ? Vector3(0, 1, 0) : Vector3(0, 0, 1); // any nonparallel vector
                pb.m_start_major = startRad * normalize(cross(perp, vbeam));
                pb.m_start_minor = startRad * normalize(cross(vbeam, pb.m_start_major));
            } else {
                pb.m_start_major = prev_major;
                pb.m_start_minor = prev_minor;
            }

            // end
            if (i == n - 1) { // beam has no child? will cut edge perpendicular to beam
                Vector3 perp = (!vbeam.x && !vbeam.y) ? Vector3(0, 1, 0) : Vector3(0, 0, 1); // any nonparallel vector
                pb.m_end_major = endRad * normalize(cross(perp, vbeam));
                pb.m_end_minor = endRad * normalize(cross(vbeam, pb.m_end_major));
            } else {
                Vector3 beam_next = normalize(next.xyz() - pb.m_end);
                Vector3 majdir = normalize((-vbeam + beam_next) / 2.0);
                float endr = endRad;
                if (dot(vbeam, majdir) != 0) {
                    endr = endr / dot(vbeam, majdir);
                }
                pb.m_end_major = majdir * endr;
                pb.m_end_minor = endRad * normalize(cross(vbeam, beam_next));
            }
            prev_major = pb.m_end_major;
            prev_minor = pb.m_end_minor;


            //debugging
            if (abs(length(pb.m_start_minor) - startRad) > 0.001) {
                std::cout << pb.m_start_minor.toString() << "\t" << length(pb.m_start_minor) << " \t" << startRad << std::endl;
            }
            if (length(pb.m_start_major) < length(pb.m_start_minor)) {
                std::cout << pb.m_start_major.toString() << " \t" << pb.m_start_minor.toString() << std::endl;
            }
            if (abs(dot(pb.m_start_major, pb.m_start_minor)) > 0.001) {
                std::cout << pb.m_start_major.toString() << " \t" << pb.m_start_minor.toString() << std::endl;
            }
            pb.m_power = Color3(0.7, 0.5, 1.0); // TODO unhardcode
            beams.append(pb);



        }
    }
    return beams;
}

const CFrame &World::getCameraCframe()
{
    const CFrame& cframe = m_camera->frame();
    return cframe;
}

void World::setCameraCframe(CFrame &cframe)
{
    m_camera->setFrame(cframe);
}

