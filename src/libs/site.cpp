#include "site.h"
#include "reactions/reaction.h"
#include "reactions/diffusion/diffusionreaction.h"
#include "kmcsolver.h"



Site::Site(uint _x, uint _y, uint _z) :
    m_active(false),
    m_x(_x),
    m_y(_y),
    m_z(_z),
    m_energy(0),
    m_particleState(particleState::solution)
{


}


Site::~Site()
{
    for (uint i = 0; i < neighborhoodLength(); ++i)
    {
        for (uint j = 0; j < neighborhoodLength(); ++j)
        {
            delete [] m_neighborHood[i][j];
        }

        delete [] m_neighborHood[i];
    }

    delete m_neighborHood;


    for (Reaction* reaction : m_siteReactions)
    {
        delete reaction;
    }

    m_activeReactions.clear();

    m_siteReactions.clear();

    m_allNeighbors.clear();

    m_nNeighbors.reset();

    m_energy = 0;

}




void Site::updateAffectedSites()
{

    for (Site* site : affectedSites)
    {
        site->updateReactions();
        site->calculateRates();
    }

    affectedSites.clear();

}


void Site::setParticleState(int state)
{

    //If we try to propagate a surface onto an existing active site
    //The site is crystallized, which propagates the surface furhter.
    switch (state)
    {
    case particleState::surface:

        switch (m_particleState)
        {

        //solution->surface
        case particleState::solution:

            //if a particle is present, we crystallize it immidiately.
            if (m_active)
            {
                crystallize();
            }

            else
            {
                m_particleState = particleState::surface;
                queueAffectedSites();
            }

            break;

            //crystal->surface
        case particleState::crystal:
            m_particleState = particleState::surface;
            propagateToNeighbors(particleState::surface, particleState::solution);
            queueAffectedSites();

            break;

            //surface -> surface
        case particleState::surface:
            //Nothing to do here.
            break;

        default:
            cout << "invalid transition" << m_particleState << "->" << state << endl;
            exit(1);
            break;
        }

        break;

    case particleState::crystal:

        switch (m_particleState)
        {

        //surface -> crystal
        case particleState::surface:
            crystallize();
            break;

        default:
            cout << "invalid transition" << m_particleState << "->" << state << endl;
            exit(1);
            break;
        }

        break;


    case particleState::solution:

        switch (m_particleState)
        {

        //surface -> solution
        case particleState::surface:

            if (!hasNeighboring(particleState::crystal))
            {
                m_particleState = particleState::solution;
                queueAffectedSites();
            }

            break;
        default:
            cout << "invalid transition" << m_particleState << "->" << state << endl;
            exit(1);
            break;
        }
        break;


    default:
        cout << "invalid transition" << m_particleState << "->" << state << endl;
        exit(1);
        break;
    }

}


//All reactions must be legal if site is allowed to spawn.
bool Site::isLegalToSpawn()
{

    if (m_active)
    {
        return false;
    }

    for (Reaction * r : m_siteReactions)
    {
        if (!r->allowedAtSite())
        {
            return false;
        }
    }


    return true;

}


void Site::crystallize()
{
    m_particleState  = particleState::crystal;
    propagateToNeighbors(particleState::solution, particleState::surface);

}


void Site::loadConfig(const Setting &setting)
{

    const uint  &limit = getSurfaceSetting<uint>(setting, "nNeighborsLimit");

    m_nNeighborsLimit = limit;
    m_neighborhoodLength = 2*m_nNeighborsLimit + 1;

    m_levelMatrix.set_size(m_neighborhoodLength, m_neighborhoodLength, m_neighborhoodLength);

    m_originTransformVector = linspace<ivec>(-(int)m_nNeighborsLimit, m_nNeighborsLimit, m_neighborhoodLength);

    for (uint i = 0; i < m_neighborhoodLength; ++i)
    {
        for (uint j = 0; j < m_neighborhoodLength; ++j)
        {
            for (uint k = 0; k < m_neighborhoodLength; ++k)
            {
                if (i == m_nNeighborsLimit && j == m_nNeighborsLimit && k == m_nNeighborsLimit)
                {
                    m_levelMatrix(i, j, k) = m_nNeighborsLimit + 1;
                    continue;
                }

                m_levelMatrix(i, j, k) = findLevel(std::abs(m_originTransformVector(i)),
                                                  std::abs(m_originTransformVector(j)),
                                                  std::abs(m_originTransformVector(k)));
            }
        }
    }

}


void Site::addReaction(Reaction *reaction)
{
    reaction->setSite(this);

    m_siteReactions.push_back(reaction);
}


void Site::updateReactions()
{

    m_activeReactions.clear();

    if (!m_active)
    {
        return;
    }

    for (Reaction* reaction : m_siteReactions)
    {
        if (reaction->isNotBlocked())
        {
            m_activeReactions.push_back(reaction);
        }
    }

}


