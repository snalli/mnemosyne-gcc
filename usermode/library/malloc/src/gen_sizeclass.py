import sys

SUPERBLOCK_SIZE = int(sys.argv[1])

sizeTable = [8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120, 128, 136, 144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 256, 264, 272, 280, 288, 296, 304, 312, 320, 328, 336, 344, 352, 360, 368, 376, 384, 392, 400, 408, 416, 424, 432, 440, 448, 456, 464, 472, 480, 488, 496, 504, 512, 576, 640, 704, 768, 832, 896, 960, 1024, 1088, 1152, 1216, 1280, 1344, 1408, 1472, 1536, 1600, 1664, 1728, 1792, 1856, 1920, 1984, 2048, 2112, 2560, 3072, 3584, 4096, 4608, 5120, 5632, 6144, 6656, 7168, 7680, 8192, 8704, 9216, 9728, 10240, 10752, 12288, 16384, 20480, 24576, 28672, 32768, 36864, 40960, 65536, 98304, 131072, 163840, 262144, 524288, 1048576, 2097152, 4194304, 8388608, 16777216, 33554432, 67108864, 134217728, 268435456, 536870912, 1073741824, 2147483648]

# create the threshold table
threshold = []
for i in range(len(sizeTable)):
    t = SUPERBLOCK_SIZE/sizeTable[i]
    if t==0:
        threshold.append(1) 
    else:        
        threshold.append(t) 

# now print out the tables
sys.stdout.write("size_t hoardHeap::_sizeTable[hoardHeap::SIZE_CLASSES]\n")
sys.stdout.write("= {")
for i in range(len(sizeTable)):
    if i==0:
        sys.stdout.write("%dUL" % sizeTable[i])
    else:    
        sys.stdout.write(", %dUL" % sizeTable[i])
sys.stdout.write("};\n\n")

sys.stdout.write("size_t hoardHeap::_threshold[hoardHeap::SIZE_CLASSES]\n")
sys.stdout.write("= {")
for i in range(len(threshold)):
    if i==0:
        sys.stdout.write("%dUL" % threshold[i])
    else:    
        sys.stdout.write(", %dUL" % threshold[i])
sys.stdout.write("};\n\n")
