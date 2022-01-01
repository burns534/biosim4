#!/usr/bin/env bash
g++ -std=c++11 -I/opt/homebrew/opt/opencv/include/opencv4 -L/opt/homebrew/opt/opencv/lib -lopencv_imgproc -lopencv_core -lopencv_videoio -pthread src/*.cpp -o bin/Debug/biosim4
