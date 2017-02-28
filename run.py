#!/usr/bin/env python

import argparse
import sys
import os
from subprocess import Popen, PIPE

sim_sizes = { 'vacation': {'test' : 'sudo ./build/bench/stamp-kozy/vacation/vacation -c4 -r400 -t200 -n1 -q80 -u99 -e1',
            'small' : 'sudo ./build/bench/stamp-kozy/vacation/vacation -c4 -r4000 -t2000 -n1 -q80 -u99 -e1',
            'medium' : 'sudo ./build/bench/stamp-kozy/vacation/vacation -c4 -r40000 -t20000 -n1 -q80 -u99 -e1',
            'large' : 'sudo ./build/bench/stamp-kozy/vacation/vacation -c4 -r4000000 -t2000000 -n1 -q80 -u99 -e1'},
            'memcached' : {'test' : ['sudo bash scripts/run_memcache.sh &', 'sleep 1', 'sudo bash scripts/run_memslap.sh', 'sudo pkill memcached']}
}

def runCmd(cmd, err, out, detached=False):
    """
    Takes two strings, command and error, runs it in the shell
    and then if error string is found in stdout, exits.
    For no output = no error, use err=""
    """
    print cmd
    if detached:
            Popen(cmd, shell=True, stdout=PIPE)
            return
    else:
            (stdout, stderr) = Popen(cmd, shell=True, stdout=PIPE).communicate()
    print stdout
    print stderr
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
    parser = argparse.ArgumentParser(description='Runs mnemosyne workloads from the whisper suite.')
    parser.add_argument('benchmarks', metavar='workload', type=str, nargs=1,
                help='workloads to be run: memcached/vacation')
    parser.add_argument('--sim_size', dest='sim_size', action='store', 
            default='test', help='Simulation size: test, small, medium, large')

    args = parser.parse_args()
    print 'Running a ' + str(args.sim_size) + ' simulation for ' + str(args.benchmarks)

    os.chdir('usermode')
    if args.sim_size in sim_sizes[args.benchmarks[0]]:
        simCmdLine = sim_sizes[args.benchmarks[0]][args.sim_size]
        if type(simCmdLine) is list:
            for simCmd in simCmdLine:
                if '&' in simCmd:
                    runCmd(simCmd, "Error", "Simulation failed", True)
                else:
                    runCmd(simCmd, "Error", "Simulation failed")
        else:
            runCmd(simCmdLine, "Error", "Simulation failed")
    else:
        print "Simulation size: " + str(args.sim_size) + " not found for workload:" + str(args.benchmarks[0])
    os.chdir('../')

if __name__ == "__main__":
    main(sys.argv[1:])
