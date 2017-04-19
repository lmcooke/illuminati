QT -= core gui
TARGET = photon-bin
TEMPLATE = app

# G3D_PATH - absolute path to G3D Library

G3D_PATH = $$(G3D_PATH)

isEmpty(G3D_PATH) {
    # default sunlab path
    G3D_PATH = /contrib/projects/g3d10/G3D10
}

# convert relative paths and sanitize
# G3D_PATH = $$absolute_path($${G3D_PATH})
# G3D_PATH = $$system_path($${G3D_PATH})

message("G3D Path : " $${G3D_PATH})

SOURCES += app.cpp \
           world.cpp \
           photonmap.cpp \
    photonscatter.cpp \
    indphotonscatter.cpp \
    dirphotonscatter.cpp \
    photonbeamette.cpp

HEADERS += app.h \
           world.h \
           photonmap.h \
    photonscatter.h \
    indphotonscatter.h \
    dirphotonscatter.h \
    photonbeamette.h

INCLUDEPATH += $${G3D_PATH}/build/include \
            += $${G3D_PATH}/tbb/include

LIBS += \
    -L$${G3D_PATH}/build/lib \
    -lGLG3D \
    -lG3D \
    -lassimp \
    -lglfw \
    -lXrandr \
    -lGLU \
    -lX11 \
    -lfreeimage \
    -lzip \
    -lz \
    -lGL \
    -lpthread \
    -lXi \
    -lXxf86vm \
    -lrt \
    -lenet \
    -ltbb \
    -lglew \
    -lXcursor

QMAKE_CXXFLAGS += -std=c++14 -msse4.1

QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O3 -fno-strict-aliasing
QMAKE_CXXFLAGS_WARN_ON -= -Wall
QMAKE_CXXFLAGS_WARN_ON += -Waddress -Warray-bounds -Wc++0x-compat -Wchar-subscripts -Wformat\
                          -Wmain -Wmissing-braces -Wparentheses -Wreorder -Wreturn-type \
                          -Wsequence-point -Wsign-compare -Wstrict-aliasing -Wstrict-overflow=1 -Wswitch \
                          -Wtrigraphs -Wuninitialized -Wunused-label -Wunused-variable \
                          -Wvolatile-register-var -Wno-extra

