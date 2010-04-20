#!/s/std/bin/python

import sys, string, os, glob, re
import copy
import parser.sync
from util.stat import mean, confidence_interval_95_percent
from pylab import *
from plot.xychart import plot_line

def plot_sync():
    result_dir = '/scratch/workspace/results'

    data_set = []
    data_list = []
    for n in range(0,17):
        data = parser.sync.get_data(result_dir, 'sync', 'pcmdisk', 'msync', nwrites=n, wsize=8)
        if data is not None:
            for i, datum in enumerate(data):
                data[i] = n* data[i]
            data_list.append((n, data*n))
    name = 'msync_pcmdisk, wsize=8'
    data_set.append((name, data_list))
    plot_line(None, 'ops', 'throughput', data_set, interactive=True)



plot_sync()    
