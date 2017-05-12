#include "app.h"

#ifndef G3D_PATH
#define G3D_PATH "/contrib/projects/g3d10/G3D10"
#endif
#define THREADS 8

static Random &rng = Random::common();

String App::m_scenePath = G3D_PATH "/data/scene";
String App::m_defaultScene = FileSystem::currentDirectory() + "/../data-files/scene/sphere_spline.Scene.Any";

App::App(const GApp::Settings &settings)
    : GApp(settings),
      m_PSettings(std::make_shared<PhotonSettings>()),
      stage(App::IDLE),
      view(App::DEFAULT),
      continueRender(true),
      m_passType(0),
      m_radius(1),
      num_passes(5000),
      m_maxPasses(20)
{
    m_scenePath = FileSystem::currentDirectory() + "/scene";

    m_PSettings->attenuation=0.0; // variable in transmission calculation
    m_PSettings->scattering=0.0; // ratio of scattering to extinction

    m_PSettings->noiseBiasRatio=0.0;
    m_PSettings->radiusScalingFactor=0.95;

    m_PSettings->maxDepthScatter=100;
    m_PSettings->maxDepthRender=3;
    m_PSettings->epsilon=0.0001;
    m_PSettings->numBeamettesDir=10;
    m_PSettings->numBeamettesInDir=2000;

    m_PSettings->directSamples=64;

    m_PSettings->gatherRadius=0.5;
    m_PSettings->useFinalGather=false;
    m_PSettings->gatherSamples=50;
    m_PSettings->dist = .5;
    m_PSettings->beamIntensity = 1;
    m_PSettings->beamSpread = 1;
}

App::~App() { }

void App::setScenePath(const char* path)
{
    m_scenePath = path;
}

void App::buildPhotonMap(bool createRngGen)
{
    if (createRngGen) {
        // Make the direct photon beams, to be splatted and rendered directly.
        m_dirBeams = std::make_unique<DirPhotonScatter>(&m_world, m_PSettings);
        m_dirBeams->makeBeams();

        // Make the indirect photon beams, to be used to evaluate the lighting equation in the scene.
        // Note that it's redundant to here calculate both of these lighting maps, but
        // we'll later be using them at different rates (and also with different scattering properties)
        m_inDirBeams = std::make_unique<IndPhotonScatter>(&m_world, m_PSettings);
        m_inDirBeams->makeBeams();

        // Create renderer
        m_indRenderer = std::make_unique<IndRenderer>(&m_world, m_PSettings);
        m_indRenderer->setBeams(m_inDirBeams->getBeams());
    } else {
        m_inDirBeams->makeBeams();
        m_indRenderer->setBeams(m_inDirBeams->getBeams());
    }
}

void App::traceCallback(int x, int y)
{

    if (!continueRender) return;
    if (indRenderCount == -1) {
        m_canvas->set(x, y, Radiance3::black());
    } else {

        // TODO : keep random or just use .5f?
        double dx = rng.uniform(), dy = rng.uniform();

        // Choose a ray, shoot it into the scean
        Ray ray = m_world.camera()->worldRay(x + dx, y + dy, m_canvas->rect2DBounds());
        if (indRenderCount == 0) {
            m_canvas->set(x, y, m_indRenderer->trace(ray, m_PSettings->maxDepthScatter));
        } else {
            Radiance3 prev = m_canvas->get(x,y);
            Radiance3 sample = m_indRenderer->trace(ray, m_PSettings->maxDepthScatter);

            float indCountFl = static_cast<float>(indRenderCount);

            float prevContrib = indCountFl / (indCountFl + 1.f);
            float nextContrib = 1.f / (indCountFl + 1.f);

            m_canvas->set(x, y, prevContrib * prev +
                                nextContrib * sample);
        }
    }
}

