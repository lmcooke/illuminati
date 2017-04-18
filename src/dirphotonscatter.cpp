#include "dirphotonscatter.h"

DirPhotonScatter::DirPhotonScatter()
    : PhotonScatter()
{
}

DirPhotonScatter::~DirPhotonScatter()
{

}


void DirPhotonScatter::preprocess()
{
}

void DirPhotonScatter::initialize(shared_ptr<Framebuffer> framebuffer,
                                  RenderDevice* rd)
{
    // initialize primary framebuffer
    m_fb = framebuffer;

    // initialize textures to bind to FBO
    m_directTex = Texture::createEmpty("DirPhotonScatter::directLight", m_fb->width(),
                                       m_fb->height(), ImageFormat::RGBA16F());

    // set color attachment point
    m_fb->set(Framebuffer::AttachmentPoint::COLOR1, m_directTex);
}

void DirPhotonScatter::renderDirect(RenderDevice* rd)
{

    rd->pushState(m_fb); {

    } rd->popState();

//    swapBuffers();

}
