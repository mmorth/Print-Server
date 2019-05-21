#!/bin/bash

cd cli-printer/
make clean
make
export LD_LIBRARY_PATH="../libprintserver/"

./cli-printer -l samplec.ps

./cli-printer -d black_white -s "Black and White Print Test" -o bw.pdf samplec.ps
./cli-printer -d color -s "Color Print Test" -o color.pdf samplec.ps

