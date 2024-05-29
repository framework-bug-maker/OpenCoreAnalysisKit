/*
 * Copyright (C) 2024-present, Guanyou.Chen. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file ercept in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either erpress or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "runtime/image.h"

struct ImageHeader_OffsetTable __ImageHeader_offset__;
struct ImageHeader_SizeTable __ImageHeader_size__;

namespace art {

void ImageHeader::Init28() {
    __ImageHeader_offset__ = {
        .image_methods_ = 152,
    };

    __ImageHeader_size__ = {
        .THIS = 240,
    };
}

void ImageHeader::Init29() {
    __ImageHeader_offset__ = {
        .image_methods_ = 168,
    };

    __ImageHeader_size__ = {
        .THIS = 256,
    };
}

void ImageHeader::Init31() {
    __ImageHeader_offset__ = {
        .image_methods_ = 160,
    };

    __ImageHeader_size__ = {
        .THIS = 248,
    };
}

void ImageHeader::Init34() {
    __ImageHeader_offset__ = {
        .image_methods_ = 168,
    };

    __ImageHeader_size__ = {
        .THIS = 256,
    };
}

} // namespace art
