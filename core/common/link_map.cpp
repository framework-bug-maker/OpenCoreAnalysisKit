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

#include "api/core.h"
#include "common/link_map.h"

struct LinkMap_OffsetTable __LinkMap_offset__;
struct LinkMap_SizeTable __LinkMap_size__;

void LinkMap::Init() {
    uint32_t cap = 64 / CoreApi::GetPointSize();
    __LinkMap_offset__ = {
        .l_addr = 0,
        .l_name = 8 / cap,
        .l_ld = 16 / cap,
        .l_next = 24 / cap,
        .l_prev = 32 / cap,
    };

    __LinkMap_size__ = {
        .THIS = 40 / cap,
    };
}

api::MemoryRef& LinkMap::GetAddrCache() {
    if (!addr_cache.Ptr()) {
        addr_cache = l_addr();
        addr_cache.Prepare(false);
    }
    return addr_cache;
}

api::MemoryRef& LinkMap::GetNameCache() {
    if (!name_cache.Ptr()) {
        name_cache = l_name();
        name_cache.Prepare(false);
    }
    return name_cache;
}

uint64_t LinkMap::begin() {
    return GetAddrCache().Ptr();
}

const char* LinkMap::name() {
    if (!GetNameCache().IsReady())
        return "";
    return reinterpret_cast<const char*>(GetNameCache().Real());
}

LoadBlock* LinkMap::block() {
    return GetAddrCache().Block();
}