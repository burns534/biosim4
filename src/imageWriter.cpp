#include "imageWriter.h"
#define N_FRAME_THREADS 8
#define N_VIDEO_THREADS 8
// frame thread declarations
// this could probably all be in the same array in a structure...
static pthread_mutex_t frame_wait_lock[N_FRAME_THREADS];
static pthread_cond_t frame_wait_cond[N_FRAME_THREADS];
static pthread_mutex_t frame_write_lock[N_FRAME_THREADS]; // mutex pool
static pthread_cond_t frame_write_cond[N_FRAME_THREADS];
static pthread_t frame_threads[N_FRAME_THREADS]; // thread pool
static std::vector<Indiv> render_cache[N_FRAME_THREADS]; // could possibly be optimized somehow. copying the entire individual is not necessary
static bool frame_thread_wait[N_FRAME_THREADS];
static bool frame_thread_no_write[N_FRAME_THREADS];
static uint8_t next_to_push = 0;
static std::vector<cv::Mat> frames; // frames vector for building the video for each generation
static uint8_t indices[N_FRAME_THREADS];
static uint8_t frame_queue_index = 0;
static bool frame_threads_should_exit = false;
// video thread declarations
static pthread_t video_threads[N_VIDEO_THREADS];
static uint8_t video_queue_index = 0;
static std::vector<cv::Mat> cached_frames[N_VIDEO_THREADS]; // holds the cached vector
static char filename[100];
static unsigned cached_generation;


// circular queue for thread pool
static inline uint8_t frame_thread_queue_front() {
    if (frame_queue_index < N_FRAME_THREADS) {
        return frame_queue_index++;
    } else {
        return frame_queue_index = 0;
    }
}

static inline uint8_t video_thread_queue_front() {
    if (video_queue_index < N_VIDEO_THREADS) {
        return video_queue_index++;
    } else {
        return video_queue_index = 0;
    }
}



