#!/bin/bash
ls ../libprintserver
make clean
make
export LD_LIBRARY_PATH="../libprintserver/"

