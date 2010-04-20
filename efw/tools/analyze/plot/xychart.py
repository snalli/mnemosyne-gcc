import sys, string, os, glob, re
import copy
import parser.sync
from util.stat import mean, confidence_interval_95_percent
from pylab import *

dashes = ['--', '-', '-.', ':', '-']

def plot_line(titlestr, xlab, ylab, data_set, output=None, interactive=False):
    vector_x = []
    vector_y = []
    vector_err_y = []
    figure(1)
    clf()
    if titlestr is not None:
        title(titlestr)
    xlabel(xlab)
    ylabel(ylab)
    for i, data_list in enumerate(data_set):
        data_label = data_list[0]
        data_vector = data_list[1]
        for data_point in data_vector:
            vector_x.append(data_point[0])
            vector_y.append(mean(data_point[1]))
            vector_err_y.append(confidence_interval_95_percent(data_point[1]))
        plot (vector_x, vector_y, label = data_label, linestyle = dashes[mod(i,len(dashes))])
        errorbar (vector_x, vector_y, vector_err_y, fmt='ro')
    legend()
    if output is not None:
        savefig(output)
    if interactive == True:    
        show()
