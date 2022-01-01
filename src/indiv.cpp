// indiv.cpp

#include <iostream>
#include <cassert>
#include "indiv.h"
// #include "simulator.h"



// This is called when any individual is spawned.
// The responsiveness parameter will be initialized here to maximum value
// of 1.0, then depending on which action activation function is used,
// the default undriven value may be changed to 1.0 or action midrange.
void Indiv::initialize(uint16_t index_, Coord loc_, Genome &&genome_)
{
    index = index_;
    loc = loc_;
    //birthLoc = loc_;
    grid.set(loc_, index_);
    age = 0;
    oscPeriod = 34; // ToDo !!! define a constant
    alive = true;
    lastMoveDir = Dir::random8();
    responsiveness = 0.5; // range 0.0..1.0
    longProbeDist = p.longProbeDistance;
    challengeBits = (unsigned)false; // will be set true when some task gets accomplished
    genome = std::move(genome_);
    createWiringFromGenome();

    // color[0] = genome.size() & 0xF | ((genome.front().sourceNum & 0xF) << 4);
    // color[1] = genome..sourceType & 0xF | ((genome.back().sourceNum & 0xF << 4);
    // color[2] = 
  // TODO: fix this  
    std::uint8_t c = ((genome.size() & 1)
         | ((genome.front().sourceType)    << 1)
         | ((genome.back().sourceType)     << 2)
         | ((genome.front().sinkType)      << 3)
         | ((genome.back().sinkType)       << 4)
         | ((genome.front().sourceNum & 1) << 5)
         | ((genome.front().sinkNum & 1)   << 6)
         | ((genome.back().sourceNum & 1)  << 7));
    r = c;
    g = c << 3;
    b = c << 5;
}

// std::uint8_t makeGeneticColor(const Genome &genome) {
//     return ((genome.size() & 1)
//          | ((genome.front().sourceType)    << 1)
//          | ((genome.back().sourceType)     << 2)
//          | ((genome.front().sinkType)      << 3)
//          | ((genome.back().sinkType)       << 4)
//          | ((genome.front().sourceNum & 1) << 5)
//          | ((genome.front().sinkNum & 1)   << 6)
//          | ((genome.back().sourceNum & 1)  << 7));
// }


