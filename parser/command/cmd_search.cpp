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
#include "command/command_manager.h"
#include "command/cmd_search.h"
#include "java/lang/Object.h"
#include "base/utils.h"
#include "api/core.h"
#include "android.h"
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sstream>
#include <regex>

int SearchCommand::main(int argc, char* const argv[]) {
    if (!CoreApi::IsReady()
            || !Android::IsSdkReady()
            || !(argc > 1))
        return 0;

    int opt;
    int option_index = 0;
    optind = 0; // reset
    static struct option long_options[] = {
        {"object",   no_argument,       0,  'o'},
        {"class",    no_argument,       0,  'c'},
        {"regex",    no_argument,       0,  'r'},
        {"print",     no_argument,      0,  'p'},
        {"show",      no_argument,      0,  's'},
        {"instanceof",  no_argument,    0,  'i'},
        {"app",        no_argument,     0,   0 },
        {"zygote",     no_argument,     0,   1 },
        {"image",      no_argument,     0,   2 },
        {"fake",       no_argument,     0,   3 },
        {"hex",        no_argument,     0,  'x'},
    };

    type_flag = 0;
    each_flag = 0;
    regex = false;
    show = false;
    format_hex = false;
    while ((opt = getopt_long(argc, argv, "ocrpixs",
                long_options, &option_index)) != -1) {
        switch (opt) {
            case 'o':
                type_flag |= SEARCH_OBJECT;
                break;
            case 'c':
                type_flag |= SEARCH_CLASS;
                break;
            case 'r':
                regex = true;
                instof = false;
                break;
            case 's':
                show = true;
                break;
            case 'p':
                show = true;
                break;
            case 'i':
                regex = false;
                instof = true;
                break;
            case 'x':
                format_hex = true;
                break;
            case 0:
                each_flag |= Android::EACH_APP_OBJECTS;
                break;
            case 1:
                each_flag |= Android::EACH_ZYGOTE_OBJECTS;
                break;
            case 2:
                each_flag |= Android::EACH_IMAGE_OBJECTS;
                break;
            case 3:
                each_flag |= Android::EACH_FAKE_OBJECTS;
                break;
        }
    }

    total_objects = 0;
    const char* classname = argv[optind];
    if (!type_flag) type_flag = SEARCH_OBJECT | SEARCH_CLASS;

    if (!each_flag) {
        each_flag |= Android::EACH_APP_OBJECTS;
        each_flag |= Android::EACH_ZYGOTE_OBJECTS;
        each_flag |= Android::EACH_IMAGE_OBJECTS;
        each_flag |= Android::EACH_FAKE_OBJECTS;
    }
    auto callback = [&](art::mirror::Object& object) -> bool {
        return SearchObjects(classname, object);
    };
    Android::ForeachObjects(callback, each_flag, false);
    return 0;
}

bool SearchCommand::SearchObjects(const char* classsname, art::mirror::Object& object) {
    int mask = object.IsClass() ? SEARCH_CLASS : SEARCH_OBJECT;
    if (!(type_flag & mask))
        return false;

    std::string descriptor;
    art::mirror::Class thiz = 0x0;
    if (object.IsClass()) {
        thiz = object;
    } else {
        thiz = object.GetClass();
    }
    descriptor = thiz.PrettyDescriptor();

    java::lang::Object java = object;
    if (regex && std::regex_search(descriptor, std::regex(classsname))
            || descriptor == classsname
            || (instof && java.instanceof(classsname))) {
        total_objects++;
        LOGI("[%ld] " ANSI_COLOR_LIGHTYELLOW  "0x%lx" ANSI_COLOR_LIGHTCYAN " %s\n" ANSI_COLOR_RESET,
                total_objects, object.Ptr(), descriptor.c_str());
        if (show) {
            int argc = 2;
            std::string address = Utils::ToHex(object.Ptr());
            char* argv[3] = {
                const_cast<char*>("p"),
                const_cast<char*>(address.c_str()),
                const_cast<char*>(""),};
            if (format_hex) {
                argc++;
                argv[2] = const_cast<char*>("--hex");
            }
            CommandManager::Execute(argv[0], argc, argv);
        }
    }

    return false;
}

void SearchCommand::usage() {
    LOGI("Usage: search <CLASSNAME> [OPTION..] [TYPE]\n");
    LOGI("Option:\n");
    LOGI("    -r, --regex        regular expression search\n");
    LOGI("    -i, --instanceof   search by instance of class\n");
    LOGI("    -o, --object       only search object\n");
    LOGI("    -c, --class        only search class\n");
    LOGI("    -p, --print        object print detail\n");
    LOGI("    -x, --hex          basic type hex print\n");
    LOGI("Type: {--app, --zygote, --image, --fake}\n");
    ENTER();
    LOGI("core-parser> search android.app.Activity -i -o --app --print\n");
    LOGI("[1] 0x13050cc8 penguin.opencore.tester.MainActivity\n");
    LOGI("Size: 0x130\n");
    LOGI("Object Name: penguin.opencore.tester.MainActivity\n");
    LOGI("  // extends androidx.appcompat.app.AppCompatActivity\n");
    LOGI("    [0x12c] private android.content.res.Resources mResources = 0x0\n");
    LOGI("    [0x128] private androidx.appcompat.app.AppCompatDelegate mDelegate = 0x130520b8\n");
    LOGI("  // extends androidx.fragment.app.FragmentActivity\n");
    LOGI("    [0x125] boolean mStopped = false\n");
    LOGI("    [0x124] boolean mStartedIntentSenderFromFragment = false\n");
    LOGI("    [0x123] boolean mStartedActivityFromFragment = false\n");
    LOGI("    [0x122] boolean mResumed = true\n");
    LOGI("    [0x121] boolean mRequestedPermissionsFromFragment = false\n");
    LOGI("    [0x120] boolean mCreated = true\n");
    LOGI("    [0x11c] int mNextCandidateRequestIndex = 0\n");
    LOGI("    [0x118] androidx.collection.SparseArrayCompat mPendingFragmentActivityResults = 0x13052188\n");
    LOGI("    [0x114] final androidx.fragment.app.FragmentController mFragments = 0x13052178\n");
    LOGI("    [0x110] final androidx.lifecycle.LifecycleRegistry mFragmentLifecycleRegistry = 0x13052150\n");
    LOGI("    ...\n");
}

