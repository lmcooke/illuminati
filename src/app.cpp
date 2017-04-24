
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

    m_PSettings.maxDepthScatter=3;
    m_PSettings.maxDepthRender=4;
    m_PSettings.epsilon=0.0001;
    m_PSettings.numBeamettes=5000;

    m_PSettings.directSamples=64;
    m_PSettings.gatherRadius=0.1;
    m_PSettings.useFinalGather=false;
    m_PSettings.dist = 0.3;
}

App::~App() { }

void App::setScenePath(const char* path)
{
    m_scenePath = path;
}

void App::buildPhotonMap()
{
    // Make the diret photon beams, to be splatted and rendered directly.
    m_dirBeams = std::make_unique<DirPhotonScatter>(&m_world, m_PSettings);

    // Make the indirect photon beams, to be used to evaluate the lighting equation in the scene.
    // Note that it's redunant to here calculate both of these lighting maps, but
    // we'll later be using them at different rates (and also with different scattering properties)
    m_inDirBeams = std::make_unique<IndPhotonScatter>(&m_world, m_PSettings);

    // Create renderer
    m_indRenderer = std::make_unique<IndRenderer>(&m_world, m_PSettings);
    m_indRenderer->setBeams(m_inDirBeams->getBeams());
}

void App::traceCallback(int x, int y)
{
    Ray ray = m_world.camera()->worldRay(x + .5f, y + .5f, m_canvas->rect2DBounds());
    m_canvas->set(x, y, m_indRenderer->trace(ray, m_PSettings.maxDepthScatter));
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
    m_count = 0.f;

    m_passes = 0;
    m_dirLight = Texture::createEmpty("App::dirLight", m_framebuffer->width(),
                                      m_framebuffer->height(), ImageFormat::RGBA16());

    m_totalDirLight1 = Texture::createEmpty("App::totalDirLight1", m_framebuffer->width(),
                                          m_framebuffer->height(), ImageFormat::RGBA16());

    m_currentComposite1 = Texture::createEmpty("App::currentComp1", m_framebuffer->width(),
                                         m_framebuffer->height(), ImageFormat::RGBA16());

    m_totalDirLight2 = Texture::createEmpty("App::totalDirLight2", m_framebuffer->width(),
                                          m_framebuffer->height(), ImageFormat::RGBA16());

    m_currentComposite2 = Texture::createEmpty("App::currentComp2", m_framebuffer->width(),
                                         m_framebuffer->height(), ImageFormat::RGBA16());

    m_dirLight->clear();
    m_totalDirLight1->clear();
    m_currentComposite1->clear();
    m_totalDirLight2->clear();
    m_currentComposite2->clear();

    m_dirFBO = Framebuffer::create(m_dirLight);
    m_FBO1 = Framebuffer::create(m_currentComposite1);
    m_FBO2 = Framebuffer::create(m_currentComposite2);

    m_dirFBO->set(Framebuffer::AttachmentPoint::COLOR1, m_dirLight);
    m_FBO1->set(Framebuffer::AttachmentPoint::COLOR1, m_currentComposite1);
    m_FBO1->set(Framebuffer::AttachmentPoint::COLOR2, m_totalDirLight1);
    m_FBO2->set(Framebuffer::AttachmentPoint::COLOR1, m_currentComposite2);
    m_FBO2->set(Framebuffer::AttachmentPoint::COLOR2, m_totalDirLight2);


    setFrameDuration(1.0f / 60.0f);


    GApp::showRenderingStats = false;
    renderDevice->setSwapBuffersAutomatically(false);

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
    developerWindow->setResizable(true);
}

