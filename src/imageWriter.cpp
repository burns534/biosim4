// imageWriter.cpp
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include "simulator.h"
#include "imageWriter.h"
#include "CImg.hpp"


cimg_library::CImgList<std::uint8_t> imageList;

// Pushes a new image frame onto .imageList.
//
void saveOneFrameImmed(const ImageFrameData &data)
{
    using namespace cimg_library;

    CImg<std::uint8_t> image(p.sizeX * p.displayScale, p.sizeY * p.displayScale,
                        1,   // Z depth
                        3,   // color channels
                        255);  // initial value
    std::uint8_t color[3];
    std::stringstream imageFilename;
    // modified this
    // imageFilename << p.imageDir << "/frames/frame-"
    //               << std::setfill('0') << std::setw(6) << data.generation
    //               << '-' << std::setfill('0') << std::setw(6) << data.simStep
    //               << ".bmp";

    // Draw barrier locations

    color[0] = color[1] = color[2] = 0x88;
    for (Coord loc : data.barrierLocs) {
            image.draw_rectangle(
                loc.x       * p.displayScale - (p.displayScale / 2), ((p.sizeY - loc.y) - 1)   * p.displayScale - (p.displayScale / 2),
                (loc.x + 1) * p.displayScale, ((p.sizeY - (loc.y - 0))) * p.displayScale,
                color,  // rgb
                1.0);  // alpha
    }

    // Draw agents

    constexpr std::uint8_t maxColorVal = 0xb0;
    constexpr std::uint8_t maxLumaVal = 0xb0;

    auto rgbToLuma = [](std::uint8_t r, std::uint8_t g, std::uint8_t b) { return (r+r+r+b+g+g+g+g) / 8; };

    for (size_t i = 0; i < data.indivLocs.size(); ++i) {
        int c = data.indivColors[i];
            color[0] = (c);                  // R: 0..255
            color[1] = ((c & 0x1f) << 3);    // G: 0..255
            color[2] = ((c & 7)    << 5);    // B: 0..255

            // Prevent color mappings to very bright colors (hard to see):
            if (rgbToLuma(color[0], color[1], color[2]) > maxLumaVal) {
                if (color[0] > maxColorVal) color[0] %= maxColorVal;
                if (color[1] > maxColorVal) color[1] %= maxColorVal;
                if (color[2] > maxColorVal) color[2] %= maxColorVal;
            }

        image.draw_circle(
                data.indivLocs[i].x * p.displayScale,
                ((p.sizeY - data.indivLocs[i].y) - 1) * p.displayScale,
                p.agentSize,
                color,  // rgb
                1.0);  // alpha
    }

    // image.save_bmp(imageFilename.str().c_str());
    imageList.push_back(image);

    //CImgDisplay local(image, "biosim3");
}


// Starts the image writer asynchronous thread.
ImageWriter::ImageWriter()
    : droppedFrameCount{0}, busy{true}, dataReady{false},
      abortRequested{false}
{
    startNewGeneration();
}


void ImageWriter::startNewGeneration()
{
    imageList.clear();
    skippedFrames = 0;
}


std::uint8_t makeGeneticColor(const Genome &genome)
{
    return ((genome.size() & 1)
         | ((genome.front().sourceType)    << 1)
         | ((genome.back().sourceType)     << 2)
         | ((genome.front().sinkType)      << 3)
         | ((genome.back().sinkType)       << 4)
         | ((genome.front().sourceNum & 1) << 5)
         | ((genome.front().sinkNum & 1)   << 6)
         | ((genome.back().sourceNum & 1)  << 7));
}

// Synchronous version, always returns true
bool ImageWriter::saveVideoFrameSync(unsigned simStep, unsigned generation)
{
    // We cache a local copy of data from params, grid, and peeps because
    // those objects will change by the main thread at the same time our
    // saveFrameThread() is using it to output a video frame.
    data.simStep = simStep;
    data.generation = generation;
    data.indivLocs.clear();
    data.indivColors.clear();
    data.barrierLocs.clear();
    data.signalLayers.clear();
    //todo!!!
    for (uint16_t index = 1; index <= p.population; ++index) {
        const Indiv &indiv = peeps[index];
        if (indiv.alive) {
            data.indivLocs.push_back(indiv.loc);
            data.indivColors.push_back(makeGeneticColor(indiv.genome));
        }
    }

    auto const &barrierLocs = grid.getBarrierLocations();
    for (Coord loc : barrierLocs) {
        data.barrierLocs.push_back(loc);
    }

    saveOneFrameImmed(data);
    return true;
}


// ToDo: put save_video() in its own thread
void ImageWriter::saveGenerationVideo(unsigned generation)
{
    if (imageList.size() > 0) {
        std::stringstream videoFilename;
        videoFilename << p.imageDir.c_str() << "/gen-"
                      << std::setfill('0') << std::setw(6) << generation
                      << ".mov"; // has to be .mov for quicktime player to work
        cv::setNumThreads(4);
        imageList.save_video(videoFilename.str().c_str(),
                             25,
                             "avc1");
        if (skippedFrames > 0) {
            std::cout << "Video skipped " << skippedFrames << " frames" << std::endl;
        }
    }
    startNewGeneration();
}

// Runs in a thread; wakes up when there's a video frame to generate.
// When this wakes up, local copies of Params and Peeps will have been
// cached for us to use.
void ImageWriter::saveFrameThread()
{
    busy = false; // we're ready for business
    std::cout << "Imagewriter thread started." << std::endl;

    while (true) {
        // wait for job on queue
        std::unique_lock<std::mutex> lck(mutex_);
        condVar.wait(lck, [&]{ return dataReady && busy; });
        // save frame
        dataReady = false;
        busy = false;

        if (abortRequested) {
            break;
        }

        // save image frame
        saveOneFrameImmed(imageWriter.data);

        //std::cout << "Image writer thread waiting..." << std::endl;
        //std::this_thread::sleep_for(std::chrono::seconds(2));

    }
    std::cout << "Image writer thread exiting." << std::endl;
}