void Site::spawnAsCrystal()
{
    m_particleState = particleState::surface;
    activate();
}


void Site::calculateRates()
{
    for (Reaction* reaction : m_activeReactions)
    {
        reaction->calcRate();
    }
}


void Site::setSolverPtr(KMCSolver *solver)
{

    NX = solver->getNX();
    NY = solver->getNY();
    NZ = solver->getNZ();

    mainSolver = solver;

}


void Site::distanceTo(const Site *other, int &dx, int &dy, int &dz, bool absolutes) const
{

    dx = (other->x() + NX - m_x)%NX;
    dy = (other->y() + NY - m_y)%NY;
    dz = (other->z() + NZ - m_z)%NZ;


    if ((uint)abs(dx) > NX/2) {
        dx = -(int)(NX - dx);
    }

    if ((uint)abs(dy) > NY/2) {
        dy = -(int)(NY - dy);
    }

    if ((uint)abs(dz) > NZ/2) {
        dz = -(int)(NZ - dz);
    }


    if (absolutes) {
        dx = abs(dx);
        dy = abs(dy);
        dz = abs(dz);
    }

}

bool Site::hasNeighboring(int state)
{

    Site *nextNeighbor;

    for (uint i = 0; i < 3; ++i)
    {
        for (uint j = 0; j < 3; ++j)
        {
            for (uint k = 0; k < 3; ++k)
            {
                if (i == 1 && j == 1 && k == 1)
                {
                    continue;
                }

                nextNeighbor = m_neighborHood[i + Site::nNeighborsLimit() - 1]
                        [j + Site::nNeighborsLimit() - 1]
                        [k + Site::nNeighborsLimit() - 1];

                if (nextNeighbor->particleState() == state)
                {
                    return true;
                }

            }
        }
    }

    return false;

}

void Site::activate()
{

#ifndef NDEBUG

    if (affectedSites.size() != 0)
    {
        cout << "affectedsites not cleared." << endl;
        cout << affectedSites.size() << " != " << 0 << endl;

        exit(1);
    }

    if (m_active == true)
    {
        cout << "Activating active site. " << endl;
        dumpInfo();
        exit(1);
    }

    else if (isCrystal())
    {
        cout << "Activating a crystal. (should always be active)";
        dumpInfo();
        exit(1);
    }

#endif

    m_active = true;

    if (isSurface())
    {
        setParticleState(particleState::crystal);
    }

    affectedSites.insert(this);

    informNeighborhoodOnChange(+1);
    queueAffectedSites();

    updateAffectedSites();

    m_totalActiveSites++;

}

void Site::deactivate()
{

#ifndef NDEBUG

    assert(affectedSites.size() == 0);

    if (m_active == false)
    {
        cout << "deactivating deactive site. " << endl;
        dumpInfo();
        exit(1);
    }
    else if (isSurface())
    {
        cout << "deactivating a surface. (should always be deactive)";
        dumpInfo();
        exit(1);
    }
#endif

    m_active = false;

    //if we deactivate a crystal site, we have to potentially
    //reduce the surface by adding more sites as solution sites.
    //Site will change only if it is not surrounded by any crystals.
    if (isCrystal())
    {
        setParticleState(particleState::surface);
    }

    informNeighborhoodOnChange(-1);
    queueAffectedSites();

    updateAffectedSites();

    m_totalActiveSites--;

}


void Site::introduceNeighborhood()
{

    assert(m_nNeighborsLimit != 0);

    uint xTrans, yTrans, zTrans;

    m_nNeighbors.set_size(m_nNeighborsLimit);
    m_nNeighbors.zeros();

    m_neighborHood = new Site***[m_neighborhoodLength];

    for (uint i = 0; i < m_neighborhoodLength; ++i)
    {
        xTrans = (m_x + m_originTransformVector(i) + NX)%NX;

        m_neighborHood[i] = new Site**[m_neighborhoodLength];

        for (uint j = 0; j < m_neighborhoodLength; ++j)
        {
            yTrans = (m_y + m_originTransformVector(j) + NY)%NY;

            m_neighborHood[i][j] = new Site*[m_neighborhoodLength];

            for (uint k = 0; k < m_neighborhoodLength; ++k)
            {
                zTrans = (m_z + m_originTransformVector(k) + NZ)%NZ;

                m_neighborHood[i][j][k] = mainSolver->getSites()[xTrans][yTrans][zTrans];

                if (m_neighborHood[i][j][k] != this)
                {
                    m_allNeighbors.push_back(m_neighborHood[i][j][k]);
                }
            }
        }
    }

}

