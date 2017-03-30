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

#include <stdlib.h>
#include "gtest/gtest.h"
#include "pegas/invtbl.hh"
#include "pegas/vmarea.hh"

using namespace alps;

TEST(InvertedTableTest, find)
{
    InvertedTable tbl;
    VmArea vma1(0x0, 0x0, 0x7f0000000000, 0x7f0000001000);
    VmArea vma2(0x0, 0x1, 0x7f0000001000, 0x7f0000002000);
    VmArea vma3(0x0, 0x3, 0x7f0000003000, 0x7f0000004000);
    tbl.insert_vmarea(&vma1);
    tbl.insert_vmarea(&vma2);
    tbl.insert_vmarea(&vma3);

    EXPECT_EQ((uintptr_t) 0x0, tbl.find_vmarea(0x7f0000000000)->offset());
    EXPECT_EQ((uintptr_t) 0x1, tbl.find_vmarea(0x7f0000001000)->offset());
    EXPECT_EQ((uintptr_t) 0x1, tbl.find_vmarea(0x7f0000001100)->offset());
    EXPECT_EQ((uintptr_t) 0x0, tbl.find_vmarea(0x7f0000002000));
    EXPECT_EQ((uintptr_t) 0x3, tbl.find_vmarea(0x7f0000003100)->offset());
}

TEST(InvertedTableTest, remove1)
{
    InvertedTable tbl;
    VmArea vma1(0x0, 0x0, 0x7f0000000000, 0x7f0000001000);
    VmArea vma2(0x0, 0x1, 0x7f0000001000, 0x7f0000002000);
    tbl.insert_vmarea(&vma1);
    tbl.insert_vmarea(&vma2);
    tbl.remove_vmarea(&vma1);
    EXPECT_EQ(0x0, tbl.find_vmarea(0x7f0000000100));
    EXPECT_EQ((uintptr_t) 0x1, tbl.find_vmarea(0x7f0000001100)->offset());
}

TEST(InvertedTableTest, remove2)
{
    InvertedTable tbl;
    VmArea vma1(0x0, 0x0, 0x7f0000000000, 0x7f0000001000);
    VmArea vma2(0x0, 0x1, 0x7f0000001000, 0x7f0000002000);
    tbl.insert_vmarea(&vma1);
    tbl.insert_vmarea(&vma2);
    tbl.remove_vmarea(&vma2);
    EXPECT_EQ((uintptr_t) 0x0, tbl.find_vmarea(0x7f0000000000)->offset());
    EXPECT_EQ(0x0, tbl.find_vmarea(0x7f0000001100));
}


int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
