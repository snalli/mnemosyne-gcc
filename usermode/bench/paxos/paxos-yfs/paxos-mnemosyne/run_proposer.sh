#!/usr/bin/env bash

#./driver_server 9330 9330 9331 9332 9333
taskset -c 0,1 ./driver_server 9330 9330 9331 9332
#./driver_server 9330 9330 9331 9332
