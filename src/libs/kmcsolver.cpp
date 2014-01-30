#include "kmcsolver.h"
#include "RNG/kMCRNG.h"

#include "site.h"
#include "reactions/reaction.h"
#include "reactions/diffusion/diffusionreaction.h"

#include <sys/time.h>

#include <armadillo>
using namespace arma;

#include <iostream>
#include <iomanip>
#include <fstream>

using namespace std;

KMCSolver::KMCSolver(const Setting & root)
{

    const Setting & SystemSettings = getSurfaceSetting(root, "System");
    const Setting & SolverSettings = getSurfaceSetting(root, "Solver");
    const Setting & InitializationSettings = getSurfaceSetting(root, "Initialization");

    const Setting & BoxSize = getSurfaceSetting(SystemSettings, "BoxSize");


    NX = BoxSize[0];
    NY = BoxSize[1];
    NZ = BoxSize[2];

    nCycles = getSurfaceSetting<uint>(SolverSettings, "nCycles");
    cyclesPerOutput = getSurfaceSetting<uint>(SolverSettings, "cyclesPerOutput");

    saturation = getSurfaceSetting<double>(InitializationSettings, "SaturationLevel");
    RelativeSeedSize = getSurfaceSetting<double>(InitializationSettings, "RelativeSeedSize");
    assert(RelativeSeedSize < 1.0 && "The seed size cannot exceed the box size.");

    Site::loadNeighborLimit(SystemSettings);
    Site::setSolverPtr(this);

    Reaction::loadTemperature(getSurfaceSetting(root, "Reactions"));
    Reaction::setSolverPtr(this);

    DiffusionReaction::loadPotential(getSetting(root, {"Reactions", "Diffusion"}));



    KMC_INIT_RNG(time(NULL));

    sites = new Site***[NX];

    for (uint x = 0; x < NX; ++x) {

        sites[x] = new Site**[NY];
        for (uint y = 0; y < NY; ++y) {

            sites[x][y] = new Site*[NZ];

            for (uint z = 0; z < NZ; ++z) {

                sites[x][y][z] = new Site(x, y, z);
            }
        }
    }


    for (uint x = 0; x < NX; ++x) {
        for (uint y = 0; y < NY; ++y) {
            for (uint z = 0; z < NZ; ++z) {
                sites[x][y][z]->introduceNeighborhood();
            }
        }
    }



}



void KMCSolver::run(){

    uint choice;
    double R;

    initialize();

    while(cycle < nCycles)
    {

        getRateVariables();


        R = kTot*KMC_RNG_UNIFORM();

        choice = getReactionChoice(R);

        allReactions[choice]->execute();


        if (cycle%cyclesPerOutput == 0)
        {
            dumpOutput();
            dumpXYZ();
        }


        totalTime += 1.0/kTot;
        cycle++;

    }

}



void KMCSolver::dumpXYZ()
{
    stringstream s;
    s << "kMC" << outputCounter++ << ".xyz";

    ofstream o;
    o.open("outfiles/" + s.str());
    o << Site::totalActiveSites() << "\n - ";

    for (uint i = 0; i < NX; ++i) {
        for (uint j = 0; j < NY; ++j) {
            for (uint k = 0; k < NZ; ++k) {
                if (sites[i][j][k]->active()) {
                    o << "\nC " << i << " " << j << " " << k << " " << sites[i][j][k]->nNeighbors();
                }
            }
        }
    }

    o.close();

}

void KMCSolver::dumpOutput()
{
    cout << setw(5) << right << setprecision(1) << fixed
         << (double)cycle/nCycles*100 << "%   "
         << cycle
         << endl;
}

void KMCSolver::getAllNeighbors()
{
    for (uint i = 0; i < NX; ++i) {
        for (uint j = 0; j < NY; ++j) {
            for (uint k = 0; k < NZ; ++k) {
                sites[i][j][k]->countNeighbors();
            }
        }
    }
}


