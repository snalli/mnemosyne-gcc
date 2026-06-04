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

set_property(GLOBAL APPEND PROPERTY ALL_ALPS_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/assert_nd.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/assorted_func.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/debug_options.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/externalizable.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/error_stack.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/log.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/os.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/rich_backtrace.cc
)
