// endOfGeneration.cpp

#include <iostream>
#include <utility>
#include <cassert>
#include <cstdlib>
#include <algorithm>
#include "simulator.h"
#include "imageWriter.h"

// At the end of each generation, we save a video file (if p.saveVideo is true) and
// print some genomic statistics to stdout (if p.updateGraphLog is true).

void endOfGeneration(unsigned generation) {
    // if (p.saveVideo && (generation % p.videoStride == 0 || generation < p.videoSaveFirstFrames)) {
    //     imageWriter.save_generation_video(generation);
    // }

    if (p.updateGraphLog && (generation == 1 || ((generation % p.updateGraphLogStride) == 0))) {
#pragma GCC diagnostic ignored "-Wunused-result"
        std::system(p.graphLogUpdateCommand.c_str());
    }
    
}