static void dispatcher(void *arg)
{
    App *self = (App*)arg;
    self->stage = App::SCATTERING;
    self->buildPhotonMap(true);

    // Create the thread pool and for each pass, send out THREADs number of threads to do the dirty work.
    ThreadPool pool( self, THREADS );
    while (self->indRenderCount < self->m_maxPasses && self->continueRender) {
        printf("Rendering ...");
        std::cout << " Pass: " << self->indRenderCount << std::endl;
        fflush(stdout);
        self->buildPhotonMap(false);
        self->indRenderCount += 1;

        if (self->prevIndRenderCount != -1) {
            self->prevIndRenderCount += 1;
        }

        self->setGatherRadius();
        self->stage = App::GATHERING;
        pool.run();
        printf("done\n");
    }
    self->stage = App::IDLE;
}



void App::onInit()
{

    // GPU stuff

    m_passes = 0;
    m_dirLight = Texture::createEmpty("App::dirLight", m_framebuffer->width(),
                                      m_framebuffer->height(), ImageFormat::RGBA16());

    m_zBuffer = Texture::createEmpty("App::zBuffer", m_framebuffer->width(),
                                      m_framebuffer->height(), ImageFormat::R16());

    m_totalDirLight1 = Texture::createEmpty("App::totalDirLight1", m_framebuffer->width(),
                                          m_framebuffer->height(), ImageFormat::RGBA16());

    m_currentComposite1 = Texture::createEmpty("App::currentComp1", m_framebuffer->width(),
                                         m_framebuffer->height(), ImageFormat::RGBA16());

    m_totalDirLight2 = Texture::createEmpty("App::totalDirLight2", m_framebuffer->width(),
                                          m_framebuffer->height(), ImageFormat::RGBA16());

    m_currentComposite2 = Texture::createEmpty("App::currentComp2", m_framebuffer->width(),
                                         m_framebuffer->height(), ImageFormat::RGBA16());

    m_dirLight->clear();
    m_zBuffer->clear();
    m_totalDirLight1->clear();
    m_currentComposite1->clear();
    m_totalDirLight2->clear();
    m_currentComposite2->clear();

    m_dirFBO = Framebuffer::create(m_dirLight);
    m_ZFBO = Framebuffer::create(m_zBuffer);
    m_FBO1 = Framebuffer::create(m_currentComposite1);
    m_FBO2 = Framebuffer::create(m_currentComposite2);

    m_ZFBO->set(Framebuffer::AttachmentPoint::COLOR1, m_zBuffer);
    m_dirFBO->set(Framebuffer::AttachmentPoint::COLOR1, m_dirLight);
    m_dirFBO->set(Framebuffer::AttachmentPoint::COLOR1, m_dirLight);
    m_FBO1->set(Framebuffer::AttachmentPoint::COLOR1, m_currentComposite1);
    m_FBO1->set(Framebuffer::AttachmentPoint::COLOR2, m_totalDirLight1);
    m_FBO2->set(Framebuffer::AttachmentPoint::COLOR1, m_currentComposite2);
    m_FBO2->set(Framebuffer::AttachmentPoint::COLOR2, m_totalDirLight2);

    setFrameDuration(1.0f / 60.0f);

    indRenderCount = 0;
    prevIndRenderCount = 0;

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

    makeGUI();

    m_canvas = Image3::createEmpty(window()->width(),
                                   window()->height());
    developerWindow->setResizable(true);
}

void App::clearParams() {
    m_updating = true;
    indRenderCount = -1;
    prevIndRenderCount = -1;
    m_passes = 0;
}

