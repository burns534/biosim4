#!/usr/bin/env bash
if [ $2 == 'clear' ]
then
clear && printf '\e[3J'
fi
if [ $1 == 'debug' ]
then
echo 'Building Debug'
g++ -std=c++11 -I/opt/homebrew/opt/opencv/include/opencv4 -L/opt/homebrew/opt/opencv/lib -lopencv_imgproc -lopencv_core -lopencv_videoio -pthread src/*.cpp -o bin/Debug/biosim4
elif [ $1 == 'release' ]
then
echo 'Building Release (Optimized)'
g++ -O3 -std=c++11 -I/opt/homebrew/opt/opencv/include/opencv4 -L/opt/homebrew/opt/opencv/lib -lopencv_imgproc -lopencv_core -lopencv_videoio -pthread src/*.cpp -o bin/Debug/biosim4
fi