void App::onRender()
{

    if(m_dispatch == NULL || (m_dispatch != NULL && m_dispatch->completed()))
    {
        continueRender = true;

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
    if (m_dirBeams)
    {
        Array<PhotonBeamette> beams = m_dirBeams->getBeams();
        for (int i=0; i<beams.size(); i++) {
            // Apparently this is how you index into a pointer to an array?
            PhotonBeamette beam = beams[i];
            mesh.setColor(beam.m_power / beam.m_power.max());
            mesh.makeVertex(beam.m_start);
            mesh.makeVertex(beam.m_end);
        }
    }
}

/** Makes the verts to visualize the direct lighting */
void App::makeLinesIndirBeams(SlowMesh &mesh)
{
    if (m_inDirBeams)
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
}

void App::renderBeams(RenderDevice *dev, World *world)
{
    world->renderWireframe(dev);

    dev->pushState();
    world->setMatrices(dev);
    SlowMesh mesh(PrimitiveType::LINES);
    mesh.setPointSize(1);

    if (view == App::DIRBEAMS){
        makeLinesDirBeams(mesh);
    }else if (view == App::INDBEAMS){
        makeLinesIndirBeams(mesh);
    }
    mesh.render(dev);
    dev->popState();
}

void App::onGraphics3D(RenderDevice *rd, Array<shared_ptr<Surface> > &surface3D)
{

    if (!m_world.camnull()){
        gpuProcess(rd);
    } else {
        swapBuffers();
        rd->clear();
    }

    if (m_dirBeams && m_inDirBeams && (view == App::DIRBEAMS || view == App::INDBEAMS))
    {
        renderBeams(rd, &m_world);
    }
}

void App::gpuProcess(RenderDevice *rd)
{

    Array<PhotonBeamette> direct_beams = m_world.visualizeSplines();

    m_count += .001;
    m_passes += 1;

    // flipFlop FBOs and textures
    bool isEvenPass = m_passes % 2 == 0;
    auto prevFBO = isEvenPass ? m_FBO1 : m_FBO2;
    auto nextFBO = isEvenPass ? m_FBO2 : m_FBO1;

    // turns on and off beam movement so we can visualize GPU averaging
    bool testGPUprogression = false;

    // beam splatting
    rd->pushState(m_dirFBO); {
        // Allocate on CPU
        Array<Vector3>   cpuVertex;
        Array<Vector3>   cpuMajor;
        Array<Vector3>   cpuMinor;
        Array<int>   cpuIndex;
        int i = 0;
        cpuIndex.append(i);

        for (PhotonBeamette pb : direct_beams) {
            if (testGPUprogression) {
                cpuVertex.append(pb.m_start + Vector3(0.0, m_count/10.0, 0.0));
                cpuVertex.append(pb.m_end + Vector3(0.0, m_count/10.0, 0.0));
            } else {
                cpuVertex.append(pb.m_start);
                cpuVertex.append(pb.m_end);
            }
            cpuMajor.append(pb.m_start_major);
            cpuMinor.append(pb.m_start_minor);
            cpuMajor.append(pb.m_end_major);
            cpuMinor.append(pb.m_end_minor);
            cpuIndex.append(i);
        }

        cpuIndex.append(i);
        rd->setObjectToWorldMatrix(CFrame());
        rd->setColorClearValue(Color3::black());
        rd->clear();
        rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE);
        rd->setProjectionAndCameraMatrix(m_world.camera()->projection(), m_world.camera()->frame());

        // Upload to GPU
        shared_ptr<VertexBuffer> vbuffer = VertexBuffer::create(
                    sizeof(Vector3) * cpuVertex.size() +
                    sizeof(Vector3) * cpuMajor.size() +
                    sizeof(Vector3) * cpuMinor.size() +
                    sizeof(int) * cpuIndex.size());
        AttributeArray gpuVertex   = AttributeArray(cpuVertex, vbuffer);
        AttributeArray gpuMajor   = AttributeArray(cpuMajor, vbuffer);
        AttributeArray gpuMinor   = AttributeArray(cpuMinor, vbuffer);
//        IndexStream gpuIndex       = IndexStream(cpuIndex, vbuffer);
        Args args;

        args.setPrimitiveType(PrimitiveType::LINES);
        args.setAttributeArray("Position", gpuVertex);
        args.setAttributeArray("Major", gpuMajor);
        args.setAttributeArray("Minor", gpuMinor);
        args.setUniform("Camera", Vector3(0, 1.5, 9));
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

    rd->push2D(nextFBO); {
        rd->setColorClearValue(Color3::black());
        rd->clear();
        rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE);

        Args argsComp;
        argsComp.setRect(rd->viewport());

        argsComp.setUniform("screenHeight", rd->height());
        argsComp.setUniform("screenWidth", rd->width());
        argsComp.setUniform("passNum", m_passes);

        argsComp.setUniform("prevDirectLight", prevFBO->texture(2), Sampler::buffer());

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
    filmSettings.setVignetteTopStrength(0);
    filmSettings.setVignetteBottomStrength(0);

    m_film->exposeAndRender(rd, filmSettings, nextFBO->texture(1),
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
    scenesPane->addRadioButton("Photon Beams (Dir)", App::DIRBEAMS, &view);
    scenesPane->addRadioButton("Photon Beams (Ind)", App::INDBEAMS, &view);
    scenesPane->addRadioButton("Splatting (temp)", App::SPLAT, &view);

    m_warningLabel = scenesPane->addLabel("");
    updateScenePathLabel();

    GuiButton* renderButton = scenesPane->addButton("Render", this, &App::onRender);
    renderButton->setFocused(true);
    scenesPane->pack();

    // RENDERING
    GuiPane* settingsPane = paneMain->addPane("Settings", GuiTheme::ORNATE_PANE_STYLE);
//    settingsPane->addNumberBox(GuiText("Passes"), &num_passes, GuiText(""), GuiTheme::NO_SLIDER, 1, 10000, 0);
    settingsPane->addLabel("Scattering");
    settingsPane->addNumberBox(GuiText(""), &m_PSettings.scattering, GuiText(""), GuiTheme::LINEAR_SLIDER, 0.0f, 1.0f, 0.05f);
    settingsPane->addLabel("Attenuation");
    settingsPane->addNumberBox(GuiText(""), &m_PSettings.attenuation, GuiText(""), GuiTheme::LINEAR_SLIDER, 0.0f, 1.0f, 0.05f);
    settingsPane->addLabel("Noise:bias ratio");
    settingsPane->addNumberBox(GuiText(""), &m_PSettings.noiseBiasRatio, GuiText(""), GuiTheme::LINEAR_SLIDER, 0.0f, 1.0f, 0.0f);
    settingsPane->addLabel("Radius scaling factor");
    settingsPane->addNumberBox(GuiText(""), &m_PSettings.radiusScalingFactor, GuiText(""), GuiTheme::LINEAR_SLIDER, 0.0f, 1.0f, 0.05f);
    settingsPane->pack();
    // Lights
    GuiPane* lightsPane = paneMain->addPane("Lights", GuiTheme::ORNATE_PANE_STYLE);
    m_lightdl = lightsPane->addDropDownList("Emitter");
    lightsPane->addCheckBox("Enable", &m_PSettings.lightEnabled);
//    lightsPane->addLabel("Beam radius");
//    lightsPane->addNumberBox(GuiText(""), &m_ptsettings.dofLens, GuiText(""), GuiTheme::LINEAR_SLIDER, 0.0f, 100.0f, 1.0f);
//    lightsPane->addLabel("Scattering");
//    lightsPane->addLabel("Attenuation");
    lightsPane->pack();

    paneMain->pack();
    windowMain->pack();
    windowMain->setVisible(true);

    addWidget(windowMain);

    loadSceneDirectory(m_scenePath);

    std::cout <<"done making GUI" << std::endl;
}
