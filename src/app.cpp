
#include "app.h"

#ifndef G3D_PATH
#define G3D_PATH "/contrib/projects/g3d10/G3D10"
#endif

//RenderMethod App::m_currRenderMethod = PATH;
String App::m_scenePath = G3D_PATH "/data/scene";
String App::m_defaultScene = FileSystem::currentDirectory() + "/../data-files/scene/sphere_spline.Scene.Any";

// set this to 1 to debug a single render thread
#define THREADS 12

App::App(const GApp::Settings &settings)
    : GApp(settings),
//    pass(0)
//    m_renderer(new PathTracer)
      stage(App::IDLE),
      view(App::DEFAULT),
      continueRender(true),
      m_passType(0)
{
    m_scenePath = FileSystem::currentDirectory() + "/scene";

    num_passes=5000;

    m_PSettings.useDirectDiffuse=true;
    m_PSettings.useDirectSpecular=true;
    m_PSettings.useEmitted=true;
    m_PSettings.useIndirect=true;

    m_PSettings.dofEnabled=false;
    m_PSettings.dofFocus=-4.8f;
    m_PSettings.dofLens=0.2f;
    m_PSettings.dofSamples=5;

    m_PSettings.attenuation=0.0;
    m_PSettings.scattering=0.0;
    m_PSettings.noiseBiasRatio=0.0;
    m_PSettings.radiusScalingFactor=0.5;

    m_PSettings.maxDepth=4;
    m_PSettings.epsilon=0.0001;
    m_PSettings.numBeamettes=5000;

    m_PSettings.directSamples=64;
    m_PSettings.gatherRadius=0.1;
    m_PSettings.useFinalGather=false;
}

App::~App() { }

void App::setScenePath(const char* path)
{
    m_scenePath = path;
}

/** Bumps a position */
static Vector3 bump(Vector3 pos, Vector3 dir, Vector3 normal)
{
    return pos + sign(dir.dot(normal)) * EPSILON * normal;
}

/** Bumps a ray in place */
static void bump(Ray &ray, shared_ptr<Surfel> surf)
{
    ray.set(bump(ray.origin(), ray.direction(), surf->shadingNormal),
            ray.direction());
}

/** Computes ray.origin + t * ray.direction, bumps the point according to the
  * normal vector, and returns it.
  */
static Vector3 bump(Ray &ray, float t, Vector3 normal)
{
    return bump(ray.origin() + t * ray.direction(), ray.direction(), normal);
}

/** Used for attenuation
 */
static Radiance3 exp( float d, const Radiance3 &tau )
{
    return Radiance3( ::exp(-d*tau.r),
                      ::exp(-d*tau.g),
                      ::exp(-d*tau.b) );
}

/**
 * Emits a photon into the scene, and bounces it in the scene, storing increment bounces in the photon map
 */
void App::scatter()
{

    // Emit a photon
    Photon photon = Photon();
    shared_ptr<Surfel> surfel;

    m_world.emit(m_random, photon, surfel);


    // Bounce the photon in the scene
    Array<Photon> incidentPhotons;
    photonTrace(photon, incidentPhotons);


    // Insert the bounced photon into the map
    m_photons.insert(incidentPhotons);
}

/**
 * Helper function for creating the photon map, clears the incidentPhotons array
 * And starts a photon trace into the scene, with initial bounces = 0
 */
void App::photonTrace(const Photon& emitPhoton, Array<Photon>& incidentPhotons){
    incidentPhotons.fastClear();
    photonTraceHelper(emitPhoton, incidentPhotons, 0);
}

/**
 * The recursive photon tracing helper, which appends intermediate photons
 * to the incidentPhotons array, and bounces the photon to continue in the scene
 * based on russian roulette termination
 */
