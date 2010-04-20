from __future__ import nested_scopes
import sys, string, os, glob, re, math
from types import *

def mean(list):
    length = len(list)

    if length == 0:
        return 0
    else:
        total = 0.0
        for i in list:
            total += i
        return total/length

# returns the median of the list
def median(list):
    sorted = list
    sorted.sort()
    length = len(list)
        
    if length == 0:
        return 0
    elif length == 1:
        return list[0]
    elif length % 2 == 0:
        # even  
        return (list[length/2]+list[(length/2)-1])/2.0
    else:
        # odd
        return list[length/2]

# returns the Nth percentile element of the list
def nth_percentile(list, n):
    sorted = list
    sorted.sort()
    length = len(list)
        
    if length == 0:
        return 0
    elif length == 1:
        return list[0]
    else:
        return list[length*n/100]

# returns the maximum element of the list
def maximum(list):
    sorted = list
    sorted.sort()
    length = len(list)
        
    if length == 0:
        return 0
    elif length == 1:
        return list[0]
    else:
        return list[length-1]

# returns the minimum of the list
def minimum(list):
    sorted = list
    sorted.sort()
    length = len(list)

    if length == 0:
        return 0
    else:
        return list[0]

# returns the average of the list
def average(list):
    if (type(list) is TupleType) or (type(list) is ListType):
        sum = reduce(lambda x, y: x+y, list, 0);
        length = len(list)
        if length == 0:   
            return 0
        else:
            return 1.0*sum/length
    else:
        return list
    
# returns the average of the list without the two extremes
def averageWithoutExtremes(list):
    av = average(list)
    length = len(list)
    max = maximum(list)
    min = minimum(list)
    if length < 3:
        return av
    else:
        return ((av*length-min-max)/(length-2))

# returns the standard deviation
def stddev(list):
    if len(list) == 1:
        print "Warning: standard deviation of a one element list"
        return 0
    
    if len(list) == 0:
        print "Warning: standard deviation of a zero element list"

    sum = 0.0;
    sumsq = 0.0;
    for num in list:
        sum = sum + num
        sumsq = sumsq + pow(num, 2)

    size = float(len(list))
    average = sum / size
    variance = (sumsq - pow(sum, 2)/size)/(size-1.0)

    if variance < 0.0:
        print "Warning: negative variance"
        variance = 0.0
        
    stddev = math.sqrt(variance)
    return stddev

#normalizes an associative array
def normalize(assoc_array, normal_key):
    if not assoc_array.has_key(normal_key):
        raise RuntimeError, "Normalization key missing"

    base = assoc_array[normal_key]
    if (type(base) is TupleType) or (type(base) is ListType):
        base = base[0]

    if base == 0:
        raise RuntimeError, "Normalizing with zero base"
    for key in assoc_array.keys():
        item = assoc_array[key]

        # act differently if we are a list or not
        if (type(item) is TupleType) or (type(item) is ListType):
            for index in range(len(item)):
                item[index] = item[index]/base
        else:
            item = item/base

        assoc_array[key] = item

def normalize_list(lines, config):
    normal_map = {}
    for line in lines:
        if line[0] == config:
            line = line[1:]  # skip label
            for pair in line:
                normal_map[pair[0]] = pair[1]

    counter = 0
    for line in lines:
        new_line = [line[0]]
        line = line[1:]  # strip off label
        for pair in line:
            x_value = pair[0]
            new_pair = [x_value]
            if normal_map.has_key(x_value):
                for index in range(1, len(pair)):
                    new_pair.append(normal_map[x_value]/pair[index])
                new_line.append(new_pair)
        lines[counter] = new_line
        counter += 1



# find the minimum value and inverted normalize to it
def make_relative(lines):
    lst = []
    for line in lines:
        line = line[1:]  # skip label
        for pair in line:
            lst.append(pair[1]) # the y-value

    minimum = min(lst)

    for line in lines:
        line = line[1:]  # strip off label
        for pair in line:
            for index in range(1, len(pair)):
                pair[index] = minimum/pair[index]

##############################
# Code to calculate exponential regressions using a transformation and
# a linear regression

