#pragma once

#include <vector>
#include <set>
#include <sys/types.h>
#include <armadillo>
#include <assert.h>

#include <libconfig_utils/libconfig_utils.h>

using namespace arma;

class KMCSolver;
class Reaction;
class DiffusionReaction;

struct ParticleStates
{
    enum
    {
        crystal,
        solution,
        surface,
        any
    };

    const static vector<string> names;
    const static vector<string> shortNames;

};

class Site
{
public:

    Site(uint _x, uint _y, uint _z);

    ~Site();

    /*
     * Static non-trivial functions
     */

    static void setSolverPtr(KMCSolver* solver);

    static void loadConfig(const Setting & setting);

    static uint findLevel(uint i, uint j, uint k);

    static void resetAll()
    {
        m_totalActiveSites = 0;
        m_totalEnergy = 0;
        m_levelMatrix.reset();
        m_originTransformVector.reset();
    }

    /*
     * Non-trivial functions
     */

    void setParticleState(int state);


    bool isLegalToSpawn();

    void spawnAsFixedCrystal();

    void crystallize();


    void activate();

    void deactivate();


    void addReaction(Reaction* reaction);

    void addDependency(Reaction* reaction)
    {
        m_dependentReactions.push_back(reaction);
    }

    void setDiffusionReaction(DiffusionReaction* reaction, uint x, uint y, uint z);

    void enableReaction(Reaction *reaction);

    void disableReaction(Reaction *reaction);

    void introduceNeighborhood();

    bool hasNeighboring(int state);

    void propagateToNeighbors(int reqOldState, int newState);

    void informNeighborhoodOnChange(int change);


    void distanceTo(const Site * other, int &dx, int &dy, int &dz, bool absolutes = false) const;



    void reset()
    {
        m_nNeighbors.zeros();
        m_totalEnergy -= m_energy;
        m_energy = 0;
    }


    void dumpInfo(int xr = 0, int yr = 0, int zr = 0) const;


    /*
     * Misc. trivial functions
     */


    static const uint &nNeighborsLimit()
    {
        return m_nNeighborsLimit;
    }

    static const uint &neighborhoodLength()
    {
        return m_neighborhoodLength;
    }

    static const ucube &levelMatrix()
    {
        return m_levelMatrix;
    }

    static const ivec &originTransformVector()
    {
        return m_originTransformVector;
    }

    static const uint & totalActiveSites()
    {
        return m_totalActiveSites;
    }

    static const double & totalEnergy()
    {
        return m_totalEnergy;
    }


    const int & particleState()
    {
        return m_particleState;
    }

    uint nNeighbors(uint level = 0) const
    {
        return m_nNeighbors(level);
    }


    bool isCrystal()
    {
        return m_particleState == ParticleStates::crystal;
    }

    bool isSurface()
    {
        return m_particleState == ParticleStates::surface;
    }

    const bool & isActive() const
    {
        return m_active;
    }


    const uint & x() const
    {
        return m_x;
    }

    const uint & y() const
    {
        return m_y;
    }

    const uint & z() const
    {
        return m_z;
    }

    const vector<Reaction*> & siteReactions() const
    {
        return m_siteReactions;
    }

    const vector<Reaction*> & dependentReactions() const
    {
        return m_dependentReactions;
    }

    const vector<Reaction*> & activeReactions() const
    {
        return m_activeReactions;
    }

    const vector<Site*> & allNeighbors() const
    {
        return m_allNeighbors;
    }

    Site**** neighborHood() const
    {
        return m_neighborHood;
    }

    double energy() const
    {
        return m_energy;
    }

    bool operator == (const Site & other) const
    {
        return this == &other;
    }


    friend class testBed;

private:

    static uint m_nNeighborsLimit;
    static uint m_neighborhoodLength;

    static ucube m_levelMatrix;
    static ivec m_originTransformVector;

    static uint m_totalActiveSites;

    static uint NX;
    static uint NY;
    static uint NZ;

    static double m_totalEnergy;

    static void updateAffectedReactions();

    static set<Reaction*> affectedReactions;

    void queueAffectedReactions();

    static KMCSolver* mainSolver;

    Site**** m_neighborHood;
    vector<Site*> m_allNeighbors;
    uvec m_nNeighbors;

    bool m_active;

    bool isFixedCrystalSeed;

    uint m_x;
    uint m_y;
    uint m_z;

    double m_energy;

    int m_particleState = ParticleStates::solution;

    vector<Reaction*> m_siteReactions;

    vector<Reaction*> m_activeReactions;

    vector<Reaction*> m_dependentReactions;

};


ostream& operator<<(ostream& os, const Site& ss);

