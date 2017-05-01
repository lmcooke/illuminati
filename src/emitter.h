#ifndef EMITTER_H
#define EMITTER_H
#include <G3D/G3DAll.h>

class emitter
        : public G3D::Tri
{
public:
    emitter();
    int m_splineIndex; // index for associated spline (-1 if not associated with any spline aka normal area light)
};

#endif // EMITTER_H