void App::photonTraceHelper(const Photon& emitPhoton, Array<Photon>& incidentPhotons, int bounces){
    // Terminate recursion
    if (bounces > MAX_DEPTH){
        return;
    }

    shared_ptr<Surfel> surfel;
    float dist = inf();
    Ray ray = Ray(emitPhoton.position, emitPhoton.wi);
    m_world.intersect(ray, dist, surfel);

    // If intersection
    if (surfel){
        // Don't store direct light contribution
        if (bounces > 0){
            Photon photon = Photon();
            photon.position = surfel->position + EPSILON * surfel->shadingNormal; //bump(surfel->position, EPSILON, surfel->shadingNormal);//bump(surfel->position, -ray.direction(), surfel->shadingNormal);//surfel->position;//bump(surfel->position, -ray.direction(), surfel->shadingNormal); // TOOD: maybe bump position beore store in photon map, use helper method for sign
            photon.wi = -ray.direction();
            photon.power = emitPhoton.power;
            incidentPhotons.append(photon);
        }
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
            Vector3 surfelPosOffset = bump(surfel->position, wOut, surfel->shadingNormal);
            Ray offsetRay = Ray(surfelPosOffset, wOut);
            Photon photon2 = Photon();
            photon2.position = surfel->position;
            photon2.wi = offsetRay.direction();
            photon2.power = emitPhoton.power * weight;
            photon2.power.r *= (probability.r/prob);
            photon2.power.g *= (probability.g/prob);
            photon2.power.b *= (probability.b/prob);
            bounces += 1;
            photonTraceHelper(photon2, incidentPhotons, bounces);
        }
    }
}

