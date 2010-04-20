from __future__ import nested_scopes
import sys, string, os, glob, re, math
from types import *

# returns a list of lines in the file that matches the pattern
def grep(filename, pattern):
    result = [];
    file = open(filename,'r')
    for line in file.readlines():
        if re.match(pattern, line):
            result.append(string.strip(line))
    return result

# returns a list of lines in the file that DO NOT match the pattern
def inverse_grep(filename, pattern):
    result = [];
    file = open(filename,'r')
    for line in file.readlines():
        if not re.match(pattern, line):
            result.append(string.strip(line))
    return result


def get_int_stat(file, str):
    grep_lines = grep(file, str)
    if (grep_lines == []):
      return -1        
    line = string.split(grep_lines[0])
    return int(line[1])
    
def get_float_stat(file, stat):
    grep_lines = grep(file, stat)
    line = string.split(grep_lines[0])
    return float(line[1])