def exp_regress(X, Y):
    # map into logarithmic space
    Y_prime = map(math.log, Y)

    # perform a linear regression in log space
    a, b = linreg(X, Y_prime)

    # Calculate the rate of growth. # The continuously compounding
    # rate equation for r:
    #       F = P*e^(Tr)   --->   r = ln(F/P) / T
    # where F is the final value, P is the starting value, T is time,
    # and r is the rate

    # Note: a, the slope of the fit, is the rate of growth of the
    # exponential curve.  This is only true because we're using the
    # natural logarithm as the base of our transformation

    rate = a
    
    # calculate the smooth line in log space
    Y_fit_prime = map(lambda x:(a*x)+b, X)

    # translate the log space back into original coordinates
    Y_fit = map(lambda x:math.pow(math.e, x), Y_fit_prime)
    
    return Y_fit, rate

#### Function for calculating confidence intervals
def t_distribution(v):
    if v > len(t_distribution_95_percent_lookup_table):
        return 1.96 # this is the value in the limit
    else:
        return t_distribution_95_percent_lookup_table[v-1]

def confidence_interval_95_percent(lst):
    n = len(lst)
    sd = stddev(lst) # standard deviation
    se = sd / math.sqrt(n) # standard error
    confidence_interval_95p = se * t_distribution(n-1)
    return confidence_interval_95p

# Note: for n < 5, the confidence interval is actually larger than the
# standard deviation.  At about n=6 is where the two are about the
# same, and around n=18 is where the 95% confidence interval is about
# half the standard deviation.  At n=60 the 95% confidence interval is
# about 1/4th the standard deviation.  The above data can be found by
# using the following code:

#for n in range(2, 100):
#    sd = 1
#    se = sd / math.sqrt(n) # standard error
#    confidence_interval_95p = se * t_distribution(n-1)
#    print n, confidence_interval_95p


# T-distribution table used in calculating 95% confidence intervals.
# The alpha for the table is 0.025 which corresponds to a 95%
# confidence interval.  (Note: a C-language stats package was used to
# generate this table, but it can also be calculated using Microsoft
# Excel's TINV() function.)

t_distribution_95_percent_lookup_table = (
    12.7062,
    4.30265,
    3.18245,
    2.77645,
    2.57058,
    2.44691,
    2.36462,
    2.306,
    2.26216,
    2.22814,
    2.20099,
    2.17881,
    2.16037,
    2.14479,
    2.13145,
    2.11991,
    2.10982,
    2.10092,
    2.09302,
    2.08596,
    2.07961,
    2.07387,
    2.06866,
    2.0639,
    2.05954,
    2.05553,
    2.05183,
    2.04841,
    2.04523,
    2.04227,
    2.03951,
    2.03693,
    2.03452,
    2.03224,
    2.03011,
    2.02809,
    2.02619,
    2.02439,
    2.02269,
    2.02108,
    2.01954,
    2.01808,
    2.01669,
    2.01537,
    2.0141,
    2.0129,
    2.01174,
    2.01063,
    2.00958,
    2.00856,
    2.00758,
    2.00665,
    2.00575,
    2.00488,
    2.00404,
    2.00324,
    2.00247,
    2.00172,
    2.001,
    2.0003,
    1.99962,
    1.99897,
    1.99834,
    1.99773,
    1.99714,
    1.99656,
    1.99601,
    1.99547,
    1.99495,
    1.99444,
    1.99394,
    1.99346,
    1.993,
    1.99254,
    1.9921,
    1.99167,
    1.99125,
    1.99085,
    1.99045,
    1.99006,
    1.98969,
    1.98932,
    1.98896,
    1.98861,
    1.98827,
    1.98793,
    1.98761,
    1.98729,
    1.98698,
    1.98667,
    1.98638,
    1.98609,
    1.9858,
    1.98552,
    1.98525,
    1.98498,
    1.98472,
    1.98447,
    1.98422,
    )

def make_eps(input_str, base_filename, eps_dir):
    filename = "%s/%s" % (eps_dir, base_filename)
    print "making eps file: %s" % filename
    jgr_file = open("%s.jgr" % filename, "w")
    jgr_file.write(input_str)
    jgr_file.close()

    # generate .eps (ghostview-able)
    (in_file, out_file) = os.popen2("jgraph")
    in_file.write(input_str)
    in_file.close()

    eps_file = open("%s.eps" % filename, "w")
    eps_file.writelines(out_file.readlines())
    eps_file.close()