bool App::onEvent(const GEvent &e)
{
    if (e.type == GEventType::GUI_CHANGE) {
        clearParams();
        return true;
    }

    if (e.type == GEventType::KEY_DOWN) {

        const CFrame& cFrame = m_world.getCameraCframe();
        float x;
        float y;
        float z;
        float yaw;
        float pitch;
        float roll;

        cFrame.getXYZYPRDegrees(x,y,z,yaw,pitch,roll);

        if (e.key.keysym.sym == 'a') {
            // move cam left
            CFrame newCframe = CFrame::fromXYZYPRDegrees(x - 0.25f, y, z, yaw, pitch, roll);
            m_world.setCameraCframe(newCframe);
            clearParams();

        } else if (e.key.keysym.sym == 'd') {
            // move cam right
            CFrame newCframe = CFrame::fromXYZYPRDegrees(x + 0.25f, y, z, yaw, pitch, roll);
            m_world.setCameraCframe(newCframe);
            clearParams();

        } else if (e.key.keysym.sym == 'z') {
            // move cam up
            CFrame newCframe = CFrame::fromXYZYPRDegrees(x, y + .25f, z, yaw, pitch, roll);
            m_world.setCameraCframe(newCframe);
            clearParams();

        } else if (e.key.keysym.sym == 'c') {
            // move cam down
            CFrame newCframe = CFrame::fromXYZYPRDegrees(x, y - .25f, z, yaw, pitch, roll);
            m_world.setCameraCframe(newCframe);
            clearParams();

        } else if (e.key.keysym.sym == 'w') {
            // move cam forward
            CFrame newCframe = CFrame::fromXYZYPRDegrees(x, y, z - .25f, yaw, pitch, roll);
            m_world.setCameraCframe(newCframe);
            clearParams();

        } else if (e.key.keysym.sym == 's') {
            // move cam backward
            CFrame newCframe = CFrame::fromXYZYPRDegrees(x, y, z + .25f, yaw, pitch, roll);
            m_world.setCameraCframe(newCframe);
            clearParams();

        } else if (e.key.keysym.sym == 'p') {
            // pitch cam
            CFrame newCframe = CFrame::fromXYZYPRDegrees(x,y,z, yaw, pitch + 5.f, roll);
            m_world.setCameraCframe(newCframe);
            clearParams();

        } else if (e.key.keysym.sym == 'e') {
            // rotate
            float a = 2.f;
            Vector4 pt = Matrix4::yawDegrees(a) * Vector4(x, y, z, 1.0);
            CFrame newCframe = CFrame::fromXYZYPRDegrees(pt.x, pt.y, pt.z, yaw + a, pitch, roll);
            m_world.setCameraCframe(newCframe);
            clearParams();

        } else if (e.key.keysym.sym == 'q') {
           // rotate
           float a = -2.f;
           Vector4 pt = Matrix4::yawDegrees(a) * Vector4(x, y, z, 1.0);
           CFrame newCframe = CFrame::fromXYZYPRDegrees(pt.x, pt.y, pt.z, yaw + a, pitch, roll);
           m_world.setCameraCframe(newCframe);
           clearParams();
        }
        return true;
    }

    return false;
}