Radiance3 App::direct(shared_ptr<Surfel> surf, Vector3 wo)
{
    Radiance3 rad;

    shared_ptr<Surfel> light;
    float P_light;
    float area;

    for (int i = 0; i < DIRECT_SAMPLES; ++i)
    {
        m_world.emissivePoint(m_random, light, P_light, area);

        Vector3 wi = light->position - surf->position;
        float dist = wi.length();
        if (dist < EPSILON)
            continue;
        wi /= dist;

        if (m_world.lineOfSight(bump(surf->position, wi, surf->geometricNormal), light->position))
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

Radiance3 App::impulse(shared_ptr<Surfel> surf, Vector3 wo, int depth)
{
    if (!--depth)
        return Radiance3::zero();

    Surfel::ImpulseArray imp;
    surf->getImpulses(PathDirection::EYE_TO_SOURCE, wo, imp);

    Radiance3 rad;
    for (int i = 0; i < imp.size(); ++i)
    {
        Ray ray(surf->position, imp[i].direction);
        bump(ray, surf);

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

Radiance3 App::diffuse(shared_ptr<Surfel> surf, Vector3 wo, int depth)
{
    // In line with the path demo, ignore diffuse interrfelection for specular
    // surfaces.
    Surfel::ImpulseArray imp;
    surf->getImpulses(PathDirection::EYE_TO_SOURCE, wo, imp);
    if (imp.size() > 0)
        return Radiance3::zero();

    Radiance3 rad;
    // If first bounce, final gather
    if (depth == MAX_DEPTH && m_PSettings.useFinalGather){
        for (int i=0; i < GATHER_SAMPLES; i++){
            // get a random sample direction from this sample point
            Vector3 wInGather = wo;
            Vector3 wOutGather = Vector3(0.f, 0.f, 0.f);
            float probabilityHint = 0.f;
            Color3 weight = Color3(1.0);
            surf->scatter(PathDirection::SOURCE_TO_EYE, wInGather, false, m_random, weight, wOutGather, probabilityHint);
            Vector3 offsetPos = bump(surf->position, wOutGather, surf->shadingNormal);//bump(surf->position, wOutGather, surf->shadingNormal);
            Ray gatherRay = Ray(offsetPos, wOutGather);
            int newDepth = depth - 1;
            Radiance3 gatherColor = trace(gatherRay, newDepth).clamp(0.f, 1.f);
            Radiance3 currColor = pif() * gatherColor * weight;
            rad += currColor;
        }
        rad /= GATHER_SAMPLES;

    // Else, do normal diffuse calcualation
    }else{
        // Iterate through photons in a sphere of radius GATHER_RADIUS
        // Using cone() as kernel
        PhotonMap::SphereIterator photon = m_photons.begin(Sphere(surf->position, GATHER_RADIUS));

        for (; photon.isValid(); ++photon){
            float dist = Vector3(surf->position - photon->position).length();
            Radiance3 scatter = surf->finiteScatteringDensity(photon->wi, wo.direction());
            rad += photon->power * cone(dist) * scatter;
        }
    }

    return rad;
}

Radiance3 App::trace(const Ray &ray, int depth)
{
    Radiance3 final;

    float dist = 0;
    shared_ptr<Surfel> surf;
    m_world.intersect(ray, dist, surf);

    if (surf)
    {
        Vector3 wo = -ray.direction();

        Radiance3 surf_radiance = surf->emittedRadiance(wo)
               + direct(surf, wo)
               + diffuse(surf, wo, depth)
               + impulse(surf, wo, depth);
        surf_radiance *= exp(dist, Radiance3(m_PSettings.attenuation));
        final += surf_radiance;
    }

    return final;
}

void App::buildPhotonMap()
{
    // Make the diret photon beams, to be splatted and rendered directly.
    m_dirBeams = std::make_unique<DirPhotonScatter>(&m_world, m_PSettings);

    // Make the indirect photon beams, to be used to evaluate the lighting equation in the scene.
    // Note that it's redunant to here calculate both of these lighting maps, but
    // we'll later be using them at different rates (and also with different scattering properties)
    m_inDirBeams = std::make_unique<IndPhotonScatter>(&m_world, m_PSettings);


    for (int i = 0; i < NUM_PHOTONS; ++i)
    {
        printf("\rBuilding photon map ... %.2f%%", 100.f * i / NUM_PHOTONS);
        scatter();
    }
    printf("\rBuilding photon map ... done       \n");

    // Create renderer
    m_indRenderer = std::make_unique<IndRenderer>(&m_world, m_PSettings);
    m_indRenderer->setBeams(m_inDirBeams->getBeams());
}

void App::traceCallback(int x, int y)
{

    Ray ray = m_world.camera()->worldRay(x + .5f, y + .5f, m_canvas->rect2DBounds());
//    m_canvas->set(x, y, trace(ray, MAX_DEPTH));

    m_canvas->set(x, y, m_indRenderer->trace(ray, MAX_DEPTH));
}

static void dispatcher(void *arg)
{

    App *self = (App*)arg;


    int w = self->window()->width(),
        h = self->window()->height();


    self->stage = App::SCATTERING;

    self->buildPhotonMap();

    printf("Rendering ...");
    fflush(stdout);
    self->stage = App::GATHERING;
    Thread::runConcurrently(Point2int32(0, 0),
                            Point2int32(w, h),
                            [self](Point2int32 pixel){self->traceCallback(pixel.x, pixel.y);});
    printf("done\n");

    self->stage = App::IDLE;
}

void App::onInit()
{

    // GPU stuff
    m_dirLight = Texture::createEmpty("App::dirLight", m_framebuffer->width(),
                                      m_framebuffer->height(), ImageFormat::RGBA16());
    m_dirLight->clear();

    m_dirFBO = Framebuffer::create(m_dirLight);

    m_dirFBO->set(Framebuffer::AttachmentPoint::COLOR1, m_dirLight);


    setFrameDuration(1.0f / 60.0f);


    GApp::showRenderingStats = false;
    renderDevice->setSwapBuffersAutomatically(false); // TODO this should be false?

    ArticulatedModel::Specification spec;
    spec.filename = System::findDataFile("model/cauldron.obj");

    spec.stripMaterials = true;

    m_model = ArticulatedModel::create(spec);
    m_model->pose(m_sceneGeometry, Point3(0.f, 0.f, 0.f));


    // Set up GUI
    createDeveloperHUD();
    developerWindow->setVisible(false);
    developerWindow->cameraControlWindow->setVisible(false);
//    m_scenePath = m_defaultScene;

    makeGUI();

    m_canvas = Image3::createEmpty(window()->width(),
                                   window()->height());
}

void App::onRender()
{

    if(m_dispatch == NULL || (m_dispatch != NULL && m_dispatch->completed()))
    {
        continueRender = true;

        m_photons.clear();

        String fullpath = m_scenePath + "/" + m_ddl->selectedValue().text();
        m_world.unload();
        m_world.load(fullpath);
        std::cout << "Loading scene path " + fullpath << std::endl;
        m_canvas = Image3::createEmpty(window()->width(),
                                       window()->height());
        m_dispatch = Thread::create("dispatcher", dispatcher, this);
        m_dispatch->start();
    } else {
        continueRender=false;
    }
}

void App::onCleanup()
{
    m_world.unload();
}

/** Makes the verts to visualize the indirect lighting */
void App::makeLinesDirBeams(SlowMesh &mesh)
{
    Array<PhotonBeamette> beams = m_dirBeams->getBeams();
    for (int i=0; i<beams.size(); i++) {
        PhotonBeamette beam = beams[i];
        mesh.setColor(beam.m_power / beam.m_power.max());
        mesh.makeVertex(beam.m_start);
        mesh.makeVertex(beam.m_end);
    }
}

/** Makes the verts to visualize the direct lighting */
void App::makeLinesIndirBeams(SlowMesh &mesh)
{
    std::shared_ptr<G3D::KDTree<PhotonBeamette>> t = m_inDirBeams->getBeams();
    G3D::KDTree<PhotonBeamette>::Iterator end = t->end();
    G3D::KDTree<PhotonBeamette>::Iterator cur = t->begin();

    while (cur != end)
    {
        mesh.setColor(cur->m_power / cur->m_power.max());
        mesh.makeVertex(cur->m_start);
        mesh.makeVertex(cur->m_end);
        ++cur;
    }
}

void App::renderBeams(RenderDevice *dev, World *world)
{
    world->renderWireframe(dev);

    dev->pushState();
    world->setMatrices(dev);
    SlowMesh mesh(PrimitiveType::LINES);
    mesh.setPointSize(1);

    // TODO: Potentially add an option to the GUI to toggle between direct and indirect visualization?
    // i.e., toggle between makeLinesIndirBeams() and makeLinesDirBeams().
    // (Might not matter once we have fully splatted beams, which just WILL be the direct visualization)
    makeLinesIndirBeams(mesh);
    mesh.render(dev);
    dev->popState();
}

void App::onGraphics3D(RenderDevice *rd, Array<shared_ptr<Surface> > &surface3D)
{
    gpuProcess(rd);
    if (m_dirBeams && m_inDirBeams && view == App::PHOTONMAP)
    {
        renderBeams(rd, &m_world);
    }
}

void App::gpuProcess(RenderDevice *rd)
{
    Array<PhotonBeamette> direct_beams = m_world.vizualizeSplines();

    rd->pushState(m_dirFBO); {

        rd->setProjectionAndCameraMatrix(m_debugCamera->projection(), m_debugCamera->frame());

        rd->setColorClearValue(Color3::black());
        rd->clear();
        rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE);

        Args args;
        // TODO : set args uniforms

        CFrame cframe;

        for (int i = 0; i < m_sceneGeometry.size(); i++) {
            const shared_ptr<UniversalSurface>& surface =
                    dynamic_pointer_cast<UniversalSurface>(m_sceneGeometry[i]);

            if (notNull(surface) && view == App::SPLAT) {
                surface->getCoordinateFrame(cframe);
                rd->setObjectToWorldMatrix(cframe);
                args.setUniform("MVP", rd->projectionMatrix() *
                                        rd->cameraToWorldMatrix().inverse() * cframe);
                surface->gpuGeom()->setShaderArgs(args);

                LAUNCH_SHADER("splat.*", args);

            }

        }

    } rd->popState();


    // beam splatting
    rd->pushState(m_dirFBO); {
        // Allocate on CPU
        Array<Vector3>   cpuVertex;
        Array<Vector3>   cpuDiff1;
        Array<Vector3>   cpuDiff2;
        Array<int>   cpuIndex;
        int i = 0;
        cpuIndex.append(i);

        for (PhotonBeamette pb : direct_beams) {
            cpuVertex.append(pb.m_start);
            cpuVertex.append(pb.m_end);
            cpuDiff1.append(pb.m_diff1);
            cpuDiff1.append(pb.m_diff1);
            cpuDiff2.append(pb.m_diff2);
            cpuDiff2.append(pb.m_diff2);
            cpuIndex.append(i);
//            std::cout << pb << std::endl;
        }

        cpuIndex.append(i);
        rd->setObjectToWorldMatrix(CFrame());

        //TODO hardcoded temp, figure out how to get camera from world
        CFrame cameraframe = CFrame::fromXYZYPRDegrees(0, 1.5, 9, 0, 0, 0 );
        Projection cameraproj = Projection();
        cameraproj.setFarPlaneZ(-200);
        cameraproj.setFieldOfViewAngleDegrees(25);
        cameraproj.setFieldOfViewDirection(FOVDirection::VERTICAL);
        cameraproj.setNearPlaneZ(-0.1);
        cameraproj.setPixelOffset(Vector2(0,0));

        rd->setProjectionAndCameraMatrix(cameraproj, cameraframe);


        // Upload to GPU
        shared_ptr<VertexBuffer> vbuffer = VertexBuffer::create(
                    sizeof(Vector3) * cpuVertex.size() +
                    sizeof(Vector3) * cpuDiff1.size() +
                    sizeof(Vector3) * cpuDiff2.size() +
                    sizeof(int) * cpuIndex.size());
        AttributeArray gpuVertex   = AttributeArray(cpuVertex, vbuffer);
        AttributeArray gpuDiff1   = AttributeArray(cpuDiff1, vbuffer);
        AttributeArray gpuDiff2   = AttributeArray(cpuDiff2, vbuffer);
        IndexStream gpuIndex       = IndexStream(cpuIndex, vbuffer);
        Args args;

        args.setPrimitiveType(PrimitiveType::LINES);
        args.setAttributeArray("Position", gpuVertex);
        args.setAttributeArray("Diff1", gpuDiff1);
        args.setAttributeArray("Diff2", gpuDiff2);
//        args.setIndexStream(gpuIndex);
//        rd->setObjectToWorldMatrix(CoordinateFrame());
        //TODO pass in spline information

        args.setUniform("MVP", rd->invertYMatrix() *
                                rd->projectionMatrix() *
                                rd->cameraToWorldMatrix().inverse());

        LAUNCH_SHADER("beamsplat.*", args);

    } rd->popState();



    shared_ptr<Texture> indirectTex = Texture::fromImage("Source", m_canvas);

    // composite direct and indirect
    rd->push2D(m_framebuffer); {
        rd->setColorClearValue(Color3::white() * 0.3f);
        rd->clear();
        rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE);

        Args argsComp;
        argsComp.setRect(rd->viewport());

        argsComp.setUniform("screenHeight", rd->height());
        argsComp.setUniform("screenWidth", rd->width());

        argsComp.setUniform("directSample", m_dirFBO->texture(1), Sampler::buffer());
        argsComp.setUniform("indirectSample", indirectTex, Sampler::buffer());

        LAUNCH_SHADER("composite.*", argsComp);

    } rd->popState();


    // TO RENDER COMPOSITITED
    swapBuffers();
    rd->clear();

    FilmSettings filmSettings = activeCamera()->filmSettings();
    filmSettings.setBloomStrength(0.0);
    filmSettings.setGamma(1.0); // default is 2.0

    m_film->exposeAndRender(rd, filmSettings, m_framebuffer->texture(0),
                            settings().hdrFramebuffer.colorGuardBandThickness.x +
                            settings().hdrFramebuffer.depthGuardBandThickness.x,
                            settings().hdrFramebuffer.depthGuardBandThickness.x);
}



FilmSettings App::getFilmSettings()
{
    FilmSettings s;
    s.setAntialiasingEnabled(false);
    s.setBloomStrength(0);
    s.setGamma(2.060);
    s.setVignetteTopStrength(0);
    s.setVignetteBottomStrength(0);
    return s;
}

void App::changeDataDirectory()
{
    Array<String> sceneFiles;
    FileSystem::getFiles(m_dirName + "*.Any", sceneFiles);

    // if no files found return
    if (!sceneFiles.size())
    {
        m_warningLabel->setCaption("Sorry, no scene files found");
        return;
    }

    loadSceneDirectory(m_dirName);
}

void App::loadSceneDirectory(String directory)
{
    setScenePath(directory.c_str());

    updateScenePathLabel();
    m_warningLabel->setCaption("");
    m_ddl->clear();

    Array<String> sceneFiles;
    FileSystem::getFiles(directory + "/*.Any", sceneFiles);
    Array<String>::iterator scene;
    for (scene = sceneFiles.begin(); scene != sceneFiles.end(); ++scene)
    {
        m_ddl->append(*scene);
    }
}

void App::loadDefaultScene()
{
    App::loadSceneDirectory(dataDir + "/scene");
}

void App::loadCustomScene()
{
    String localDataDir = FileSystem::currentDirectory() + "/scene";
    App::loadSceneDirectory(localDataDir);
}

void App::loadCS244Scene()
{
    App::loadSceneDirectory("/contrib/projects/g3d/cs224/scenes");
}

void App::updateScenePathLabel()
{
    m_scenePathLabel->setCaption("..." + m_scenePath.substr(m_scenePath.length() - 30, m_scenePath.length()));
}

void App::saveCanvas()
{
    time_t rawtime;
    struct tm *info;
    char dayHourMinSec [7];
    time(&rawtime);
    info = localtime(&rawtime);
    strftime(dayHourMinSec, 7, "%d%H%M%S",info);

    shared_ptr<Texture> colorBuffer = Texture::createEmpty("Color", renderDevice->width(), renderDevice->height());
    m_film->exposeAndRender(renderDevice, getFilmSettings(), Texture::fromImage("Source", m_canvas), 0, 0, colorBuffer);
    colorBuffer->toImage(ImageFormat::RGB8())->save(String("../images/scene-") +
                                                    "p" + String(std::to_string(pass).c_str()) +
                                                    "-" + dayHourMinSec + ".png");
}


void App::makeGUI()
{
    std::cout << "making GUI" << std::endl;

    shared_ptr<GuiWindow> windowMain = GuiWindow::create("Main",
                                                     debugWindow->theme(),
                                                     Rect2D::xywh(0,0,60,60),
                                                     GuiTheme::MENU_WINDOW_STYLE);
    GuiPane* paneMain = windowMain->pane();

    // INFO
    GuiPane* infoPane = paneMain->addPane("Info", GuiTheme::ORNATE_PANE_STYLE);
    infoPane->addButton("Save Image", this, &App::saveCanvas);
    infoPane->addButton("Exit", [this]() { m_endProgram = true; });
    infoPane->pack();

    // SCENE
    GuiPane* scenesPane = paneMain->addPane("Scenes", GuiTheme::ORNATE_PANE_STYLE);

    // SCENE
    m_ddl = scenesPane->addDropDownList("Scenes");

    scenesPane->addLabel("Scene Directory: ");
    m_scenePathLabel = scenesPane->addLabel("");
    scenesPane->addTextBox("Directory:", &m_dirName);
    scenesPane->addButton("Change Directory", this, &App::changeDataDirectory);

    scenesPane->addLabel("Scene Folders");
    scenesPane->addButton("Demo Scenes", this,  &App::loadCustomScene);

    scenesPane->addLabel("View");
    scenesPane->addRadioButton("Default", App::DEFAULT, &view);
    scenesPane->addRadioButton("Photon Beams", App::PHOTONMAP, &view);
    scenesPane->addRadioButton("Splatting (temp)", App::SPLAT, &view);

    m_warningLabel = scenesPane->addLabel("");
    updateScenePathLabel();

    GuiButton* renderButton = scenesPane->addButton("Render", this, &App::onRender);
    renderButton->setFocused(true);
    scenesPane->pack();

    // RENDERING
    GuiPane* settingsPane = paneMain->addPane("Settings", GuiTheme::ORNATE_PANE_STYLE);
//    settingsPane->addNumberBox(GuiText("Passes"), &num_passes, GuiText(""), GuiTheme::NO_SLIDER, 1, 10000, 0);
    settingsPane->addLabel("Noise:bias ratio");
    settingsPane->addNumberBox(GuiText(""), &m_PSettings.noiseBiasRatio, GuiText(""), GuiTheme::LINEAR_SLIDER, 0.0f, 1.0f, 0.0f);
    settingsPane->addLabel("Radius scaling factor");
    settingsPane->addNumberBox(GuiText(""), &m_PSettings.radiusScalingFactor, GuiText(""), GuiTheme::LINEAR_SLIDER, 0.0f, 1.0f, 0.05f);
    settingsPane->addLabel("Scattering");
    settingsPane->addNumberBox(GuiText(""), &m_PSettings.scattering, GuiText(""), GuiTheme::LINEAR_SLIDER, 0.0f, 1.0f, 0.05f);
    settingsPane->addLabel("Attenuation");
    settingsPane->addNumberBox(GuiText(""), &m_PSettings.attenuation, GuiText(""), GuiTheme::LINEAR_SLIDER, 0.0f, 1.0f, 0.05f);

    // Lights
    GuiPane* lightsPane = paneMain->addPane("Lights", GuiTheme::ORNATE_PANE_STYLE);
    m_lightdl = lightsPane->addDropDownList("Emitter");
    lightsPane->addCheckBox("Enable", &m_PSettings.lightEnabled);
//    lightsPane->addLabel("Beam radius");
//    lightsPane->addNumberBox(GuiText(""), &m_ptsettings.dofLens, GuiText(""), GuiTheme::LINEAR_SLIDER, 0.0f, 100.0f, 1.0f);
//    lightsPane->addLabel("Scattering");
//    lightsPane->addLabel("Attenuation");
    lightsPane->pack();

    windowMain->pack();
    windowMain->setVisible(true);

    addWidget(windowMain);

    loadSceneDirectory(m_scenePath);

    std::cout <<"done making GUI" << std::endl;
}
