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


ALPINiSM: Abstraction Layer for Programming Persistent Shared Memory   {#mainpage}
=====================================================================

ALPINiSM provides a low-level abstraction layer that reliefs the user 
from the details of mapping, addressing, and allocating persistent shared 
memory (also known as fabric-attached memory).
This layer can be used as a building block for building higher level 
abstractions and data structures such as heaps, logs, objects, etc.

![ALPINiSM layers](alps-layers.png) 
@image latex alps-layers.pdf "ALPINiSM layers" width = 10cm

Currently, we provide two layers (or API classes). 
First, a [PErsistent Global Address Space for Universal Sharing (PEGASUS)](@ref pegasuspage)
layer provides a shared address space between multiple worker processes.
Second, a [Global Symmetric Heap](@ref globalheap-page) layer for allocating
variable-size chunks of shared persistent memory (a.k.a. fabric-attached memory).

The APIs strive to be as generic as possible. Thus, we do not hardcode policy 
but instead seek providing generic mechanisms that can be used to support 
higher level policies. 

## Example Programs

ALPINiSM comes with several samples in the `examples` directory.

## This Document
 
This document is written in Doxygen and maintained in the ALPINiSM git instance at:
 
[git://git-pa1.labs.hpecorp.net/ssftm/alps](git://git-pa1.labs.hpecorp.net/ssftm/alps)

## Generating the documentation 

We include the ALPINiSM documentation as part of the source (as opposed to using 
a hosted wiki, such as the github wiki, as the definitive documentation) to 
enable the documentation to evolve along with the source code and be captured 
by revision control (currently git). This way the code automatically includes 
the version of the documentation that is relevant regardless of which version 
or release you have checked out or downloaded.
 
 @verbatim
 $ cd $ALPS/doc
 $ doxygen
 @endverbatim

## Reporting issues

Please report feedback, including performance and correctness issues and 
extension requests, through the ALPS Jira instance:

https://jira-pa1.labs.hpecorp.net/browse/ALPS/

## Stable download locations

The most recent version is published at: N/A

## Contact information
 
@author Haris Volos <haris.volos@hpe.com>
