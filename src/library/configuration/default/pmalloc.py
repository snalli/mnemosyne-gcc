########################################################################
#                                                                    
#         PERSISTENT MEMORY ALLOCATOR BUILD-TIME CONFIGURATION               
#                                                                    
########################################################################

########################################################################
# GENALLOC: Determines the generic persistent memory allocator
# 
# GENALLOC_DOUGLEA: persistent memory allocator based on Doug Lea's 
#   memory allocator (dlmalloc)
#   
# GENALLOC_VISTAHEAP: persistent memory allocator based on Vistaheap 
#
########################################################################

GENALLOC = 'GENALLOC_DOUGLEA'
