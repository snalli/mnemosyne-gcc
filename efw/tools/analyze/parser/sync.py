import sys, string, os, glob, re
import copy
from util.grepstat import get_int_stat


def get_data(results_dir, basename, system_name, ubench_name, nwrites, wsize, generation_id = 0):
    generation = '%05d' % generation_id
    data = []
    #config_str = "--system=%s_--ubench=%s_--runtime=*_--nwrites=%d_--wsize=%d" % (system_name, ubench_name, nwrites, wsize)
    config_str = "--system=%s_--ubench=%s_--runtime=*_--nops=%d_--wsize=%d" % (system_name, ubench_name, nwrites, wsize)
    glob_str = "%s/%s/%s_%s-%s*.output" % (results_dir, basename, 
                    basename, config_str, generation)
    files = glob.glob(glob_str)
    data_points = []
    if files == []:
        return
    for file in files:
        ops = get_int_stat(file, 'total_its')
        time = get_int_stat(file, 'runtime')
        if ops > -1:
            data_points.append(ops/(float(time)/1000))
    return data_points            
