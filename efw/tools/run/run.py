#!/usr/bin/python

import getopt, signal, os, sys, shutil
import subprocess
import string
import datetime, time
from logger import *

generation_id = 0

default_config = {}
default_config['NAME'] = ''
default_config['CMD'] = ''
default_config['RESULTDIR'] = ''
default_config['WORKDIR'] = ''
default_config['LOG'] = ''
default_config['TIMEOUT'] = 120
default_config['ITERATIONS'] = 5
default_config['MPSTAT'] = False
default_config['VMSTAT'] = False
default_config['OPROFILE'] = False
default_config['RUN'] = False
default_config['MCORE_INI'] = ''
default_config['MTM_INI'] = ''
default_config['CONFIG_ARGS'] = []
default_config['ENV'] = {}
default_config['PREPROCESS_SCRIPT'] = ''
default_config['RECOVERY_SCRIPT'] = ''
default_config['STDIN'] = ''
default_config['MPSTAT_PATH'] = '/usr/bin/mpstat'
default_config['VMSTAT_PATH'] = '/usr/bin/vmstat'


statistics_sources = {}
statistics_sources['mtm']= ['%mtm_statistics_begin', '%mtm_statistics_end', 'mtm.stats']
statistics_sources['mcore']= ['%mcore_statistics_begin', '%mcore_statistics_end', 'mcore.stats']
statistics_sources['mpstat']= ['%mpstat_begin', '%mpstat_end']
statistics_sources['vmstat']= ['%vmstat_begin', '%vmstat_end']
statistics_sources['oprofile']= ['%oprofile_begin', '%oprofile_end']


initialization_sources = ['itm.ini', 'txc.ini']

def createConfig(local_config, global_config):
    config = {}
    for i in global_config:
        if i not in local_config:
            config[i] = global_config[i]
        else:
            config[i] = local_config[i]
    return config


def alarmHandler(signum, frame):
    pass


