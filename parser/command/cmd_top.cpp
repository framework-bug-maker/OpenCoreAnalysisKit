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
#include "runtime/mirror/class.h"
#include "command/command_manager.h"
#include "command/cmd_top.h"
#include "sun/misc/Cleaner.h"
#include "libcore/util/NativeAllocationRegistry.h"
#include "api/core.h"
#include "android.h"
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sstream>
#include <regex>
#include <map>
#include <vector>

int TopCommand::main(int argc, char* const argv[]) {
    if (!CoreApi::IsReady()
            || !Android::IsSdkReady()
            || !(argc > 1))
        return 0;

    num = atoi(argv[1]);
    order = ORDERBY_ALLOC;
    show = false;

    int opt;
    int option_index = 0;
    optind = 0; // reset
    static struct option long_options[] = {
        {"alloc",      no_argument,       0,  'a'},
        {"shallow",    no_argument,       0,  's'},
        {"native",     no_argument,       0,  'n'},
        {"display",    no_argument,       0,  'd'},
        {"app",        no_argument,       0,   0 },
        {"zygote",     no_argument,       0,   1 },
        {"image",      no_argument,       0,   2 },
    };

    while ((opt = getopt_long(argc, argv, "asnd",
                long_options, &option_index)) != -1) {
        switch (opt) {
            case 'a':
                order = ORDERBY_ALLOC;
                break;
            case 's':
                order = ORDERBY_SHALLOW;
                break;
            case 'n':
                order = ORDERBY_NATIVE;
                break;
            case 'd':
                show = true;
                break;
            case 0:
                flag |= Android::EACH_APP_OBJECTS;
                break;
            case 1:
                flag |= Android::EACH_ZYGOTE_OBJECTS;
                break;
            case 2:
                flag |= Android::EACH_IMAGE_OBJECTS;
                break;
        }
    }

    if (!flag) {
        flag |= Android::EACH_APP_OBJECTS;
        flag |= Android::EACH_ZYGOTE_OBJECTS;
        flag |= Android::EACH_IMAGE_OBJECTS;
    }
    std::map<art::mirror::Class, TopCommand::Pair> classes;
    art::mirror::Class cleaner = 0;
    std::vector<art::mirror::Object> cleaners;
    auto callback = [&](art::mirror::Object& object) -> bool {
        if (object.IsClass())
            return false;

        art::mirror::Class thiz = object.GetClass();
        if (!cleaner.Ptr()) {
            if (thiz.PrettyDescriptor() == "sun.misc.Cleaner") {
                cleaner = thiz;
                cleaners.push_back(object);
            }
        } else if (cleaner == thiz) {
            cleaners.push_back(object);
        }

        auto it = classes.find(thiz.Ptr());
        if (it == classes.end()) {
            TopCommand::Pair pair = {
                .alloc_count = 1,
                .shallow_size = object.SizeOf(),
            };
            classes.insert(std::pair<art::mirror::Class, TopCommand::Pair>(thiz, pair));
        } else {
            TopCommand::Pair& pair = it->second;
            pair.alloc_count += 1;
            pair.shallow_size += object.SizeOf();
        }

        return false;
    };
    Android::ForeachObjects(callback, flag);

    LOGI("Address       Allocations       ShallowSize       NativeSize     %s\n", show ? "ClassName" : "");
    art::mirror::Class cur_max_thiz = 0;
    TopCommand::Pair cur_max_pair = {
        .alloc_count = 0,
        .shallow_size = 0,
        .native_size = 0,
    };

    for (int i = 0; i < cleaners.size(); i++) {
        sun::misc::Cleaner cleaner = cleaners[i];
        java::lang::Object referent = cleaner.getReferent();

        if (referent.isNull())
            continue;

        auto it = classes.find(referent.klass());
        if (it == classes.end())
            continue;

        try {
            libcore::util::NativeAllocationRegistry::CleanerThunk thunk = cleaner.getThunk();
            if (thunk.isNull())
                continue;

            libcore::util::NativeAllocationRegistry registry = thunk.getRegistry();
            if (registry.isNull())
                continue;

            TopCommand::Pair& pair = it->second;
            pair.native_size += registry.getSize();
        } catch (InvalidAddressException e) {}
    }

    uint64_t total_count = 0;
    uint64_t total_shallow = 0;
    uint64_t total_native = 0;

    for (const auto& value : classes) {
        const art::mirror::Class& thiz = value.first;
        const TopCommand::Pair& pair = value.second;

        total_count += pair.alloc_count;
        total_shallow += pair.shallow_size;
        total_native += pair.native_size;
    }

    LOGI("TOTAL            %8ld      %11ld       %11ld\n",
         total_count, total_shallow, total_native);
    LOGI("------------------------------------------------------------\n");

    for (int i = 0; i < num; ++i) {
        for (const auto& value : classes) {
            const art::mirror::Class& thiz = value.first;
            const TopCommand::Pair& pair = value.second;

            switch (order) {
                case ORDERBY_ALLOC: {
                   if (pair.alloc_count >= cur_max_pair.alloc_count) {
                       cur_max_thiz = thiz;
                       cur_max_pair = pair;
                   }
                } break;
                case ORDERBY_SHALLOW: {
                   if (pair.shallow_size >= cur_max_pair.shallow_size) {
                       cur_max_thiz = thiz;
                       cur_max_pair = pair;
                   }
                } break;
                case ORDERBY_NATIVE: {
                   if (pair.native_size >= cur_max_pair.native_size) {
                       cur_max_thiz = thiz;
                       cur_max_pair = pair;
                   }
                } break;
            }
        }

        if (!cur_max_thiz.Ptr())
            break;

        LOGI("0x%8lx       %8ld      %11ld       %11ld     %s\n",
             cur_max_thiz.Ptr(), cur_max_pair.alloc_count,
             cur_max_pair.shallow_size, cur_max_pair.native_size,
             show ? cur_max_thiz.PrettyDescriptor().c_str() : "");

        classes.erase(cur_max_thiz);
        cur_max_thiz = 0;
        cur_max_pair = {0, 0, 0};
    }
    return 0;
}

void TopCommand::usage() {
    LOGI("Usage: top <NUM> [--alloc|-a] [--shallow|-s] [--native|-n] [--display|-d] [--app|--zygote|--image]\n");
}

