#pragma once


#include <exception>

#ifdef KMC_RNG_ZIG

#include "zignor.h"
#include "zigrandom.h"

#define KMC_RNG_NORMAL DRanNormalZig32

#define KMC_RNG_UNIFORM DRan_MWC8222

typedef int seed_type;

#define KMC_INIT_RNG(seed)                  \
    Seed::initialSeed = seed;               \
    int inseed = static_cast<int>(seed);    \
    int cseed = 100;                        \
    int seed2 = inseed * 3;                 \
    RanSetSeed_MWC8222(&seed2, cseed);      \
    RanNormalSetSeedZig32(&inseed, 5)



#endif

struct Seed
{

    enum SeedState
    {
        fromTime,
        specific
    };


    static seed_type initialSeed;


    class SeedNotSetException : public std::exception
    {
    public:
        virtual const char* what() const throw()
        {
            return "Seed not set.";
        }
    } static seedNotSetException;


};
