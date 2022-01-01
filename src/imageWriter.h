#ifndef IMAGEWRITER_H_INCLUDED
#define IMAGEWRITER_H_INCLUDED

// Creates a graphic frame for each simStep, then
// assembles them into a video at the end of a generation.

#include "simulator.h"
#include <pthread.h>
#include "opencv2/core.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/imgproc.hpp"

struct ImageWriter {
    ImageWriter();
    void start_threads();
    void push_frame();
};

extern ImageWriter imageWriter;
extern uint32_t generation;
void destroy_threads();


#endif // IMAGEWRITER_H_INCLUDED
