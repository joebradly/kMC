include(../../defaults.pri) 

TEMPLATE = lib 
TARGET = kMC

HEADERS = RNG/kMCRNG.h \
    reactions/reaction.h \
    kmcsolver.h \
    site.h \
    reactions/diffusion/diffusionreaction.h \
    debugger/kmcdebugger.h \
    debugger/bits/nodebug.h \
    debugger/bits/intrinsicmacros.h \
    debugger/bits/kmcdebugger_class.h \
    debugger/bits/debug_api.h

SOURCES += \
    reactions/reaction.cpp \
    kmcsolver.cpp \
    site.cpp \
    reactions/diffusion/diffusionreaction.cpp \
    RNG/kMCRNG.cpp \
    debugger/bits/kmcdebugger_class.cpp

RNG_ZIG {

HEADERS += RNG/zigrandom.h \
           RNG/zignor.h

SOURCES += RNG/zigrandom.cpp \
           RNG/zignor.cpp

}


