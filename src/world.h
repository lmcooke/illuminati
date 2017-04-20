#ifndef WORLD_H
#define WORLD_H

#include <fstream>
#include <sstream>
#include <string>

#include <G3D/G3DAll.h>

#include "photonmap.h"
#include "photonbeamette.h"

/** Represents a static scene with triangle mesh geometry, multiple lights, and
  * an initial camera specification
  */
class World
{
public:
    World();
    virtual ~World();

    /** Loads the geometry, lights and camera from a scene file.
      * Fails an assert if anything goes wrong.
      *
      * @param path The file to load (*.scn.any)
      */
    void load(const String &path);

    /** Clears the contents of this world object
      * Geometry and lights are cleared. The camera is not affected.
      */
    void unload();

    /** Gets the scene's camera */
    shared_ptr<Camera> camera();

    /** Picks a point of light from the scene that emits light from the scene.
      * The point is picked uniformly at random.
      * @param random   A random number generator
      * @param surf     The surface at the point picked
      * @param prob     Receives the probability of picking that point out of
      *                 all light-emitting points in the scene
      * @param area     The area of the emitter chosen
      */
    void emissivePoint(Random &random, shared_ptr<Surfel> &surf, float &prob, float &area);

    /** Emits a photon into the scene. If this method returns true, the
      * resulting photon will have already been shot into the scene (i.e. the
      * first bounce is done for you). If it returns false, the photon exited
      * the scene.
      *
      * @param random   A random number generator
      * @param photon   Receives the photon emitted
      * @param surf     The surface the photon hit on its first bounce
      *
      * @return         Whether or not there is a photon to continue scattering
      */
    bool emit(Random &random, Photon &photon, shared_ptr<Surfel> &surf);


    bool emitBeam(Random &random, PhotonBeamette &beam, shared_ptr<Surfel> &surf);

    /** Finds the first point a ray intersects with this scene
      *
      * @param ray  The ray to intersect
      * @param dist The distance from the ray origin to the point of
      *             intersection
      * @param surf The surface at the point of intersection
      */
    void intersect(const Ray &ray, float &dist, shared_ptr<Surfel> &surf);

    /** Determines whether an object occludes the line of sight from beg to end
     *
      * @param beg  The starting point
      * @param end  The ending point
      * @return     True if there is no geometry in the scene between the
      *             starting point and the ending point
      */
    bool lineOfSight(const Vector3 &beg, const Vector3 &end);

    /** Returns whether there are any lights in the scene */
    bool lightsExist() { return m_emit.size() > 0; }

    /** Sets the projection and camera matrices */
    void setMatrices(RenderDevice *dev);

    /** Renders the world in wireframe */
    void renderWireframe(RenderDevice *dev);

    /** Reads in spline file and parses it into a G3D Model.
     *  Spline files consist of a series of points, one per line, represented as:
     *  x y z radius
     *  The last line must be a comment starting with a #. */
    shared_ptr<ArticulatedModel> createSplineModel(const String& str);

    /** Returns exact beamette representation of splines used as spline lights,
     * for testing splatting
     */
    Array<PhotonBeamette> vizualizeSplines();

    TriTree             m_tris;     // The scene's geometry in world space

private:
    shared_ptr<Camera>  m_camera;   // The scene's camera
    Array<Tri>          m_emit;     // Triangles that emit light
    CPUVertexArray      m_verts;    // The scene's vertices
    Array<shared_ptr<Surface>> m_geometry;

    Array<Array<Vector4>> m_splines; // collection of spline lights, each light represented by x, y, z, radius
};

#endif
