#!/bin/bash
#memslap --cfg_cmd -s 127.0.0.1:11211 -T 1 -c 1 -x 1 -X 64
#memslap -s 127.0.0.1:11211 -T 1 -c 1 -x 10 -X 64
memslap -s 127.0.0.1:11211 -T 2 -c 2 -x 10 -X 64
#memslap -s 127.0.0.1:11211 -T 1 -c 1 -t 1s -X 64

