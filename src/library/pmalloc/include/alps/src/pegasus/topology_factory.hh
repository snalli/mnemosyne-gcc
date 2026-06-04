/* 
 * (c) Copyright 2016 Hewlett Packard Enterprise Development LP
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "pegasus/fstype_factory.hh"
#include "pegasus/topology.hh"

namespace alps {

class TopologyFactory: public FileSystemTypeFactory<Topology> {
public:
    TopologyFactory(const PegasusOptions& pegasus_options)
        : FileSystemTypeFactory<Topology>(pegasus_options)
    { }

};

} // namespace alps
