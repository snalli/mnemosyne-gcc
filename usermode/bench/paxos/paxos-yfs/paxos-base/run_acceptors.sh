#!/usr/bin/env bash

rm /mnt/pcmfs/*
#taskset -c 2 ./driver_server 9330 9331 &
#taskset -c 3 ./driver_server 9330 9332 &
./driver_server 9330 9331 &
./driver_server 9330 9332 &
#./driver_server 9330 9333 &
