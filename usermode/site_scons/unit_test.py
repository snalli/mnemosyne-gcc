import os, subprocess

def runUnitTests(source, target, env):
    rv = 0
    for unit_test in env['UNIT_TEST_CMDS']:
        osenv = os.environ 
        osenv.update(unit_test[0])
        path = unit_test[1]
        args = unit_test[2]
        print path, args
        utest = subprocess.Popen([path] + args.split(" "), shell=False,
                                 stdin=subprocess.PIPE,
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE, close_fds=True, env=osenv)
        ret = os.waitpid(utest.pid, 0)
        for line in utest.stdout.readlines():
            print line,
        for line in utest.stderr.readlines():
            print line,
        if ret[1] != 0:
            rv = 1
    if rv == 0:
        open(str(target[0]),'w').write("PASSED\n")
        
