#!/bin/bash
# Reduce the number of relations 'r' if you don't have space
# This amounts to roughly 1.5 GB of reservation tables
sudo ./vacation -c0 -r4000000 -n1 -q100 -u100
