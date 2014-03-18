#pragma once

#include "../boundary.h"

namespace kMC
{


class ConcentrationWall : public Boundary
{
public:
    ConcentrationWall(const uint dimension, const uint orientation);
    ~ConcentrationWall();

    void setMinDistanceFromSite(const uint minDistanceFromSite)
    {
        m_minDistanceFromSurface = minDistanceFromSite;
    }

    // Boundary interface
public:
    void update();
    void initialize();

private:

    uint m_minDistanceFromSurface;

    umat::fixed<3, 2> crystalBoxTopology;

};

}
