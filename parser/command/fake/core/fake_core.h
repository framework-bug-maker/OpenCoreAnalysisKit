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

#ifndef PARSER_COMMAND_FAKE_CORE_FAKECORE_H_
#define PARSER_COMMAND_FAKE_CORE_FAKECORE_H_

#include "tombstone/tombstone.h"
#include <memory>

class FakeCore {
public:
    static inline const char* FILE_EXTENSIONS = ".fakecore";
    FakeCore() {}
    virtual ~FakeCore() {}
    virtual int execute(const char* output) { return 0; }

    static int OptionCore(int argc, char* const argv[]);
    static void Usage();
    static std::unique_ptr<FakeCore> Make(int bits);
    static std::unique_ptr<FakeCore> Make(android::Tombstone& tombstone);
};

#endif // PARSER_COMMAND_FAKE_CORE_FAKECORE_H_
