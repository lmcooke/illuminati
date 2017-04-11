#ifndef PHOTONMAP_H
#define PHOTONMAP_H

#include <G3D/G3DAll.h>

class World;

/** Represents a single photon traced through a scene */
class Photon
{
public:
    Photon();
    virtual ~Photon();

    /** The position of this photon, in world space */
    Point3 position;

    /** The direction from which the photon arrived. 
      * In accordance with G3D's BRDFs, this vector points away from the photon
      */
    Vector3 wi;

    /** The power transported by this photon */
    Power3 power;

    // Helpers for PointHashGrid
    // (see PhotonMap below)
    static void getPosition(const Photon &photon, Vector3 &outPosition);
    static bool equals(const Photon &p, const Photon &q);
    bool operator==(const Photon &other) const;
};


/** Spatial acceleration data structure for storing and quickly retrieving
  * photons in the scene.
  * For documentation, refer to the G3D manual.
  * G3D::FastPointHashGrid<> implements the same interface as
  * G3D::PointHashGrid, which happens to be more well-documented
  */
class PhotonMap
    : public FastPointHashGrid<Photon, Photon> 
{
public:
    /** Renders the contents of this photon map as well as the world in wireframe to the
     *  given render device dev.
     */
    void render(RenderDevice *dev, World *world);
};

#endif
