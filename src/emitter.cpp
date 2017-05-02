#include "emitter.h"
#include <G3D/G3DAll.h>

Emitter::Emitter():
    m_splineIndex(-1)
//      m_splineIndex(index),
//      m_tri(tri)
{
}

Emitter::Emitter(int index, Tri &tri){
    m_splineIndex = index;
    m_tri = tri;
}

Emitter::~Emitter(){
}