void KMCSolver::setDiffusionReactions()
{

    Site* currentSite;

    //Loop over all sites
    for (uint x = 0; x < NX; ++x) {
        for (uint y = 0; y < NY; ++y) {
            for (uint z = 0; z < NZ; ++z) {

                currentSite = sites[x][y][z];

                //For each site, loop over all neightbours
                for (uint i = 0; i < 3; ++i) {
                    for (uint j = 0; j < 3; ++j) {
                        for (uint k = 0; k < 3; ++k) {

                            //This menas we are at the current site.
                            if((i == 1) && (j == 1) && (k == 1)) {
                                continue;
                            }

                            //And add diffusion reactions
                            currentSite->addReaction(new DiffusionReaction(currentSite->getNeighborhood()[i][j][k]));

                        }
                    }
                }

                //Then we update the site reactions based on the current setup
                currentSite->updateReactions();

                //And calculate the process rates (only dependent on other sites, not reactions)
                currentSite->calculateRates();

            }
        }
    }
}


void KMCSolver::initialize()
{

    for (uint i = 0; i < NX; ++i) {
        for (uint j = 0; j < NY; ++j) {
            for (uint k = 0; k < NZ; ++k) {
                if (KMC_RNG_UNIFORM() > 1 - saturation) {

                    sites[i][j][k]->activate();

                }
            }
        }
    }


    int D = NX*RelativeSeedSize/2;

    for (uint i = NX/2 - D; i < NX/2 + D; ++i) {
        for (uint j = NY/2 - D; j < NY/2 + D; ++j) {
            for (uint k = NZ/2 - D; k < NZ/2 + D; ++k) {
                if (!sites[i][j][k]->active()){
                    sites[i][j][k]->activate();
                }
            }
        }
    }


    getAllNeighbors();

    cout << "Initialized "
         << Site::totalActiveSites()
         << " active sites."
         << endl;

    dumpXYZ();

    setDiffusionReactions();

}



void KMCSolver::getRateVariables()
{

    kTot = 0;
    accuAllRates.clear();
    allReactions.clear();

    for (uint x = 0; x < NX; ++x) {
        for (uint y = 0; y < NY; ++y) {
            for (uint z = 0; z < NZ; ++z) {
                for (Reaction* reaction : sites[x][y][z]->activeReactions()) {
                    kTot += reaction->rate();
                    accuAllRates.push_back(kTot);
                    allReactions.push_back(reaction);
                }
            }
        }
    }

}


Reaction* KMCSolver::getChosenReaction(uint choice)
{
    uint K = 0;

    for (uint x = 0; x < NX; ++x) {
        for (uint y = 0; y < NY; ++y) {
            for (uint z = 0; z < NZ; ++z) {

                for (Reaction* reaction : sites[x][y][z]->activeReactions()) {

                    if (K == choice) {
                        return reaction;
                    }

                    K++;
                }

            }
        }
    }

    cout << "FAIL AT CHOOSING REACTION" << endl;
    exit(1);
}

uint KMCSolver::getReactionChoice(double R)
{

    uint imax = accuAllRates.size();
    uint imin = 0;
    uint imid = 1;

    // continue searching while imax != imin + 1
    while (imid != imin)
    {
        // calculate the midpoint for roughly equal partition
        imid = imin + (imax - imin)/2;

        if (imid == imin) {
            return imid;
        }

        //perform two tests due to integer division
        else if (accuAllRates.at(imid - 1) < R && accuAllRates.at(imid) > R) {
            return imid-1;
        }
        else if (accuAllRates.at(imid) < R && accuAllRates.at(imid + 1) > R) {
            return imid;
        }

        else if (accuAllRates.at(imid) < R) {
            imin = imid + 1;
        } else {
            imax = imid - 1;
        }


    }

    cout << "LOCATING RATE FAILED" << endl;
    return 0;

}


