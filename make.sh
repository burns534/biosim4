#!/usr/bin/env bash
g++ -std=c++11 $(pkg-config --cflags --libs opencv4) src/*.cpp -o bin/Debug/biosim4
