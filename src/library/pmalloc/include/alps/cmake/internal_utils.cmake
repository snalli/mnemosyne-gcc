# 
# (c) Copyright 2016 Hewlett Packard Enterprise Development LP
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

function(set_arch_conf target_arch_mem arch_libs_var arch_defs_var)
	string(TOUPPER ${target_arch_mem} target_arch_mem)
	if(${target_arch_mem} MATCHES "CC-NUMA")
		# no arch-specific configuration
	elseif(${target_arch_mem} MATCHES "NV-CC-NUMA")
		list(APPEND arch_libs pmem)
		list(APPEND arch_defs -D__ARCH_NON_VOLATILE__)
	elseif(${target_arch_mem} MATCHES "NV-CC-FAM")
		list(APPEND arch_libs pmem)
		list(APPEND arch_libs fam_atomic)
		list(APPEND arch_defs -D__ARCH_NON_VOLATILE__)
		list(APPEND arch_defs -D__ARCH_FAM__)
	elseif(${target_arch_mem} MATCHES "NV-NCC-FAM")
		list(APPEND arch_libs pmem)
		list(APPEND arch_libs fam_atomic)
		list(APPEND arch_defs -D__ARCH_NON_VOLATILE__)
		list(APPEND arch_defs -D__ARCH_NON_CACHE_COHERENT__)
		list(APPEND arch_defs -D__ARCH_FAM__)
	else()
		message(STATUS "ERROR: Unknown target memory architecture: " ${target_arch_mem})
		message(STATUS "ERROR: Known target memory architectures")
		message(STATUS "  - CC-NUMA     Cache-Coherent NUMA")
		message(STATUS "  - NV-CC-NUMA  Non-Volatile Cache-Coherent NUMA")
		message(STATUS "  - NV-CC-FAM   Non-Volatile Cache-Coherent Fabric-Attached Memory")
		message(STATUS "  - NV-NCC-FAM  Non-Volatile Non-Cache-Coherent Fabric-Attached Memory")
		message(FATAL_ERROR "Terminating build due to unknown target memory architecture")
	endif()

	set(${arch_libs_var} ${arch_libs} PARENT_SCOPE)
	set(${arch_defs_var} ${arch_defs} PARENT_SCOPE)

	message(STATUS "Configuring target memory architecture: " ${target_arch_mem} " libs=" "${arch_libs}" " defs=" "${arch_defs}")

endfunction()
