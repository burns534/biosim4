#include "imageWriter.h"
#define N_VIDEO_THREADS 8
// frame thread declarations
static pthread_mutex_t frame_wait_lock;
static pthread_cond_t frame_wait_cond;
static pthread_t frame_thread;
static bool frame_wait = true, frame_thread_should_exit = false;
static std::vector<Indiv> render_cache;
static uint32_t frame_cached_generation;
static std::vector<cv::Mat> global_frames; // frames vector for building the video for each generation

// video thread declarations
static bool video_wait[N_VIDEO_THREADS];
static pthread_cond_t video_wait_cond[N_VIDEO_THREADS];
static pthread_mutex_t video_wait_lock[N_VIDEO_THREADS];
static pthread_t video_threads[N_VIDEO_THREADS];
static uint8_t video_queue_index = 0;
static std::vector<cv::Mat> cached_frames[N_VIDEO_THREADS]; // holds the cached vector
static char filename[100];
static unsigned cached_generations[N_VIDEO_THREADS];
static bool video_threads_should_exit = false;
static uint8_t indices[N_VIDEO_THREADS];

// circular queue for video thread pool
static inline uint8_t video_thread_queue_front() {
    if (video_queue_index < N_VIDEO_THREADS) {
        return video_queue_index++;
    } else {
        video_queue_index = 0;
        return video_queue_index++;
    }
}

static void * frame_writer(void *arg) {
    using namespace cv;
    Scalar size = Scalar (255, 255, 255);
    int rows = p.displayScale * p.sizeX,
        cols = p.displayScale * p.sizeY;
    while (1) {
        // lock the predicate
        pthread_mutex_lock(&frame_wait_lock);
        // wait for work
        while (frame_wait && !frame_thread_should_exit) {
            pthread_cond_wait(&frame_wait_cond, &frame_wait_lock);
        }

        if (frame_thread_should_exit) {
            pthread_mutex_unlock(&frame_wait_lock);
            pthread_exit(NULL);
        }
// create image frame
        Mat image = Mat(rows, cols, CV_8UC3, size);

        // process image here
        for (uint16_t i = 1; i <= p.population; i++) {
            const Indiv &indiv = render_cache[i];
            if (indiv.alive) { // possibly optimize this as well
                // draw peep on the image
                circle(image, 
                    Point(indiv.loc.x * p.displayScale, ((p.sizeY - indiv.loc.y) - 1) * p.displayScale),
                    p.agentSize,
                    Scalar(indiv.b, indiv.g, indiv.r), // bgr
                    FILLED,
                    LINE_8);
            }
        }

        // push the frame
        global_frames.push_back(image);

        // if necessary, save the video

        if (p.saveVideo && (global_frames.size() == p.stepsPerGeneration) && ((frame_cached_generation < p.videoSaveFirstFrames) || (frame_cached_generation % p.videoStride == 0))) {
            // get available video writer thread
            uint8_t front = video_thread_queue_front();
            // attempt to lock its predicate variable
            pthread_mutex_lock(&video_wait_lock[front]);
            // cache generation
            cached_generations[front] = frame_cached_generation;
            // cache global_frames
            cached_frames[front] = global_frames;
            // update its predicate
            video_wait[front] = false;
            // signal to wake up
            pthread_cond_signal(&video_wait_cond[front]);
            // unlock predicate
            pthread_mutex_unlock(&video_wait_lock[front]);
            // clear global frames
            global_frames.clear();
        }

        // update predicate
        frame_wait = true;
        // unlock the predicate
        pthread_mutex_unlock(&frame_wait_lock);
    }
}

