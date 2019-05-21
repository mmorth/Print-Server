#!/bin/bash

cd print-server/printer/
./setup-virt-printer.sh
./run-virt-printer.sh

cd ..

./setup-print-server.sh
./run-print-server.sh

