#ifndef EMITTER_H
#define EMITTER_H
#include <G3D/G3DAll.h>

class Emitter
{
public:
    Emitter();
    Emitter(int index, Tri &tri);
    ~Emitter();
    int m_splineIndex; // index for associated spline (-1 if not associated with any spline aka normal area light)
    Tri m_tri;

    int index(){
        return m_splineIndex;
    }

    Tri tri(){
        return m_tri;
    }
};

#endif // EMITTER_H
