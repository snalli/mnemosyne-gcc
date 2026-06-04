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

#ifndef _ALPS_PEGASUS_MULTI_REGIONFILE_HH_
#define _ALPS_PEGASUS_MULTI_REGIONFILE_HH_

#include <string>
#include <vector>
#include <boost/filesystem.hpp>
#include "region_file.hh"

namespace alps {

/**
 * @brief Represents a pseudo region file comprising multiple region files.
 * 
 * @details We provide this pseudo region file as a helper class for 
 * glueing and mapping multiple existing region files as a single contiguous file.
 * This class does not support operations that require modifying file-system 
 * metadata of underlying region files, including create, truncate, unlink, 
 * set/get attributes.
 * 
 */
class MultiRegionFile: public RegionFile {
public:
    MultiRegionFile(const std::vector<RegionFile*> region_files)
        : region_files_(region_files)
    { }

    ~MultiRegionFile() {};  

    /**
     * @brief Operation not supported for this region file type.
     */
    ErrorCode create(mode_t mode);
    ErrorCode open(int flags, mode_t mode);
    ErrorCode open(int flags);

    /**
     * @brief Operation not supported for this region file type.
     */
    ErrorCode unlink();

    ErrorCode close();

    /**
     * @brief Operation not supported for this region file type.
     */
    ErrorCode truncate(loff_t length);
    ErrorCode size(loff_t* length);
    ErrorCode map(void* addr_hint, size_t length, int prot, int flags, loff_t offset, void** mapped_addr);
    ErrorCode unmap(void* addr, size_t length);

    /**
     * @brief Operation not supported for this region file type.
     */
    ErrorCode getxattr(const char *name, void *value, size_t size);

    /**
     * @brief Operation not supported for this region file type.
     */
    ErrorCode setxattr(const char *name, const void *value, size_t size, int flags);

    size_t booksize();
 
public:
    /* methods specialized by multi region file implementation */

    ErrorCode set_interleave_group(loff_t offset, loff_t length, const std::vector<InterleaveGroup>& vig);
    ErrorCode interleave_group(loff_t offset, loff_t length, std::vector<InterleaveGroup>* vig);

private:
    std::vector<RegionFile*> region_files_;
};

} // namespace alps

#endif // _ALPS_PEGASUS_MULTI_REGIONFILE_HH_
