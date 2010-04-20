import sys, datetime, time

class Logger:
    def __init__(self, logfile, prefix):
        self.prefix = prefix
        self.logfile = logfile
        self.flog = sys.stderr
    def open(self):
        if self.logfile != '':
            try:
                self.flog = open(self.logfile, 'a')
            except IOError:
                pass
    def write(self, msg):
        t = datetime.datetime.now()
        now = datetime.datetime.fromtimestamp(time.mktime(t.timetuple()))
        self.flog.write("%s # %s : %s\n" % (now.ctime(), self.prefix, msg))
    def write2(self, prefix, msg, sep=' ; '):
        t = datetime.datetime.now()
        now = datetime.datetime.fromtimestamp(time.mktime(t.timetuple()))
        self.flog.write("%s # %s%s%s : %s\n" % (now.ctime(), self.prefix, sep, prefix, msg))
    def flush(self):
        self.flog.flush()
    def close(self):
        if self.flog != sys.stderr:
            self.flog.close()
            self.flog = sys.stderr
