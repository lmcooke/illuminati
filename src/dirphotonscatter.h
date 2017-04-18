#ifndef DIRPHOTONSCATTER_H
#define DIRPHOTONSCATTER_H
#include "photonscatter.h"
#include <G3D/G3DAll.h>

class DirPhotonScatter
    : public PhotonScatter
{
public:
    DirPhotonScatter();
    ~DirPhotonScatter();
    void preprocess();

    void initialize(shared_ptr<Framebuffer> framebuffer, RenderDevice *rd);

    void renderDirect(RenderDevice *rd);

private:


    shared_ptr<Framebuffer> m_fb;
    shared_ptr<Texture> m_directTex;



};

#endif // DIRPHOTONSCATTER_H