static void * video_write(void *arg) {
    using namespace cv;
    double fps = p.videoFPS;
    Size size = Size(p.displayScale * p.sizeX, p.displayScale * p.sizeY);
    uint8_t index = *(uint8_t *)arg;
    while (1) {
        // try to lock video wait predicate
        pthread_mutex_lock(&video_wait_lock[index]);
        // printf("v%hhu: video_write standby\n", index);
        // wait for signal to process video
        while (video_wait[index] && !video_threads_should_exit) {
            pthread_cond_wait(&video_wait_cond[index], &video_wait_lock[index]);
        }

        if (video_threads_should_exit) {
            // printf("v%hhu: video_write exiting exit 1\n", index);
            pthread_mutex_unlock(&video_wait_lock[index]);
            pthread_exit(NULL);
        }

        // process video
        // assign temporary for appropriate cached frames
        const std::vector<Mat> &frames = cached_frames[index];

        // printf("v%hhu video_write processing video with frame count %lu\n", index, frames.size());

        if (frames.empty()) {
            // this should be impossible
            printf("error: video_writer %hhu: no frames to write!", index);
            pthread_mutex_unlock(&video_wait_lock[index]);
            destroy_threads();
            pthread_exit(NULL);
        }

        snprintf(filename, 100, "%s/gen-%06u.mp4", p.imageDir.c_str(), cached_generations[index]);
    
        VideoWriter vw;
        // 0x7634706d is codec for mp4
        vw.open(filename, 0x7634706d, fps, size);

        if (!vw.isOpened()) {
            printf("error: video_writer %hhu: could not open cv::VideoWriter", index);
            pthread_mutex_unlock(&video_wait_lock[index]);
            vw.release(); // must call explicitly
            destroy_threads();
            pthread_exit(NULL);
        }

// TODO: change this to use ffmpeg with hardware acceleration
        for (auto it = frames.begin(); it != frames.end(); it++) {
            vw.write(*it);
        }

        // reset predicate
        video_wait[index] = true;

        // printf("v%hhu video_write video processing complete\n", index);

        if (video_threads_should_exit) {
            // printf("v%hhu: video_write exiting exit 2\n", index);
            pthread_mutex_unlock(&video_wait_lock[index]);
            // must call this explicitly before exiting the thread
            // or else the video will be corrupted
            vw.release();
            pthread_exit(NULL);
        }

        // unlock predicate
        pthread_mutex_unlock(&video_wait_lock[index]);
    }
}

ImageWriter::ImageWriter() {}

void ImageWriter::start_threads() {
    frame_wait = true;
    pthread_mutex_init(&frame_wait_lock, NULL);
    pthread_cond_init(&frame_wait_cond, NULL);
    pthread_create(&frame_thread, NULL, frame_writer, NULL);

// initialize video threads and their data
    for (uint8_t i = 0; i < N_VIDEO_THREADS; i++) {
        indices[i] = i;
        video_wait[i] = true;
        pthread_mutex_init(&video_wait_lock[i], NULL);
        pthread_cond_init(&video_wait_cond[i], NULL);
        pthread_create(&video_threads[i], NULL, video_write, indices + i);
    }
}

void ImageWriter::push_frame() {
    // attempt to lock the mutex associated selected thread
    pthread_mutex_lock(&frame_wait_lock);
    // once successful, cache the corresponding frame data
    render_cache = peeps.get_individuals();
    // printf("pushing frame %u and assigning generation %u to thread %hhu\n", ++frame_count, generation, index);
    // cache current generation
    frame_cached_generation = generation;
    // tell thread it is allowed to write frame
    frame_wait = false;
    // wake up the thread
    pthread_cond_signal(&frame_wait_cond);
    // unlock frame wait lock
    pthread_mutex_unlock(&frame_wait_lock);
    // return
}

void destroy_threads() {
    frame_thread_should_exit = true;
    pthread_cond_signal(&frame_wait_cond);
    pthread_join(frame_thread, NULL);
    pthread_cond_destroy(&frame_wait_cond);
    pthread_mutex_destroy(&frame_wait_lock);

    video_threads_should_exit = true;
    for (uint8_t i = 0; i < N_VIDEO_THREADS; i++) {
        pthread_cond_signal(&video_wait_cond[i]);
    }

    for (uint8_t i = 0; i < N_VIDEO_THREADS; i++) {
        pthread_join(video_threads[i], NULL);
        pthread_cond_destroy(&video_wait_cond[i]);
        pthread_mutex_destroy(&video_wait_lock[i]);
    }
}