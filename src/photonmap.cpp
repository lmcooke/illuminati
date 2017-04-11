#include "photonmap.h"
#include "world.h"

Photon::Photon() { }

Photon::~Photon() { }

void Photon::getPosition(const Photon &photon, Vector3 &outPosition)
{
    outPosition = photon.position;
}

bool Photon::equals(const Photon &p, const Photon &q)
{
    return p == q;
}

bool Photon::operator==(const Photon &other) const
{
    return position == other.position &&
           wi == other.wi &&
           power == other.power;
}

void PhotonMap::render(RenderDevice *dev, World *world)
{
    world->renderWireframe(dev);

    dev->pushState();
    // Bad design on display here
    world->setMatrices(dev);
    SlowMesh mesh(PrimitiveType::POINTS);
    mesh.setPointSize(1);
    for (PhotonMap::Iterator photon = begin(); photon.isValid(); ++photon) {
        mesh.setColor(photon->power / photon->power.max());
        mesh.makeVertex(photon->position);
    }
    mesh.render(dev);
    dev->popState();
}
