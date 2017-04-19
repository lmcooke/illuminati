#include "app.h"

#ifndef G3D_PATH
#define G3D_PATH "/contrib/projects/g3d10/G3D10"
#endif

//RenderMethod App::m_currRenderMethod = PATH;
String App::m_scenePath = G3D_PATH "/data/scene";

static const char *g_scenePath;

G3D_START_AT_MAIN();

int main(int argc, const char *argv[])
{
    GApp::Settings s;
    s.window.caption = "Photon Beams";
    s.dataDir = G3D_PATH "/../data10/common";

    if (argc > 1)
    {
        g_scenePath = argv[1];
    }
    else
    {
        g_scenePath = "/contrib/projects/g3d10/data10/common/scene/CornellBox-spheres.Scene.Any";
//        g_scenePath = "/contrib/projects/g3d10/data10/common/scene/CornellBox.Scene.Any";
        printf("No scene specified, using default: %s\n", g_scenePath);
    }

    return App(s).run();
}

App::App(const GApp::Settings &settings)
    : GApp(settings),
      stage(App::IDLE),
      continueRender(true),
      view(App::DEFAULT),
      m_useGather(false)
{
    m_scenePath = dataDir + "/scene";

    m_ptsettings.useDirectDiffuse=true;
    m_ptsettings.useDirectSpecular=true;
    m_ptsettings.useEmitted=true;
    m_ptsettings.useIndirect=true;

    m_ptsettings.dofEnabled=false;
    m_ptsettings.dofFocus=-4.8f;
    m_ptsettings.dofLens=0.2f;
    m_ptsettings.dofSamples=5;

    m_ptsettings.attenuation=true;
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
    if (depth == MAX_DEPTH && m_useGather){
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

        for (photon; photon.isValid() ; ++photon){
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
        Point3 loc = ray.origin() + ray.direction() * dist;
        Point3 eye = ray.origin();
        Vector3 wo = -ray.direction();

        final += surf->emittedRadiance(wo)
               + direct(surf, wo)
               + diffuse(surf, wo, depth)
               + impulse(surf, wo, depth);
    }

    return final;
}

void App::buildPhotonMap()
{
    m_dirBeams = std::make_unique<DirPhotonScatter>(&m_world);

    for (int i = 0; i < NUM_PHOTONS; ++i)
    {
//        printf("\rBuilding photon map ... %.2f%%", 100.f * i / NUM_PHOTONS);
        scatter();
    }
    printf("\rBuilding photon map ... done       \n");
}

void App::traceCallback(int x, int y)
{
    Ray ray = m_world.camera()->worldRay(x + .5f, y + .5f, m_canvas->rect2DBounds());
    m_canvas->set(x, y, trace(ray, MAX_DEPTH));
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
    GApp::showRenderingStats = false;
    renderDevice->setSwapBuffersAutomatically(true);
    m_world.load(g_scenePath);
    int w = window()->width(),
        h = window()->height();

    // Set up GUI
    createDeveloperHUD();
    developerWindow->setVisible(false);
    developerWindow->cameraControlWindow->setVisible(false);
//    makeGUI();

    m_canvas = Image3::createEmpty(w, h);

    m_dispatch = Thread::create("dispatcher", dispatcher, this);
    m_dispatch->start();
}


void App::onRender()
{
    // user clicks the render button
    if(m_dispatch == NULL || (m_dispatch != NULL && m_dispatch->completed()))
    {
        continueRender = true;
        String fullpath = m_scenePath + "/" + m_ddl->selectedValue().text();

        m_world.unload();
        m_world.load(fullpath);

//        m_renderer->setWorld(&m_world);
//        m_renderer->setPTSettings(m_ptsettings);

//        shared_ptr<Camera> cam = m_world.camera();
//        cam->depthOfFieldSettings().setEnabled(true);
//        cam->depthOfFieldSettings().setModel(DepthOfFieldModel::PHYSICAL);

//        cam->depthOfFieldSettings().setLensRadius(m_ptsettings.dofLens);
//        cam->depthOfFieldSettings().setFocusPlaneZ(m_ptsettings.dofFocus);

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

void App::renderBeams(RenderDevice *dev, World *world)
{
    world->renderWireframe(dev);

    dev->pushState();
    world->setMatrices(dev);
    SlowMesh mesh(PrimitiveType::LINES);
    mesh.setPointSize(1);

    std::vector<PhotonBeamette> beams = m_dirBeams->getBeams();
    for (int i=0; i<beams.size(); i++) {
        PhotonBeamette beam = beams[i];
        mesh.setColor(beam.m_power / beam.m_power.max());
        mesh.makeVertex(beam.m_start);
        mesh.makeVertex(beam.m_end);
    }
    mesh.render(dev);
    dev->popState();
}

void App::onGraphics(RenderDevice *dev,
                     Array<shared_ptr<Surface> >& posed3D,
                     Array<shared_ptr<Surface2D> >& posed2D)
{
    View v = this->view;
    if (v == DEFAULT)
        v = stage == SCATTERING ? PHOTONMAP : RENDITION;

    if (v == PHOTONMAP)
    {
        dev->setColorClearValue(Color4(0.0, 0.0, 0.0, 0.0));
        dev->clear();
        renderBeams(dev, &m_world);
//        m_photons.render(dev, &m_world);
    }
    // If you want to display other things (e.g. shadow photon maps), do it here
    else
    {
        shared_ptr<Texture> tex = Texture::fromImage("Source", m_canvas);

        FilmSettings s;
        s.setAntialiasingEnabled(false);
        s.setBloomStrength(0);
        s.setGamma(2.060);
        s.setVignetteTopStrength(0);
        s.setVignetteBottomStrength(0);
        m_film->exposeAndRender(renderDevice, s, tex, 0, 0);

        Surface2D::sortAndRender(dev, posed2D);

    }

//    shared_ptr<Texture> tex = Texture::fromImage("Source", m_canvas);

//    m_film->exposeAndRender(renderDevice, getFilmSettings(), tex, 0, 0);

//    Surface2D::sortAndRender(dev, posed2D);
}

void App::onUserInput(UserInput* input)
{
    if (input->keyReleased(GKey('1')))
        this->view = PHOTONMAP;

    if (input->keyReleased(GKey('2')))
        this->view = RENDITION;

    if (input->keyReleased(GKey('0')))
        this->view = DEFAULT;
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

void App::saveCanvas()
{
    std::cout << "saving canvas" << std::endl;
    time_t rawtime;
    struct tm *info;
    char dayHourMinSec [7];
    time(&rawtime);
    info = localtime(&rawtime);
    strftime(dayHourMinSec, 7, "%d%H%M%S",info);

    shared_ptr<Texture> colorBuffer = Texture::createEmpty("Color", renderDevice->width(), renderDevice->height());
    m_film->exposeAndRender(renderDevice, getFilmSettings(), Texture::fromImage("Source", m_canvas), 0, 0, colorBuffer);
    colorBuffer->toImage(ImageFormat::RGB8())->save (String("../images/scene-") +
                                                    "p" + String(std::to_string(pass).c_str()) +
                                                    "-" + dayHourMinSec + ".png");
}

void App::toggleWindowRendering()
{
    std::cout << "toggling rendering window" << std::endl;
    m_windowRendering->setVisible(!m_windowRendering->visible());
}

void App::toggleWindowScenes()
{
    m_windowScenes->setVisible(!m_windowScenes->visible());
}

void App::toggleWindowPath()
{
    m_windowPath->setVisible(!m_windowPath->visible());
}

void App::makeGUI()
{
    std::cout << "making GUI" << std::endl;

    shared_ptr<GuiWindow> windowMain = GuiWindow::create("Main",
                                                     debugWindow->theme(),
                                                     Rect2D::xywh(0,0,60,60),
                                                     GuiTheme::MENU_WINDOW_STYLE);
    GuiPane* paneMain = windowMain->pane();

//    // INFO
//    GuiPane* infoPane = paneMain->addPane("Info", GuiTheme::ORNATE_PANE_STYLE);
//    infoPane->addButton("Save Image", this, &App::saveCanvas);
//    infoPane->addButton("Exit", [this]() { m_endProgram = true; });
//    infoPane->pack();

//    // SCENE
//    GuiPane* scenesPane = paneMain->addPane("Scenes", GuiTheme::ORNATE_PANE_STYLE);

//    m_ddl = scenesPane->addDropDownList("Scenes");
//    scenesPane->addLabel("Scene Directory: ");
//    m_scenePathLabel = paneMain->addLabel("");
////    scenesPane->addTextBox("Directory:", &m_dirName);
////    scenesPane->addButton("Change Directory", this, &App::changeDataDirectory);

////    scenesPane->addLabel("Scene Folders");
//    scenesPane->addButton("Demo Scenes", this,  &App::loadDefaultScene);

//    GuiButton* renderButton = scenesPane->addButton("Render", this, &App::onRender);
//    renderButton->setFocused(true);
//    scenesPane->pack();

//    // RENDERING
//    GuiPane* settingsPane = paneMain->addPane("Settings", GuiTheme::ORNATE_PANE_STYLE);
//    settingsPane->addNumberBox(GuiText("Passes"), &num_passes, GuiText(""), GuiTheme::NO_SLIDER, 1, 10000, 0);
////    settingsPane->addCheckBox("Attenuation", &m_ptsettings.attenuation);
//    settingsPane->addLabel("Noise:bias ratio");
//    settingsPane->addNumberBox(GuiText(""), &m_ptsettings.dofLens, GuiText(""), GuiTheme::LINEAR_SLIDER, 0.0f, 1.0f, 0.05f);

//    // Lights
//    GuiPane* lightsPane = paneMain->addPane("Lights", GuiTheme::ORNATE_PANE_STYLE);
//    m_lightdl = lightsPane->addDropDownList("Emitter");
//    lightsPane->addCheckBox("Enable", &m_ptsettings.attenuation);
//    lightsPane->addLabel("Beam radius");
//    lightsPane->addNumberBox(GuiText(""), &m_ptsettings.dofLens, GuiText(""), GuiTheme::LINEAR_SLIDER, 0.0f, 100.0f, 1.0f);
//    lightsPane->addLabel("Scattering");
//    lightsPane->addLabel("Attenuation");
//    lightsPane->pack();

//    loadSceneDirectory(m_scenePath);

    windowMain->pack();
    windowMain->setVisible(true);

    addWidget(windowMain);

    std::cout <<"done making GUI" << std::endl;
}

//void App::makeGUI()
//{
//    std::cout << "making GUI" << std::endl;

//    shared_ptr<GuiWindow> windowMain = GuiWindow::create("Main",
//                                                     debugWindow->theme(),
//                                                     Rect2D::xywh(0,0,50,50),
//                                                     GuiTheme::MENU_WINDOW_STYLE);

//    m_windowRendering = GuiWindow::create("Rendering",
//                                                     debugWindow->theme(),
//                                                     Rect2D::xywh(0,0,50,50),
//                                                     GuiTheme::NORMAL_WINDOW_STYLE);

//    m_windowScenes = GuiWindow::create("Scenes",
//                                                 debugWindow->theme(),
//                                                 Rect2D::xywh(50,0,50,50),
//                                                 GuiTheme::NORMAL_WINDOW_STYLE);

//    m_windowPath = GuiWindow::create("Path Tracer",
//                                                 debugWindow->theme(),
//                                                 Rect2D::xywh(50,0,50,50),
//                                                 GuiTheme::NORMAL_WINDOW_STYLE);


//    GuiPane* paneMain = windowMain->pane();
//    GuiPane* paneScenes = m_windowScenes->pane();
//    GuiPane* paneRendering = m_windowRendering->pane();
//    GuiPane* panePath = m_windowPath->pane();

//    //MAIN
//    paneMain->addButton("Rendering", this, &App::toggleWindowRendering);
//    paneMain->addButton("Scenes", this, &App::toggleWindowScenes);
//    paneMain->addButton("Path Tracer", this, &App::toggleWindowPath);

//    // SCENE
//    m_ddl = paneScenes->addDropDownList("Scenes");

//    paneScenes->addLabel("Scene Directory: ");
//    m_scenePathLabel = paneScenes->addLabel("");
//    paneScenes->addTextBox("Directory:", &m_dirName);
//    paneScenes->addButton("Change Directory", this, &App::changeDataDirectory);

//    paneScenes->addLabel("Scene Folders");
//    paneScenes->addButton("My Scenes", this,  &App::loadCustomScene);
//    paneScenes->addButton("G3D Scenes", this,  &App::loadDefaultScene);
//    paneScenes->addButton("CS224 Scenes", this,  &App::loadCS244Scene);

//    m_warningLabel = paneScenes->addLabel("");
//    updateScenePathLabel();

//    // RENDERING WINDOW
////    GuiControl::Callback changeRender(this, &App::changeRenderMethod);
////    m_renderdl = paneRendering->addDropDownList("Renderer", Array<GuiText>("Ray", "Path", "Photon"), (int*)(&m_currRenderMethod), changeRender);

//    paneRendering->addButton("Save Image", this, &App::saveCanvas);
//    GuiButton* renderButton = paneRendering->addButton("Render", this, &App::onRender);
//    renderButton->setFocused(true);
//    renderButton->moveBy(140.0f,0.0f);

////    paneRendering->addLabel("--- Skybox ---");
////    paneRendering->addCheckBox("Enable", &m_ptsettings.useSkyMap);

////    paneRendering->addLabel("--- Depth of Field ---");
////    paneRendering->addCheckBox("Enable", &m_ptsettings.dofEnabled);

////    paneRendering->addLabel("Lens Radius");
////    paneRendering->addNumberBox(GuiText(""), &m_ptsettings.dofLens, GuiText(""), GuiTheme::LINEAR_SLIDER, 0.0f, 1.0f, 0.05f);
////    paneRendering->addLabel("Focus Plane");
////    paneRendering->addNumberBox(GuiText(""), &m_ptsettings.dofFocus, GuiText(""), GuiTheme::LINEAR_SLIDER, 0.0f, 3.45f, 0.05f);
////    paneRendering->addLabel("DOF Samples");
////    paneRendering->addNumberBox(GuiText(""), &m_ptsettings.dofSamples, GuiText(""), GuiTheme::LINEAR_SLIDER, 1, 20, 1);

////    paneRendering->addLabel("--- Stratified Sampling ---");
////    paneRendering->addLabel("Subpixel Divisions");
////    paneRendering->addNumberBox(GuiText(""), &m_ptsettings.superSamples, GuiText(""), GuiTheme::LINEAR_SLIDER, 1, 5, 1);

//    // PATH
////    panePath->addNumberBox(GuiText("Passes"), &num_passes, GuiText(""), GuiTheme::NO_SLIDER, 1, 10000, 0);
////    panePath->addCheckBox("Attenuation", &m_ptsettings.attenuation);
////    paneRendering->addLabel("--- Radiance Components ---");
////    panePath->addCheckBox("Emitted Light", &m_ptsettings.useEmitted);
////    panePath->addCheckBox("Scattered Direct Light from Diffuse", &m_ptsettings.useDirectDiffuse);
////    panePath->addCheckBox("Scattered Direct Light from Specular", &m_ptsettings.useDirectSpecular);
////    panePath->addCheckBox("Scattered Indirect Light from Diffuse", &m_ptsettings.useIndirect);
////    panePath->addCheckBox("Participating Media Attenuation", &m_ptsettings.useMedium);

//    loadSceneDirectory(m_scenePath);

//    m_windowPath->pack();
//    m_windowPath->setVisible(false);

//    m_windowScenes->pack();
//    m_windowScenes->setVisible(false);

//    m_windowRendering->pack();
//    m_windowRendering->setVisible(false);

//    windowMain->pack();
//    windowMain->setVisible(true);

//    addWidget(m_windowPath);
//    addWidget(m_windowRendering);
//    addWidget(m_windowScenes);
//    addWidget(windowMain);

//    std::cout <<"done making GUI" << std::endl;
//}
