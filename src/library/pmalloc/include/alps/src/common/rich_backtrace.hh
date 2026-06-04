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

#ifndef _ALPS_COMMON_RICH_BACKTRACE_HH_
#define _ALPS_COMMON_RICH_BACKTRACE_HH_

#include <string>
#include <vector>

namespace alps {

/**
 * @brief Returns the backtrace information of the current stack.
 * @details
 * If rich flag is given, the backtrace information is converted to human-readable format
 * as much as possible via addr2line (which is linux-only).
 * Also, this method does not care about out-of-memory situation.
 * When you are really concerned with it, just use backtrace, backtrace_symbols_fd etc.
 */
std::vector<std::string> get_backtrace(bool rich = true);

}  // namespace alps


#endif  // _ALPS_COMMON_RICH_BACKTRACE_HH_