def cdf(count, input):
    data = []
    total = 0.0
    for tuple in input:
        total += float(tuple[1])/float(count)
        data.append([tuple[0], total])

    return data

def pdf(count, input):
    data = []
    for tuple in input:
        p = float(tuple[1])/float(count)
        data.append([tuple[0], p])

    return data

############################################
# Merge Data:
#
# This is a utility to merge a list of
# data points from several runs into a
# single graph.  Points are collected into
# bins by their x value.  The resulting
# set has tuples with 4 entries, the x
# value, the average, and the average plus
# and minus 1 std dev.
############################################

def merge_data(group_title, data):
    
    map = {}
    for tuple in data:
        key = tuple[0]
        if map.has_key(key):
            map[key].append(tuple[1])
        else:
            map[key] = []
            map[key].append(tuple[1])

    points = []
    for key in map.keys():
        avg = average(map[key])
        dev = stddev(map[key])
        points.append([key, avg, avg+dev, avg-dev])
    points.sort()
    graph_lines = [group_title]
    graph_lines += points
    return graph_lines

############################################
# Excel Line Graphs
#
# This function generates a text file
# that can be imported, and (through a
# somewhat ugly macro) easily plotted
# as a line graph.
############################################

excel_dir = "excel_files"

def make_excel_line(name, data):
    filename = "%s/%s.txt" % (excel_dir, name)
    print "making excel text file: %s" % filename
    excel_file = open(filename, "w")

    for set in data:
        is_first = 1
        for point in set:
            if is_first == 1:
                excel_file.write(set[0])
                is_first = 0
                continue
            else:
                excel_file.write("\t")
                                
            print point
            x_val = point[0]
            avg = point[1]
            # tuples are expected to be formed
            # with "mfgraph.merge_data" and in the
            # form [x, avg, avg+stdd, avg-stdd]
            if len(point) >= 3:
                std_dev = point[2] - point[1]
                excel_file.write("%f\t%f\t%f\n" % (x_val, avg, std_dev))
            else:
                excel_file.write("%f\t%f\n" % (x_val, avg))

            #excel_file.write("%f\t%f\n" % (x_val, avg))
            #excel_file.write("\t\t%f\n" % (std_dev))

    excel_file.close()

############################################
# Excel Bar Graphs
#
# This function generates a text file
# that can be imported, and (hopefully)
# easily plotted as a stacked bar graph.
############################################

def make_excel_stacked_bar(name, fields, data):
    filename = "%s/%s.txt" % (excel_dir, name)
    print "making excel text file: %s" % filename
    excel_file = open(filename, "w")

    fields.reverse()
    excel_file.write("\t")
    for f in fields:
        excel_file.write("\t%s" % f)
    excel_file.write("\n")

    for set in data:
        is_first = 1
       
        for tuple in set:
            if is_first == 1:
                excel_file.write(tuple) # name of the set
                is_first = 0
                continue
            else:
                excel_file.write("\t")

            excel_file.write("%f" % tuple[0])
            values = tuple[1:]
            values.reverse()
            for value in values:
                excel_file.write("\t%f" % (value))

            excel_file.write("\n")

    excel_file.close()
    
############################################
# Excel Bar Graphs
#
# This function generates a text file
# that can be imported, and (hopefully)
# easily plotted as a stacked bar graph.
############################################

def make_excel_bar(name, data):
    filename = "%s/%s.txt" % (excel_dir, name)
    print "making excel text file: %s" % filename
    excel_file = open(filename, "w")

    for set in data:
        is_first = 1
        for tuple in set:
            if is_first == 1:
                
                excel_file.write(tuple) # name of the set
                is_first = 0
                #continue
            else:
                print "tuple:"
                print tuple
                excel_file.write("\t")
                excel_file.write(tuple[0]) # name of the set
                if len(tuple) == 2:
                    values = tuple[1]
                else:
                    values = tuple[1:]
                
                for value in values:
                    excel_file.write("\t%f" % (value))
                excel_file.write("\n")

    excel_file.close()