void Site::propagateToNeighbors(int reqOldState, int newState)
{

    Site *nextNeighbor;

    for (uint i = 0; i < 3; ++i)
    {
        for (uint j = 0; j < 3; ++j)
        {
            for (uint k = 0; k < 3; ++k)
            {

                nextNeighbor = m_neighborHood[i + Site::nNeighborsLimit() - 1]
                        [j + Site::nNeighborsLimit() - 1]
                        [k + Site::nNeighborsLimit() - 1];

                assert(!(newState == particleState::solution && nextNeighbor->particleState() == particleState::solution));
                if (nextNeighbor == this)
                {
                    assert(i == j && j == k && k == 1);
                    continue;
                }

                if (nextNeighbor->particleState() == reqOldState || reqOldState == particleState::any)
                {
                    nextNeighbor->setParticleState(newState);
                }

            }
        }
    }

}

void Site::informNeighborhoodOnChange(int change)
{

    Site *neighbor;
    uint level;
    double dE;

    for (uint i = 0; i < m_neighborhoodLength; ++i)
    {
        for (uint j = 0; j < m_neighborhoodLength; ++j)
        {
            for (uint k = 0; k < m_neighborhoodLength; ++k)
            {

                neighbor = m_neighborHood[i][j][k];

                if (neighbor == this) {
                    assert(i == j && j == k && k == m_nNeighborsLimit);
                    continue;
                }

                level = m_levelMatrix(i, j, k);
                neighbor->m_nNeighbors(level)+=change;


                dE = change*DiffusionReaction::potential()(i, j, k);

                neighbor->m_energy += dE;

                m_totalEnergy += dE;

            }
        }
    }

}

void Site::queueAffectedSites()
{
    affectedSites.insert(allNeighbors().begin(), allNeighbors().end());
}

uint Site::findLevel(uint i, uint j, uint k)
{

    uint m = i;

    if (j > i)
    {
        m =  j;
    }

    if (k > m)
    {
        m = k;
    }

    return m - 1;

}


void Site::dumpInfo(int xr, int yr, int zr)
{

    cout << "Site   " << m_x << " " << m_y << " " << m_z << endl;
    cout << "in Box " << NX << " " << NY << " " << NZ << endl;
    cout << "nNeighbors : " << m_nNeighbors.t();
    cout << "type: " << particleState::names.at(m_particleState) << endl;

    if (m_active)
    {
        cout << "ACTIVE";
    }

    else
    {
        cout << "DEACTIVE";
    }

    cout << endl;

    ucube nN;
    nN.copy_size(m_levelMatrix);
    nN.zeros();

    for (uint i = 0; i < m_neighborhoodLength; ++i)
    {
        for (uint j = 0; j < m_neighborhoodLength; ++j)
        {
            for (uint k = 0; k < m_neighborhoodLength; ++k)
            {

                if (i == j && j == k && k == Site::nNeighborsLimit())
                {
                    nN(i, j, k) = 3;
                }

                else if ((i == Site::nNeighborsLimit() + xr) && (j == Site::nNeighborsLimit() + yr) && (k == Site::nNeighborsLimit() + zr))
                {
                    nN(i, j, k) = 2;
                }

                else if (m_neighborHood[i][j][k]->isActive())
                {
                    nN(i, j, k) = 1;
                }

            }

        }

    }

    umat A;
    stringstream ss;
    for(int i = nN.n_slices - 1; i >= 0; --i)
    {
        A = nN.slice(i).t();

        for (int j = A.n_rows - 1; j >= 0; --j)
        {
            ss << A.row(j);
        }

        ss << endl;

    }


    string s = ss.str();

    int position = s.find("0");
    while (position != (int)string::npos)
    {
        s.replace(position, 1, ".");
        position = s.find("0", position + 1);
    }

    position = s.find("1");
    while (position != (int)string::npos)
    {
        s.replace(position, 1, "X");
        position = s.find("1", position + 1);
    }

    position = s.find("2");
    while (position != (int)string::npos)
    {
        s.replace(position, 1, "O");
        position = s.find("2", position + 1);
    }

    position = s.find("3");
    while (position != (int)string::npos)
    {
        s.replace(position, 1, "#");
        position = s.find("3", position + 1);
    }

    cout << s << endl;

}


KMCSolver* Site::mainSolver;

uint       Site::NX;
uint       Site::NY;
uint       Site::NZ;

uint       Site::m_nNeighborsLimit;

uint       Site::m_neighborhoodLength;

ucube      Site::m_levelMatrix;
ivec       Site::m_originTransformVector;

uint       Site::m_totalActiveSites = 0;

double     Site::m_totalEnergy = 0;

set<Site*> Site::affectedSites;


const vector<string> particleState::names = {"crystal", "solution", "surface", "any"};
const vector<string> particleState::shortNames = {"C", "P", "S", "X"};
