// Copyright 2023 The Tint Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "src/tint/utils/memory/bump_allocator.h"

#include "gtest/gtest.h"

namespace tint {
namespace {

using BumpAllocatorTest = testing::Test;

TEST_F(BumpAllocatorTest, AllocationSizes) {
    BumpAllocator allocator;
    for (size_t n : {1u, 0x10u, 0x100u, 0x1000u, 0x10000u, 0x100000u,  //
                     2u, 0x34u, 0x567u, 0x8912u, 0x34567u, 0x891234u}) {
        auto ptr = allocator.Allocate(n);
        memset(ptr, 0x42, n);
    }
}

TEST_F(BumpAllocatorTest, AllocationSizesAroundBlockSize) {
    for (size_t n : {
             BumpAllocator::kDefaultBlockDataSize - sizeof(void*),
             BumpAllocator::kDefaultBlockDataSize - 4,
             BumpAllocator::kDefaultBlockDataSize - 1,
             BumpAllocator::kDefaultBlockDataSize,
             BumpAllocator::kDefaultBlockDataSize + 1,
             BumpAllocator::kDefaultBlockDataSize + 4,
             BumpAllocator::kDefaultBlockDataSize + sizeof(void*),
         }) {
        BumpAllocator allocator;
        auto* ptr = allocator.Allocate(n);
        memset(ptr, 0x42, n);
    }
}

TEST_F(BumpAllocatorTest, Count) {
    for (size_t n : {0u, 1u, 10u, 16u, 20u, 32u, 50u, 64u, 100u, 256u, 300u, 512u, 500u, 512u}) {
        BumpAllocator allocator;
        EXPECT_EQ(allocator.Count(), 0u);
        for (size_t i = 0; i < n; i++) {
            allocator.Allocate(5);
        }
        EXPECT_EQ(allocator.Count(), n);
    }
}

TEST_F(BumpAllocatorTest, MoveConstruct) {
    for (size_t n : {0u, 1u, 10u, 16u, 20u, 32u, 50u, 64u, 100u, 256u, 300u, 512u, 500u, 512u}) {
        BumpAllocator allocator_a;
        for (size_t i = 0; i < n; i++) {
            allocator_a.Allocate(5);
        }
        EXPECT_EQ(allocator_a.Count(), n);

        BumpAllocator allocator_b{std::move(allocator_a)};
        EXPECT_EQ(allocator_b.Count(), n);
    }
}

}  // namespace
}  // namespace tint
