#!/usr/bin/env bash

rm /mnt/pcmfs/*
rm /tmp/segments/*
./driver_server 9330 9331 &
./driver_server 9330 9332 &
./driver_server 9330 9333 &
