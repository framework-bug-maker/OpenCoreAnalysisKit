/*
 * Copyright (C) 2024-present, Guanyou.Chen. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "runtime/gc/space/large_object_space.h"
#include <string.h>

struct LargeObjectSpace_OffsetTable __LargeObjectSpace_offset__;
struct LargeObjectSpace_SizeTable __LargeObjectSpace_size__;
struct LargeObjectMapSpace_OffsetTable __LargeObjectMapSpace_offset__;
struct LargeObjectMapSpace_SizeTable __LargeObjectMapSpace_size__;
struct FreeListSpace_OffsetTable __FreeListSpace_offset__;
struct FreeListSpace_SizeTable __FreeListSpace_size__;

namespace art {
namespace gc {
namespace space {

void LargeObjectSpace::Init() {
    if (CoreApi::GetPointSize() == 64) {
        __LargeObjectSpace_offset__ = {
            .lock_ = 304,
            .num_bytes_allocated_ = 344,
            .num_objects_allocated_ = 352,
            .total_bytes_allocated_ = 360,
            .total_objects_allocated_ = 368,
            .begin_ = 376,
            .end_ = 384,
        };

        __LargeObjectSpace_size__ = {
            .THIS = 392,
        };
    } else {
        __LargeObjectSpace_offset__ = {
            .lock_ = 160,
            .num_bytes_allocated_ = 192,
            .num_objects_allocated_ = 200,
            .total_bytes_allocated_ = 208,
            .total_objects_allocated_ = 216,
            .begin_ = 224,
            .end_ = 228,
        };

        __LargeObjectSpace_size__ = {
            .THIS = 232,
        };
    }
}

void LargeObjectMapSpace::Init() {
    if (CoreApi::GetPointSize() == 64) {
        __LargeObjectMapSpace_offset__ = {
            .large_objects_ = 392,
        };

        __LargeObjectMapSpace_size__ = {
            .THIS = 416,
        };
    } else {
        __LargeObjectMapSpace_offset__ = {
            .large_objects_ = 232,
        };

        __LargeObjectMapSpace_size__ = {
            .THIS = 244,
        };
    }
}

void FreeListSpace::Init() {
    if (CoreApi::GetPointSize() == 64) {
        __FreeListSpace_offset__ = {
            .mem_map_ = 392,
            .allocation_info_map_ = 464,
            .allocation_info_ = 536,
            .free_end_ = 544,
            .free_blocks_ = 552,
        };

        __FreeListSpace_size__ = {
            .THIS = 576,
        };
    } else {
        __FreeListSpace_offset__ = {
            .mem_map_ = 232,
            .allocation_info_map_ = 272,
            .allocation_info_ = 312,
            .free_end_ = 316,
            .free_blocks_ = 320,
        };

        __FreeListSpace_size__ = {
            .THIS = 332,
        };
    }
}

bool LargeObjectSpace::IsFreeListSpace() {
    return !strcmp(GetName(), FREELIST_SPACE);
};

bool LargeObjectSpace::IsMemMapSpace() {
    return !strcmp(GetName(), MEMMAP_SPACE);
}

void LargeObjectMapSpace::Walk(std::function<bool (mirror::Object& object)> visitor) {

}

void FreeListSpace::Walk(std::function<bool (mirror::Object& object)> visitor) {
}

} // namespace space
} // namespace gc
} // namespace art