class Experiment:
    def __init__(self, local_config, global_config):
        experiment_config = createConfig(local_config, global_config)
        for el in experiment_config:
            self.__dict__[el.lower()] = experiment_config[el]
        self.generation_id = generation_id
        self.logger = Logger(self.log, self.name)
        self.resultdir = os.path.join(self.resultdir, self.name)
        try:
            os.makedirs(self.resultdir)
        except OSError, (errno, strerror):
            if errno == 17:
                # Directory already exists -- just ignore error
                pass
            else:
                print "OS error(%s): %s" % (errno, strerror)
                raise

    def setup_ini_files(self):
        # remove any previous initialization files
        for file in initialization_sources:
            try:
                dstname = os.path.join(self.workdir, file)
                os.remove(dstname)
            except OSError:
                pass
        # now copy any new initialization files if instructed
        if self.mtm_ini != '':
            try:
                srcname = self.mtm_ini
                dstname = os.path.join(self.workdir, 'itm.ini')
                shutil.copyfile(srcname, dstname)
            except IOError:
                self.logger.write("No such file %s" % srcname)
        if self.mcore_ini != '':
            try:
                srcname = self.mcore_ini
                dstname = os.path.join(self.workdir, 'txc.ini')
                shutil.copyfile(srcname, dstname)
            except IOError:
                self.logger.write("No such file %s" % srcname)

    def start(self):
        if self.run == False:
            return
        self.logger.open()
        self.setup_ini_files()
        for config in self.config_args:
            for i in range(self.iterations):
                run_recovery_script = False
                argstr = self.cmd + ' ' + config
                generation = '-%05d-%02d' % (self.generation_id, i)
                instance_name = self.name + '_' +  config.replace(" ", "_") + generation
                result_file_prefix = os.path.join(self.resultdir, instance_name)
                result_file_error = result_file_prefix + '.error'                    
                result_file_output = result_file_prefix + '.output'                    
                result_file_stats = result_file_prefix + '.stats'                    
                for stat_source_key in statistics_sources:
                    stat_source_val = statistics_sources[stat_source_key]
                    if stat_source_key == 'mtm' or stat_source_key == 'mcore':
                        try:
                            dstname = os.path.join(self.workdir, stat_source_val[2])
                            os.remove(dstname)
                        except OSError:
                            pass
                if self.oprofile == True:
                    sys.stderr.write("OProfile: Starting...\n")
                    os.system("sudo opcontrol --reset") 
                    os.system("sudo opcontrol --start") 
                    sys.stderr.write("OProfile: Started\n")
                if self.mpstat == True:    
                    mpstat = subprocess.Popen([self.mpstat_path, '1'], shell=False,
                                              stdin=subprocess.PIPE,
                                              stdout=subprocess.PIPE,
                                              stderr=subprocess.PIPE, close_fds=True)
                if self.vmstat == True:    
                    vmstat = subprocess.Popen([self.vmstat_path, '1'], shell=False,
                                              stdin=subprocess.PIPE,
                                              stdout=subprocess.PIPE,
                                              stderr=subprocess.PIPE, close_fds=True)
                if self.preprocess_script != '':    
                    pps = subprocess.Popen([self.preprocess_script],      \
                                           shell=True, cwd=self.workdir,  \
                                           stdin=subprocess.PIPE,         \
                                           stdout=subprocess.PIPE,        \
                                           stderr=subprocess.PIPE,        \
                                           close_fds=True, env=self.env)
                    os.waitpid(pps.pid, 0)

                signal.alarm(self.timeout)
                if self.stdin == '':
                    pstdin = subprocess.PIPE
                else:
                    pstdin = open(self.stdin, 'r')
                p = subprocess.Popen(argstr.split(" "), shell=False,        \
                      cwd=self.workdir,                                     \
                      stdin=pstdin, stdout=subprocess.PIPE,                 \
                      stderr=subprocess.PIPE, close_fds=True, env=self.env)
                try:
                    ret = os.waitpid(p.pid, 0)
                    signal.alarm(0)
                except OSError:
                    os.system("kill -9 %d" % p.pid)
                    ret = [0, 9]
                if self.mpstat == True:    
                    os.system("kill -9 %d" % mpstat.pid)
                if self.vmstat == True:    
                    os.system("kill -9 %d" % vmstat.pid)
                if self.oprofile == True:
                    sys.stderr.write("OProfile: Stopping...\n")
                    os.system("sudo opcontrol --stop") 
                    sys.stderr.write("OProfile: Stopped\n")
                if ret[1] == 9:
                    self.logger.write2(instance_name, "TIMEOUT (PID: %d)" % p.pid, '/')
                    run_recovery_script = True
                else:
                    if self.oprofile == True:
                        tgidstr = "tgid:%d" % p.pid
                        opreport_ps = subprocess.Popen(['opreport', '--symbols', '--merge=tgid', tgidstr], shell=False,
                                           stdin=subprocess.PIPE,
                                           stdout=subprocess.PIPE,
                                           stderr=subprocess.PIPE, close_fds=True)
                    fout = open(result_file_output, 'w')
                    ferr = open(result_file_error, 'w')
                    fstats = open(result_file_stats, 'w')
                    ferr.writelines(p.stderr.readlines())
                    fout.writelines(p.stdout.readlines())
                    for stat_source_key in statistics_sources:
                        stat_source_val = statistics_sources[stat_source_key]
                        fstats.write("%s\n" % stat_source_val[0])
                        if stat_source_key == 'mtm' or stat_source_key == 'mcore':
                            try:
                                srcname = os.path.join(self.workdir,
                                        stat_source_val[2])
                                fin = open(srcname, 'r')
                                src_lines = fin.readlines()
                                fstats.writelines(src_lines)
                                fin.close()
                            except IOError, (errno, strerror):
                                if errno == 2:   # error is because stats file does not exist, just ignore the error
                                    continue
                                print "OS error(%s): %s" % (errno, strerror)
                                print "srcname: %s" % srcname
                        elif stat_source_key == 'mpstat':
                            if self.mpstat == True:
                                fstats.writelines(mpstat.stdout.readlines())
                        elif stat_source_key == 'vmstat':
                            if self.vmstat == True:
                                fstats.writelines(vmstat.stdout.readlines())
                        elif stat_source_key == 'oprofile':
                            if self.oprofile == True:
                                fstats.writelines(opreport_ps.stdout.readlines())
                        fstats.write("\n%s\n\n\n" % stat_source_val[1])
                    self.logger.write2(instance_name, "DONE (PID: %d, RET: %d)" % (p.pid, ret[1]), '/')
                    if ret[1] != 0:
                        run_recovery_script = True
                    fout.close()
                    ferr.close()
                    fstats.close()
                if self.recovery_script != '' and run_recovery_script == True:    
                    recover_ps = subprocess.Popen([self.recovery_script],        \
                        shell=True, cwd=self.workdir,                       \
                      stdin=subprocess.PIPE, stdout=subprocess.PIPE,        \
                      stderr=subprocess.PIPE, close_fds=True, env=self.env)
                    os.waitpid(recover_ps.pid, 0)
                self.logger.flush()
        self.logger.close()

def usage():
    sys.stderr.write("Usage: %s <flags>  config_file\n" % 'run.py')
    sys.stderr.write("Optional flags:\n")
    sys.stderr.write("    -g, --generation    Generation number to tag experiments with (default = 0)\n")

def main(argv):
    global generation_id

    try:
        opts, args = getopt.getopt(argv, "g:", ["generation"])
        if len(args) < 1:
            raise Exception
    except:
        usage()
        sys.exit(2)
    for opt, arg in opts:
        if opt in ("-g", "--generation"):
            generation_id = string.atoi(arg)
            
    config_filename = args[0]
    config_dict = {}	
    execfile(config_filename, {}, config_dict)
    global_config = createConfig(config_dict, default_config)
    if 'EXPERIMENTS' in config_dict:
        for experiment_config in config_dict['EXPERIMENTS']:
            experiment = Experiment(experiment_config, global_config)
            experiment.start() 


if __name__ == "__main__":
    signal.signal(signal.SIGALRM, alarmHandler)
    main(sys.argv[1:])
