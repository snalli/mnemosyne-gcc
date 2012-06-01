import re

def add(x,y): return x+y

cacheline_size = 64
objsizes = [64, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288, 1048576]


def parse(filename):
    table_throughput = {}
    table_latency = {}

    f = open(filename, 'r')
    for l in f.readlines():
        m = re.search('T(.): objsize = (\d*), nops = (\d*), duration = (\d*) cycles, duration/op = (\d*) cycles, throughput = ([\d|\.]*)', l)
        if m:
            tid = m.group(1)
            objsize = int(m.group(2))
            nops = m.group(3)
            latency = m.group(4)
            oplatency = int(m.group(5))
            throughput = float(m.group(6))
            
            if not objsize in table_throughput:
                table_throughput[objsize] = []
                table_latency[objsize] = []
            table_throughput[objsize].append(throughput*objsize/64)
            table_latency[objsize].append(oplatency)
    return (table_latency, table_throughput)

latency = {}
throughput = {}

#latency[1], throughput[1] = parse('clflush-1T.dat')
#latency[2], throughput[2] = parse('clflush-2T.dat')
#latency[4], throughput[4] = parse('clflush-4T.dat')

latency[1], throughput[1] = parse('ntflush-1T.dat')
latency[2], throughput[2] = parse('ntflush-2T.dat')
latency[4], throughput[4] = parse('ntflush-4T.dat')


print 'THROUGHPUT: Cacheline flushes/s '
for s in objsizes:
    print s,
    for t in [1,2,4]:
        print reduce(add, throughput[t][s]),
    print

print 'LATENCY: ns '
for s in objsizes:
    print s,
    for t in [1,2,4]:
        print reduce(add, latency[t][s])/len(latency[t][s])/2.5/(s/cacheline_size),
    print
