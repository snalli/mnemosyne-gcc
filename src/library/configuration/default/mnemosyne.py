########################################################################
#                                                                    
#                         MNEMOSYNE USER-MODE               
#                                                                    
########################################################################



########################################################################
# M_PCM_EMULATE_CRASH: PCM emulation layer emulates system crashes
########################################################################

M_PCM_EMULATE_CRASH = False

########################################################################
# M_PCM_EMULATE_LATENCY: PCM emulation layer emulates latency
########################################################################

M_PCM_EMULATE_LATENCY = False

########################################################################
# M_PCM_CPUFREQ: CPU frequency in GHz used by the PCM emulation layer to 
#   calculate latencies
########################################################################

M_PCM_CPUFREQ = 2500

########################################################################
# M_PCM_LATENCY_WRITE: Latency of a PCM write in nanoseconds. This 
#   latency is in addition to the DRAM latency.
########################################################################

M_PCM_LATENCY_WRITE = 150

########################################################################
# M_PCM_BANDWIDTH_MB: Bandwidth to PCM in MB/s. This is used to model 
# sequential writes.
########################################################################

M_PCM_BANDWIDTH_MB = 1200
