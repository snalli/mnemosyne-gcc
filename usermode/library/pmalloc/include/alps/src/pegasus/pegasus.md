[//]: # ( (c) Copyright 2016 Hewlett Packard Enterprise Development LP             )
[//]: # (                                                                          )
[//]: # ( Licensed under the Apache License, Version 2.0 (the "License");          )
[//]: # ( you may not use this file except in compliance with the License.         )
[//]: # ( You may obtain a copy of the License at                                  )
[//]: # (                                                                          )
[//]: # (     http://www.apache.org/licenses/LICENSE-2.0                           )
[//]: # (                                                                          )
[//]: # ( Unless required by applicable law or agreed to in writing, software      )
[//]: # ( distributed under the License is distributed on an "AS IS" BASIS,        )
[//]: # ( WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. )
[//]: # ( See the License for the specific language governing permissions and      )
[//]: # ( limitations under the License.                                           )


PEGASUS: Persistent Global Address Space for Universal Sharing       {#pegasuspage}
==============================================================

A PErsistent Global Address Space for Universal Sharing (PEGASUS) 
object (alps::AddressSpace) provides a shared address space between 
multiple worker processes.
The address space comprises multiple persistent memory regions, which are 
contiguous segments of the address space that are backed by fabric-attached 
memory (FAM). 
Each process has its own PEGASUS object instance that it can use to map 
and access shared persistent memory regions.
For emulation purposes, we also provide an implementation
of persistent memory regions on top of an in-memory file system (TMPFS).
 
Persistent regions (alps::Region) are instantiated by binding and 
mapping region files (alps::RegionFile) into the address space.
As each process may memory map the region file at a different location, 
each region type provides smart pointers (\ref SMARTPOINTERS) for 
referencing locations within the region in a manner that is independent 
of mapping address. 


