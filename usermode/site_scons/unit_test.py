import os, subprocess, string, re
from xml.etree.ElementTree import ElementTree
import xml

def runUnitTests(source, target, env):
    results = []
    for test in env['UNIT_TEST_CMDS']:
        osenv = os.environ 
        osenv.update(test[0])
        path = test[1]
        args = test[2]
        utest = subprocess.Popen([path] + args, shell=False,
                                 stdin=subprocess.PIPE,
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE, close_fds=True, env=osenv)
        ret = os.waitpid(utest.pid, 0)
        lines = utest.stderr.readlines()
        if re.search("xml", lines[0]):
            xmlout = string.join(lines)
            tree = xml.etree.ElementTree.XML(xmlout)
            test_list = []
            for child in tree:
                failure_list = []
                for failure in child:
                    failure_list.append(failure.attrib["message"])
                test_list.append((child.attrib["suite"], child.attrib["name"], child.attrib["time"], failure_list))
            results.extend(test_list)
        else:
            # Something really bad happen; fail immediately 
            print "FAILURE:", ret[1]
            for line in utest.stderr.readlines():
                print line,
            return 
    
    num_tests = 0
    num_failed_tests = 0
    num_failures = 0
    for test in results:
        num_tests = num_tests + 1
        if len(test[3]) > 0:
            num_failed_tests = num_failed_tests+1
            num_failures = num_failures+len(test[3])
            for failure in test[3]:
                print "ERROR: Failure in", test[0] + ':' + test[1], failure
    if num_failed_tests > 0:
        print "FAILURE:", num_failed_tests, 'out of', num_tests, 'tests failed (', num_failures, 'failures).'
    else:
        print "Success:", num_tests, 'tests passed.'
        open(str(target[0]),'w').write("PASSED\n")
        
