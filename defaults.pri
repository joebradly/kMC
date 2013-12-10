LIBS += -llapack -larmadillo -lconfig++
CONFIG -= app_bundle
CONFIG -= qt
CONFIG += RNG_ZIG


COMMON_CXXFLAGS = -std=c++0x
QMAKE_CXXFLAGS += $$COMMON_CXXFLAGS
QMAKE_CXXFLAGS_DEBUG += $$COMMON_CXXFLAGS
QMAKE_CXXFLAGS_RELEASE += $$COMMON_CXXFLAGS -O3 -DARMA_NO_DEBUG

# Directories
INCLUDEPATH += $$TOP_PWD/src/libs
SRC_DIR = $$TOP_PWD

copydata.commands = $(COPY_DIR) $$PWD/infiles $$OUT_PWD
first.depends = $(first) copydata
export(first.depends)
export(copydata.commands)
QMAKE_EXTRA_TARGETS += first copydata


CONFIG(RNG_ZIG) {
    DEFINES += KMC_RNG_ZIG
}
