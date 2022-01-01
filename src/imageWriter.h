#ifndef IMAGEWRITER_H_INCLUDED
#define IMAGEWRITER_H_INCLUDED

// Creates a graphic frame for each simStep, then
// assembles them into a video at the end of a generation.

#include "simulator.h"
#include <pthread.h>
#include "opencv2/core.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/imgproc.hpp"


// This holds all data needed to construct one image frame. The data is
// cached in this structure so that the image writer can work on it in
// a separate thread while the main thread starts a new simstep.
struct IndivData {
    Coord loc;
    uint8_t r, g, b;
    IndivData(Coord loc, uint8_t r, uint8_t g, uint8_t b) {
        this->loc = loc;
        this->r = r;
        this->g = g;
        this->b = b;
    }
};

struct ImageFrameData {
    unsigned simStep, generation;
    std::vector<IndivData> indivData;
    std::vector<Coord> barrierLocs;
    typedef std::vector<std::vector<uint8_t>> SignalLayer;  // [x][y]
    std::vector<SignalLayer> signalLayers; // [layer][x][y]
};


// struct ImageWriter {
//     ImageWriter();
//     void startNewGeneration();
//     // bool saveVideoFrame(unsigned simStep, unsigned generation);
//     bool saveVideoFrameSync(unsigned simStep, unsigned generation);
//     void saveGenerationVideo(unsigned generation);
//     void saveFrameThread(); // runs in a thread
//     void save_video(std::string filename, std::vector<cv::Mat> *images);
//     std::atomic<unsigned> droppedFrameCount;
// private:
//     std::atomic<bool> busy;
//     std::mutex mutex_;
//     std::condition_variable condVar;
//     bool dataReady;

//     ImageFrameData data;
//     bool abortRequested;
//     unsigned skippedFrames;
// };

struct ImageWriter {
    ImageWriter();
    void push_frame();
    void save_generation_video(unsigned);
};

extern ImageWriter imageWriter;
void destroy_threads();


#endif // IMAGEWRITER_H_INCLUDED