static void * frame_write(void *arg) {
    using namespace cv;
    // get index from argument
    uint8_t index = *(uint8_t *)arg;
    while (1) {
        printf("ft %hhu I'm on standby\n", index);
        // attempt to lock frame write
        pthread_mutex_lock(&frame_wait_lock[index]);
        // now await signal
        while (frame_thread_wait[index])
            pthread_cond_wait(&frame_wait_cond[index], &frame_wait_lock[index]);
        printf("ft %hhu I'm processing the image\n", index);

        // create image frame
        Mat image = Mat(p.sizeX * p.displayScale, p.sizeY * p.displayScale, CV_8UC3, Scalar (255, 255, 255));

        // process image here
        for (uint16_t i = 1; i <= p.population; i++) {
            const Indiv &indiv = render_cache[index][i];
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
        printf("ft %hhu I'm done processing the image\n", index);
        // now attempt to write the frame
        // to determine this, it needs to check whose turn it is
        // and wait for its predecessor if it is not its turn
        if (next_to_push == index) {
            printf("ft %hhu It's my turn already!\n", index);
            frames.push_back(image);
        } else {
            printf("ft %hhu It's not my turn... I'l wait\n", index);
            // try to lock frame write mutex
            pthread_mutex_lock(&frame_write_lock[index]);
            while (frame_thread_no_write[index])
                pthread_cond_wait(&frame_write_cond[index], &frame_write_lock[index]);
            
            // now push the frame
            frames.push_back(image);
            printf("ft %hhu It's my turn now!\n", index);
            // unlock the frame_thread_no_write variable
            pthread_mutex_unlock(&frame_write_lock[index]);
        }
        
        // calculate index of successor
        uint8_t next = (next_to_push + 1) % N_FRAME_THREADS;

        // notify successor, who may or may not be waiting
        printf("ft %hhu I'm notifying my successor %hhu\n", index, next);
        // lock write predicate of successor
        pthread_mutex_lock(&frame_write_lock[next]);
        // update write predicate for successor
        frame_thread_no_write[next] = false;
        // update next in line to push
        next_to_push = next;
        // awaken successor if waiting
        pthread_cond_signal(&frame_write_cond[next]);
        // unlock predicate
        pthread_mutex_unlock(&frame_write_lock[next]);
        
        // update main predicate for this thread
        frame_thread_wait[index] = true;

        // unlock frame wait predicate
        pthread_mutex_unlock(&frame_wait_lock[index]);


        printf("ft %hhu I unlocked my main mutex. I'm done\n", index);
    }
}

void destroy_threads() {
    for (uint8_t i = 0; i < N_FRAME_THREADS; i++) {
        frame_threads_should_exit = true;
        pthread_join(frame_threads[i], NULL);
        pthread_mutex_destroy(&frame_write_lock[i]);
        pthread_mutex_destroy(&frame_wait_lock[i]);
        pthread_cond_destroy(&frame_write_cond[i]);
        pthread_cond_destroy(&frame_wait_cond[i]);
    }
}

// executes inside frame thread
// static void * save_frame_exec(void *arg) {
//     using namespace cv;
//     // get index from argument
//     uint16_t index = *(uint16_t *)arg;
//     std::vector<Indiv> *render_data = &render_cache[index];
//     while (1) {
       
//         // create image frame
//         Mat image = Mat(p.sizeX * p.displayScale, p.sizeY * p.displayScale, CV_8UC3, Scalar (255, 255, 255));

//         for (uint16_t index = 1; index <= p.population; index++) {
//             const Indiv &indiv = render_data->at(index);
//             if (indiv.alive) { // possibly optimize this as well
//                 // draw peep on the image
//                 circle(image, 
//                     Point(indiv.loc.x * p.displayScale, ((p.sizeY - indiv.loc.y) - 1) * p.displayScale),
//                     p.agentSize,
//                     Scalar(indiv.b, indiv.g, indiv.r), // bgr
//                     FILLED,
//                     LINE_8);
//             }
//         }
//         // write frame
//         frames.push_back(image);
      
//         if (simulation_ended) return NULL;
//     }
// }


static void * save_generation_video_exec(void *arg) {
    using namespace cv;
    // while (1) {
    //     // try to lock cached generation
    //     pthread_mutex_lock(&video_write_lock);
    //     // wait for signal to write video
    //     while (!should_write_video)
    //         pthread_cond_wait(&video_write_condition, &video_write_lock);
        
    //     clock_t tic = clock();

    //     if (cached_frames.empty()) {
    //         puts("error: ImageWriter::save_generation_video: no frames to write!");
    //         should_write_video = false;
    //         pthread_mutex_unlock(&video_write_lock);
    //         continue;
    //     }
    //     snprintf(filename, 100, "%s/gen-%06u.mp4", p.imageDir.c_str(), *(unsigned *)arg);
    //     // printf("saving %s\n", filename);
    //     unsigned width = cached_frames.front().rows, height = cached_frames.front().cols;
    //     VideoWriter vw;
    //     // 0x7634706d is codec for mp4
    //     // 20fps
    //     vw.open(filename, 0x7634706d, 20.0, Size(width, height));

    //     if (!vw.isOpened()) {
    //         puts("error: ImageWriter::save_generation_video: could not open cv::VideoWriter");
    //     }

    //     cv::setNumThreads(6);
        
    //     for (auto it = cached_frames.begin(); it != cached_frames.end(); it++) {
    //         vw.write(*it);
    //     }
    //     cached_frames.clear();

    //     should_write_video = false;
    //     // unlock video write lock and access to cached generation
    //     pthread_mutex_unlock(&video_write_lock);

    //     printf("took %fs to save %s\n", double(clock() - tic)/CLOCKS_PER_SEC, filename);
    //     if (simulation_ended) return NULL;
    // }
}

ImageWriter::ImageWriter() {
    for (uint8_t i = 0; i < N_FRAME_THREADS; i++) {
        indices[i] = i;
        // create thread and supply it with its own index
        pthread_create(&frame_threads[i], NULL, frame_write, indices + i);
        pthread_mutex_init(&frame_wait_lock[i], NULL);
        pthread_mutex_init(&frame_write_lock[i], NULL);
        pthread_cond_init(&frame_wait_cond[i], NULL);
        pthread_cond_init(&frame_write_cond[i], NULL);
        frame_thread_no_write[i] = true;
        frame_thread_wait[i] = true;
    }

    // pthread_create(&video_thread, NULL, save_generation_video_exec, &cached_generation);
}

void ImageWriter::push_frame() {
    // get front of queue
    uint8_t index = frame_thread_queue_front();
    // attempt to lock the mutex associated selected thread
    pthread_mutex_lock(&frame_wait_lock[index]);
    // once successful, cache the corresponding frame data
    render_cache[index] = peeps.get_individuals();
    // tell thread it is allowed to write frame
    frame_thread_wait[index] = false;
    // wake up the thread
    pthread_cond_signal(&frame_wait_cond[index]);
    // unlock frame wait lock
    pthread_mutex_unlock(&frame_wait_lock[index]);
    // return
}

// void ImageWriter::push_frame() {
//     // puts("waiting to push frame");
//     // attempt to lock cached individuals
//     pthread_mutex_lock(&frame_write_lock);
//     // cache individuals from the current frame
//     // from globally defined peeps instance
//     // puts("caching individuals");
//     cached_individuals = peeps.get_individuals();
//     should_write_frame = true;
//     // signal frame thread to write the frame
//     pthread_cond_signal(&frame_write_condition);
//     // unlock the cached individuals frame for use by thread
//     pthread_mutex_unlock(&frame_write_lock);
//     // puts("pushed frame successfully");
// }

#include <unistd.h>
// test synchronously first
void ImageWriter::save_generation_video(unsigned generation) {
    puts("im sleeping 2 seconds to let the frames finish");
    sleep(2);
    using namespace cv;
    clock_t tic = clock();

    if (frames.empty()) {
        puts("error: ImageWriter::save_generation_video: no frames to write!");
        return;
    }

    snprintf(filename, 100, "%s/gen-%06u.mp4", p.imageDir.c_str(), generation);
    // printf("saving %s\n", filename);
    unsigned width = frames.front().rows, height = frames.front().cols;
    VideoWriter vw;
    // 0x7634706d is codec for mp4
    // 20fps
    vw.open(filename, 0x7634706d, 20.0, Size(width, height));

    if (!vw.isOpened()) {
        puts("error: ImageWriter::save_generation_video: could not open cv::VideoWriter");
    }

    cv::setNumThreads(6);
    
    for (auto it = frames.begin(); it != frames.end(); it++) {
        vw.write(*it);
    }
    frames.clear();
    printf("took %fs to save %s\n", double(clock() - tic)/CLOCKS_PER_SEC, filename);





    //  // try to lock cached generation and cached_frames
    // pthread_mutex_lock(&video_write_lock);
    // // wait for permission to copy the frame given by the frame push execution
    // // this is required to prevent this function from locking the frame write
    // // mutex before the frame write thread is able to from the previous
    // // frame push call and then saving the next video with the first frame
    // // as the what should've been the last frame of the current video
    // while (frames.size() < p.stepsPerGeneration)
    //     pthread_cond_wait(&frame_copy_condition, &video_write_lock);
    // // lock frame to prevent changed while caching
    // pthread_mutex_lock(&frame_write_lock);
    // // update cached generation and cached_frames
    // cached_generation = generation;
    // cached_frames = frames;
    // // signal video writer to attempt the write
    // pthread_cond_signal(&video_write_condition);
    // should_write_video = true;
    // frames.clear(); // clear frames before continuing main loop
    // // unlock frames to allow program to proceed
    // pthread_mutex_unlock(&frame_write_lock);
    // // unlocked cached generation and cached frames
    // pthread_mutex_unlock(&video_write_lock);
    // // // unlock frame copy
    // // pthread_mutex_unlock(&frame_copy_lock);
    // // continue the simulation on main thread
}