void App::onRender()
{
    if(m_dispatch == NULL || (m_dispatch != NULL && m_dispatch->completed()))
    {
        continueRender = true;
        indRenderCount = -1;
        prevIndRenderCount = -1;
        String fullpath = m_scenePath + "/" + m_ddl->selectedValue().text();

        m_world.unload();
        m_world.setSettings(m_PSettings);
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
    if (!m_world.camnull() && m_dirBeams){


        m_dirBeams->setRadius(m_radius);


        m_dirBeams->makeBeams();

        // camera has changed, reset direct light
        if (indRenderCount == 0 && prevIndRenderCount == -1) {
            m_passes = 0;
            prevIndRenderCount = 0;
        }


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
    if (m_passes == 0) {

        rd->pushState(m_ZFBO); {
            rd->setObjectToWorldMatrix(CFrame());
            rd->setColorClearValue(Color4::zero());
            rd->clear();
            rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE);
            rd->setProjectionAndCameraMatrix(m_world.camera()->projection(), m_world.camera()->frame());

            Args args;
            args.setPrimitiveType(PrimitiveType::TRIANGLES);
            Array<shared_ptr<Surface>> geometry = m_world.geometry();
            CFrame cframe;
            for (int i=0; i<geometry.size(); i++)
            {
                const shared_ptr<UniversalSurface>& surface = dynamic_pointer_cast<UniversalSurface>(geometry[i]);
                if (notNull(surface))
                {
                    surface->getCoordinateFrame(cframe);
                    args.setUniform("MVP", rd->invertYMatrix()*rd->projectionMatrix()*rd->cameraToWorldMatrix().inverse() * cframe);
                    surface->gpuGeom()->setShaderArgs(args);
                    LAUNCH_SHADER("zBuff.*", args);
                }
            }
        } rd->popState();
    }

    Array<PhotonBeamette> direct_beams = Array<PhotonBeamette>();
    direct_beams.append(m_dirBeams->getBeams());

    float calcRadius = m_radius*m_PSettings->radiusScalingFactor;
    m_radius = max(calcRadius, 0.05f);
    m_passes += 1;

    // flipFlop FBOs and textures
    bool isEvenPass = m_passes % 2 == 0;
    auto prevFBO = isEvenPass ? m_FBO1 : m_FBO2;
    auto nextFBO = isEvenPass ? m_FBO2 : m_FBO1;



    /* splat beams */

    rd->pushState(m_dirFBO); {
        rd->setProjectionAndCameraMatrix(m_world.camera()->projection(),
                                         m_world.camera()->frame());

        Array<Vector3>   cpuVertex;
        Array<Vector3>   cpuMajor;
        Array<Vector3>   cpuMinor;
        Array<Color3>    cpuPower;

        for (PhotonBeamette pb : direct_beams) {
            cpuVertex.append(pb.m_start);
            cpuVertex.append(pb.m_end);

            cpuMajor.append(pb.m_start_major);
            cpuMinor.append(pb.m_start_minor);
            cpuMajor.append(pb.m_end_major);
            cpuMinor.append(pb.m_end_minor);
            cpuPower.append(pb.m_power/direct_beams.size() * pow(m_PSettings->beamIntensity, 2));
            cpuPower.append(pb.m_power/direct_beams.size() * pow(m_PSettings->beamIntensity, 2));
        }

        rd->setObjectToWorldMatrix(CFrame());
        rd->setColorClearValue(Color4::zero());
        rd->clear();

        glEnable(GL_BLEND);
        rd->setBlendFunc(Framebuffer::COLOR1,
                         RenderDevice::BLEND_ONE,
                         RenderDevice::BLEND_ONE,
                         RenderDevice::BLENDEQ_ADD,
                         RenderDevice::BLEND_SRC_ALPHA,
                         RenderDevice::BLEND_DST_ALPHA);

        // Upload to GPU
        shared_ptr<VertexBuffer> vbuffer = VertexBuffer::create(
                    sizeof(Vector3) * cpuVertex.size() +
                    sizeof(Vector3) * cpuMajor.size() +
                    sizeof(Vector3) * cpuMinor.size() +
                    sizeof(Color3) * cpuPower.size());
        AttributeArray gpuVertex   = AttributeArray(cpuVertex, vbuffer);
        AttributeArray gpuMajor   = AttributeArray(cpuMajor, vbuffer);
        AttributeArray gpuMinor   = AttributeArray(cpuMinor, vbuffer);
        AttributeArray gpuPower  = AttributeArray(cpuPower, vbuffer);
        Args args;

        args.setPrimitiveType(PrimitiveType::LINES);
        args.setAttributeArray("Position", gpuVertex);
        args.setAttributeArray("Major", gpuMajor);
        args.setAttributeArray("Minor", gpuMinor);
        args.setAttributeArray("Power", gpuPower);
        args.setUniform("zDepth", m_ZFBO->texture(1), Sampler::buffer());
        args.setUniform("Resolution", Vector2(rd->width(), rd->height()));
        args.setUniform("Look", m_world.camera()->frame().lookVector());
        args.setUniform("MVP",
                        rd->invertYMatrix() *
                        (rd->projectionMatrix() *
                         rd->cameraToWorldMatrix().inverse().toMatrix4()));

        LAUNCH_SHADER("beamsplat.*", args);

    } rd->popState();
    shared_ptr<Texture> indirectTex = Texture::fromImage("Source", m_canvas);


    /* composite direct and indirect */

    rd->push2D(nextFBO); {
        rd->setColorClearValue(Color4::zero());
        rd->clear();
        rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE);

        Args argsComp;
        argsComp.setRect(rd->viewport());

        argsComp.setUniform("Resolution", Vector2(rd->width(), rd->height()));
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

// sets the gather radius of the indirect renderer
void App::setGatherRadius()
{
    // the closer this value is to 1, the slower the radius will decrease.
    float radReductionRate = 1.08f;

    m_indRenderer->setGatherRadius(m_PSettings->gatherRadius / pow(radReductionRate, (indRenderCount - 1)));
}

void App::loadSceneDirectory(String directory)
{
    setScenePath(directory.c_str());

    m_ddl->clear();

    Array<String> sceneFiles;
    FileSystem::getFiles(directory + "/*.Any", sceneFiles);
    Array<String>::iterator scene;
    for (scene = sceneFiles.begin(); scene != sceneFiles.end(); ++scene)
    {
        m_ddl->append(*scene);
    }
}

void App::makeGUI()
{
    shared_ptr<GuiWindow> windowMain = GuiWindow::create("Main",
                                                     debugWindow->theme(),
                                                     Rect2D::xywh(0,0,60,60),
                                                     GuiTheme::NORMAL_WINDOW_STYLE);
    windowMain->setResizable(true); // a lie

    GuiPane* paneMain = windowMain->pane();

    // INFO
    GuiPane* infoPane = paneMain->addPane("Info", GuiTheme::ORNATE_PANE_STYLE);
    infoPane->addLabel("Controls: WASD forward/back/side");
    infoPane->addLabel("QE rotate about Y axis");
    infoPane->addLabel("ZC pan up/down");
    infoPane->pack();

    // SCENE
    GuiPane* scenesPane = paneMain->addPane("Scenes", GuiTheme::ORNATE_PANE_STYLE);

    // SCENE
    m_ddl = scenesPane->addDropDownList("Scenes");

    scenesPane->addLabel("View");
    scenesPane->addRadioButton("Default", App::DEFAULT, &view);
    scenesPane->addRadioButton("Photon Beams (Dir)", App::DIRBEAMS, &view);
    scenesPane->addRadioButton("Photon Beams (Indir)", App::INDBEAMS, &view);

    GuiButton* renderButton = scenesPane->addButton("Render", this, &App::onRender);
    renderButton->setFocused(true);
    scenesPane->pack();

    GuiPane* settingsPane = paneMain->addPane("Beam Settings", GuiTheme::ORNATE_PANE_STYLE);
    settingsPane->addNumberBox(GuiText("Scattering"), &m_PSettings->scattering, GuiText(""), GuiTheme::LINEAR_SLIDER, 0.0f, 1.0f, 0.05f);
    settingsPane->addNumberBox(GuiText("Attenuation"), &m_PSettings->attenuation, GuiText(""), GuiTheme::LINEAR_SLIDER, 0.0f, 1.0f, 0.05f);
    settingsPane->addNumberBox(GuiText("Intensity"), &m_PSettings->beamIntensity, GuiText(""), GuiTheme::LINEAR_SLIDER, 0.0f, 10.0f, 0.05f);
    settingsPane->addNumberBox(GuiText("Radius Scale"), &m_PSettings->radiusScalingFactor, GuiText(""), GuiTheme::LINEAR_SLIDER, 0.0f, 1.0f, 0.05f);
    settingsPane->addNumberBox(GuiText("Beam Spread"), &m_PSettings->beamSpread, GuiText(""), GuiTheme::LINEAR_SLIDER, 0.001f, 1.0f, 0.005f);
//    settingsPane->addNumberBox(GuiText("Curve Power"), &m_PSettings->curveScatterPower, GuiText(""), GuiTheme::LINEAR_SLIDER, 1.00f, 2.01f, 1.10f);
    settingsPane->addNumberBox(GuiText("Gather Radius"), &m_PSettings->gatherRadius, GuiText(""), GuiTheme::LINEAR_SLIDER, 0.0f, 1.0f, 0.05f);

    // Rendering
    GuiPane* renderPane = paneMain->addPane("Render Settings", GuiTheme::ORNATE_PANE_STYLE);
    renderPane->addCheckBox("Use Final Gather", &m_PSettings->useFinalGather);
    renderPane->pack();

    paneMain->pack();
    windowMain->pack();
    windowMain->setVisible(true);

    addWidget(windowMain);

    loadSceneDirectory(m_scenePath);
}