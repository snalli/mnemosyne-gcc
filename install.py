#!/usr/bin/env python

import argparse
import sys
import os
from subprocess import Popen, PIPE

workload = 'mnemosyne'

def runCmd(cmd, err, out):
    """
    Takes two strings, command and error, runs it in the shell
    and then if error string is found in stdout, exits.
    For no output = no error, use err=""
    """
    print cmd
    (stdout, stderr) = Popen(cmd, shell=True, stdout=PIPE).communicate()
    if err is None:
        if stdout != "":
            print "Error: %s" %(out,)
            print "Truncated stdout below:"
            print '... ', stdout[-500:]
            sys.exit(2)
    else:
        if err in stdout:
            print "Error: %s" %(out,)
            print "Truncated stdout below:"
            print '... ', stdout[-500:]
            sys.exit(2)

def main(argv): 
    """
    Parses the arguments and cleans and/or builds the specified
    workloads of the whisper suite
    """
    parser = argparse.ArgumentParser(description='Builds echo from the whisper suite.')
    parser.add_argument('--clean', dest='clean', action='store_true',
                default='false',
                help='clean')
    parser.add_argument('--build', dest='build', action='store_true',
                default='false',
                help='build')

    args = parser.parse_args()
   
    os.chdir("usermode")
    if args.clean == True:
        print "Cleaning " + workload
        cleanCmd = "rm -rf build/"
        runCmd(cleanCmd, "Error", "Couldn't clean %s dir!" % (workload, ))

    if args.build == True:
        print "Building " + workload
        buildCmd = "scons --build-bench=stamp-kozy"
        runCmd(buildCmd, "Error", "Couldn't build %s" % (workload,))
        buildCmd = "scons --build-bench=memcached"
        runCmd(buildCmd, "Error", "Couldn't build %s" % (workload,))
    os.chdir("../")

if __name__ == "__main__":
    main(sys.argv[1:])
