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

#include "logger/log.h"
#include "command/cmd_cxx.h"
#include "base/utils.h"
#include "api/core.h"
#include "cxx/string.h"
#include "cxx/vector.h"
#include "cxx/map.h"
#include "cxx/list.h"
#include "cxx/unordered_map.h"
#include "cxx/deque.h"
#include <unistd.h>
#include <getopt.h>

typedef int (*CxxCall)(int argc, char* const argv[]);
struct CxxOption {
    const char* cmd;
    CxxCall call;
};

static CxxOption cxx_option[] = {
    {"string", CxxCommand::DumpCxxString},
    {"vector", CxxCommand::DumpCxxVector},
    {"map", CxxCommand::DumpCxxMap},
    {"unordered_map", CxxCommand::DumpCxxUnOrderedMap},
    {"list", CxxCommand::DumpCxxList},
    {"deque", CxxCommand::DumpCxxDeque},
};

int CxxCommand::main(int argc, char* const argv[]) {
    if (!CoreApi::IsReady())
        return 0;

    if (!(argc > 2)) {
        usage();
        return 0;
    }

    int count = sizeof(cxx_option)/sizeof(cxx_option[0]);
    for (int index = 0; index < count; ++index) {
        if (!strcmp(argv[1], cxx_option[index].cmd)) {
            return cxx_option[index].call(argc - 1, &argv[1]);
        }
    }
    LOGI("unknown command (%s)\n", argv[1]);
    return 0;
}

int CxxCommand::DumpCxxString(int argc, char* const argv[]) {
    uint64_t addr = Utils::atol(argv[1]) & CoreApi::GetVabitsMask();
    cxx::string target = addr;
    LOGI("%s\n", target.c_str());
    return 0;
}

int CxxCommand::DumpCxxVector(int argc, char* const argv[]) {
    int entry_size = CoreApi::GetPointSize();
    int opt;
    int option_index = 0;
    optind = 0; // reset
    static struct option long_options[] = {
        {"entry-size",       required_argument, 0,  'e'},
    };

    while ((opt = getopt_long(argc, argv, "e:",
                long_options, &option_index)) != -1) {
        switch(opt) {
            case 'e':
                entry_size = atoi(optarg);
                break;
        }
    }

    uint64_t addr = Utils::atol(argv[optind]) & CoreApi::GetVabitsMask();
    cxx::vector target = addr;
    target.SetEntrySize(entry_size);
    int idx = 0;
    for (const auto& value : target) {
        LOGI("[%d] 0x%lx\n", idx++, value);
    }
    return 0;
}

int CxxCommand::DumpCxxMap(int argc, char* const argv[]) {
    uint64_t addr = Utils::atol(argv[1]) & CoreApi::GetVabitsMask();
    cxx::map target = addr;
    int idx = 0;
    for (const auto& value : target) {
        LOGI("[%d] 0x%lx\n", idx++, value);
    }
    return 0;
}

int CxxCommand::DumpCxxUnOrderedMap(int argc, char* const argv[]) {
    uint64_t addr = Utils::atol(argv[1]) & CoreApi::GetVabitsMask();
    cxx::unordered_map target = addr;
    int idx = 0;
    for (const auto& value : target) {
        LOGI("[%d] 0x%lx\n", idx++, value);
    }
    return 0;
}

int CxxCommand::DumpCxxList(int argc, char* const argv[]) {
    uint64_t addr = Utils::atol(argv[1]) & CoreApi::GetVabitsMask();
    cxx::list target = addr;
    int idx = 0;
    for (const auto& value : target) {
        LOGI("[%d] 0x%lx\n", idx++, value);
    }
    return 0;
}

int CxxCommand::DumpCxxDeque(int argc, char* const argv[]) {
    uint64_t addr = Utils::atol(argv[1]) & CoreApi::GetVabitsMask();
    cxx::deque target = addr;
    int idx = 0;
    for (const auto& value : target) {
        LOGI("[%d] 0x%lx\n", idx++, value.Ptr());
    }
    return 0;
}

void CxxCommand::usage() {
    LOGI("Usage: cxx <TYPE> <ADDR> [OPTION]\n");
    LOGI("Type: {string, vector, map, unordered_map, list, deque}\n");
}