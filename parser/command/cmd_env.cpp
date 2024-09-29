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
#include "android.h"
#include "runtime/cache_helpers.h"
#include "command/cmd_env.h"
#include "command/env.h"
#include "api/core.h"
#include "api/elf.h"
#include "common/elf.h"
#include "common/disassemble/capstone.h"
#include "base/utils.h"
#include "base/macros.h"
#include <linux/elf.h>
#include <unistd.h>
#include <getopt.h>

typedef int (*EnvCall)(int argc, char* const argv[]);
struct EnvOption {
    const char* cmd;
    EnvCall call;
};

static EnvOption env_option[] = {
    { "config", EnvCommand::onConfigChanged },
    { "logger", EnvCommand::onLoggerChanged },
    { "art", EnvCommand::showArtEnv },
    { "core", EnvCommand::showCoreEnv },
};

int EnvCommand::main(int argc, char* const argv[]) {
    if (!(argc > 1))
        return dumpEnv();

    int count = sizeof(env_option)/sizeof(env_option[0]);
    for (int index = 0; index < count; ++index) {
        if (!strcmp(argv[1], env_option[index].cmd)) {
            return env_option[index].call(argc - 1, &argv[1]);
        }
    }

    LOGI("unknown command (%s)\n", argv[1]);
    return 0;
}

int EnvCommand::onConfigChanged(int argc, char* const argv[]) {
    if (!CoreApi::IsReady())
        return 0;

    int opt;
    int option_index = 0;
    int current_sdk = Android::UPSIDE_DOWN_CAKE;
    int current_pid = Env::CurrentPid();
    int current_oat = 0;
    optind = 0; // reset
    static struct option long_options[] = {
        {"pid",     required_argument, 0, 'p'},
        {"sdk",     required_argument, 0,  0 },
        {"oat",     required_argument, 0,  1 },
        {0,         0,                 0,  0 }
    };

    while ((opt = getopt_long(argc, argv, "p:0:1:",
                long_options, &option_index)) != -1) {
        switch (opt) {
            case 'p':
                current_pid = atoi(optarg);
                if (Env::SetCurrentPid(current_pid))
                    Env::Dump();
                break;
            case 0:
                if (Android::IsReady()) {
                    current_sdk = atoi(optarg);
                    Android::OnSdkChanged(current_sdk);
                }
                break;
            case 1:
                if (Android::IsReady()) {
                    current_oat = atoi(optarg);
                    Android::OnOatChanged(current_oat);
                }
                break;
        }
    }

    return 0;
}

int EnvCommand::onLoggerChanged(int argc, char* const argv[]) {
    int opt;
    int option_index = 0;
    optind = 0; // reset
    static struct option long_options[] = {
        {"debug",   no_argument,       0, Logger::LEVEL_DEBUG},
        {"info",    no_argument,       0, Logger::LEVEL_INFO},
        {"warn",    no_argument,       0, Logger::LEVEL_WARN},
        {"error",   no_argument,       0, Logger::LEVEL_ERROR},
        {"fatal",   no_argument,       0, Logger::LEVEL_FATAL},
        {0,         0,                 0,  0 }
    };

    if (argc < 2) {
        LOGI("Current logger level %s\n", long_options[Logger::GetLevel()].name);
        return 0;
    }

    while ((opt = getopt_long(argc, argv, "01234",
                long_options, &option_index)) != -1) {
        switch (opt) {
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
                LOGI("Switch logger level %s\n", long_options[option_index].name);
                Logger::SetLevel(opt);
                break;
            default:
                LOGI("Unkown logger level.\n");
                break;
        }
    }
    return 0;
}

int EnvCommand::showArtEnv(int argc, char* const argv[]) {
    if (!CoreApi::IsReady() || !Android::IsReady())
        return 0;

    art::Runtime& runtime = Android::GetRuntime();

    int opt;
    int option_index = 0;
    optind = 0; // reset
    static struct option long_options[] = {
        {"clean-cache",   no_argument, 0, 'c'},
        {"entry-points",  no_argument, 0, 'e'},
        {"nterp",         no_argument, 0, 'n'},
    };

    while ((opt = getopt_long(argc, argv, "cen",
                long_options, &option_index)) != -1) {
        switch (opt) {
            case 'c':
                art::CacheHelper::Clean();
                if (runtime.Ptr()) {
                    runtime.CleanCache();
                    runtime = 0x0;
                }
                Android::ResetOatVersion();
                return 0;
            case 'e':
                art::CacheHelper::EntryPointDump();
                return 0;
            case 'n':
                art::CacheHelper::NterpDump();
                return 0;
        }
    }

    LOGI("  * LIB: " ANSI_COLOR_LIGHTGREEN "%s\n" ANSI_COLOR_RESET, Android::GetRealLibart().c_str());
    LOGI("  * art::OatHeader::kOatVersion: " ANSI_COLOR_LIGHTMAGENTA "%d\n" ANSI_COLOR_RESET, Android::Oat());
    LOGI("  * art::Runtime: " ANSI_COLOR_LIGHTMAGENTA "0x%lx\n" ANSI_COLOR_RESET, runtime.Ptr());
    if (!runtime.Ptr())
        return 0;

    LOGI("  * art::gc::Heap: " ANSI_COLOR_LIGHTMAGENTA "0x%lx\n" ANSI_COLOR_RESET, runtime.GetHeap().Ptr());
    if (runtime.GetHeap().Ptr()) {
        LOGI("  *     continuous_spaces_: " ANSI_COLOR_LIGHTMAGENTA "0x%lx\n" ANSI_COLOR_RESET, runtime.GetHeap().GetContinuousSpacesCache().Ptr());
        LOGI("  *     discontinuous_spaces_: " ANSI_COLOR_LIGHTMAGENTA "0x%lx\n" ANSI_COLOR_RESET, runtime.GetHeap().GetDiscontinuousSpacesCache().Ptr());
    }
    LOGI("  * art::MonitorPool: " ANSI_COLOR_LIGHTMAGENTA "0x%lx\n" ANSI_COLOR_RESET, runtime.GetMonitorPool().Ptr());
    LOGI("  * art::ThreadList: " ANSI_COLOR_LIGHTMAGENTA "0x%lx\n" ANSI_COLOR_RESET, runtime.GetThreadList().Ptr());
    if (runtime.GetThreadList().Ptr()) {
        LOGI("  *     list_: " ANSI_COLOR_LIGHTMAGENTA "0x%lx\n" ANSI_COLOR_RESET, runtime.GetThreadList().GetListCache().Ptr());
    }
    LOGI("  * art::ClassLinker: " ANSI_COLOR_LIGHTMAGENTA "0x%lx\n" ANSI_COLOR_RESET, runtime.GetClassLinker().Ptr());
    if (runtime.GetClassLinker().Ptr()) {
        if (Android::Sdk() < Android::TIRAMISU) {
            LOGI("  *     dex_caches_: " ANSI_COLOR_LIGHTMAGENTA "0x%lx\n" ANSI_COLOR_RESET, runtime.GetClassLinker().GetDexCachesData().Ptr());
        } else {
            LOGI("  *     dex_caches_: " ANSI_COLOR_LIGHTMAGENTA "0x%lx\n" ANSI_COLOR_RESET, runtime.GetClassLinker().GetDexCachesData_v33().Ptr());
        }
    }
    LOGI("  * art::JavaVMExt: " ANSI_COLOR_LIGHTMAGENTA "0x%lx\n" ANSI_COLOR_RESET, runtime.GetJavaVM().Ptr());
    if (runtime.GetJavaVM().Ptr()) {
        LOGI("  *     globals_: " ANSI_COLOR_LIGHTMAGENTA "0x%lx\n" ANSI_COLOR_RESET, runtime.GetJavaVM().GetGlobalsTable().Ptr());
        LOGI("  *     weak_globals_: " ANSI_COLOR_LIGHTMAGENTA "0x%lx\n" ANSI_COLOR_RESET, runtime.GetJavaVM().GetWeakGlobalsTable().Ptr());
    }
    LOGI("  * art::jit::Jit: " ANSI_COLOR_LIGHTMAGENTA "0x%lx\n" ANSI_COLOR_RESET, runtime.GetJit().Ptr());
    if (runtime.GetJit().Ptr()) {
        LOGI("  *     code_cache_: " ANSI_COLOR_LIGHTMAGENTA "0x%lx\n" ANSI_COLOR_RESET, runtime.GetJit().GetCodeCache().Ptr());
    }
    return 0;
}

int EnvCommand::showCoreEnv(int argc, char* const argv[]) {
    int opt;
    int option_index = 0;
    optind = 0; // reset
    static struct option long_options[] = {
        {"load",   no_argument,       0, 1},
        {"arm",    required_argument, 0, 2},
        {"crc",    no_argument,       0, 3},
        {"num",    required_argument, 0,'n'},
        {"quick-load", no_argument,   0, 4},
    };

    bool crc = false;
    int num = 0;
    while ((opt = getopt_long(argc, argv, "12:3:4n:",
                long_options, &option_index)) != -1) {
        switch (opt) {
            case 1: return showLoadEnv(false);
            case 2:
                capstone::Disassember::SetArmMode(optarg);
                return 0;
            case 3:
                crc = true;
                break;
            case 4: return showLoadEnv(true);
            case 'n':
                num = std::atoi(optarg);
                break;
        }
    }
    if (crc) {
        clocLoadCRC32(num);
    } else {
        LOGI("  * r_debug: " ANSI_COLOR_LIGHTMAGENTA "0x%lx\n" ANSI_COLOR_RESET, CoreApi::GetDebugPtr());
        LOGI("  * arm mode: " ANSI_COLOR_LIGHTMAGENTA "%s\n" ANSI_COLOR_RESET, !capstone::Disassember::GetArmMode() ? "arm" : "thumb");
        LOGI("  * mLoad: " ANSI_COLOR_LIGHTMAGENTA "%ld\n" ANSI_COLOR_RESET, CoreApi::GetLoads(false).size());
        LOGI("  * mQuickLoad: " ANSI_COLOR_LIGHTMAGENTA "%ld\n" ANSI_COLOR_RESET, CoreApi::GetLoads(true).size());
    }
    return 0;
}

int EnvCommand::showLoadEnv(bool quick) {
    if (!CoreApi::IsReady())
        return 0;

    int index = 0;
    auto callback = [&index](LoadBlock *block) -> bool {
        index++;
        std::string name;
        if (block->isMmapBlock()) {
            name.append(ANSI_COLOR_GREEN);
            name.append(block->name());
            name.append(ANSI_COLOR_RESET);
        } else {
            name.append("[]");
        }
        std::string valid;
        if (block->isValid()) {
            valid.append("[*]");
            if (block->isOverlayBlock()) {
                valid.append("(OVERLAY)");
            } else if (block->isMmapBlock()) {
                valid.append("(MMAP");
                if (block->GetMmapOffset()) {
                    valid.append(" ");
                    valid.append(Utils::ToHex(block->GetMmapOffset()));
                }
                valid.append(")");
            }
        } else {
            valid.append("[EMPTY]");
        }
        LOGI("  %-5d " ANSI_COLOR_CYAN "[%lx, %lx)" ANSI_COLOR_RESET "  %s  %010lx  ""%s"" %s\n",
                index, block->vaddr(), block->vaddr() + block->size(), block->convertFlags().c_str(),
                block->realSize(), name.c_str(), valid.c_str());
        return false;
    };
    LOGI(ANSI_COLOR_LIGHTRED "INDEX   REGION               FLAGS FILESZ      PATH\n" ANSI_COLOR_RESET);
    CoreApi::ForeachLoadBlock(callback, false, quick);
    return 0;
}

int EnvCommand::clocLoadCRC32(int num) {
    bool cloc_all = num == 0;
    if (!CoreApi::IsReady())
        return 0;

    bool first = true;
    int index = 0;
    auto callback = [&](LoadBlock *block) -> bool {
        index++;
        if (!cloc_all && num != index)
            return false;

        if (!block->isMmapBlock() || !block->isValidBlock())
            return false;

        // only check .text crc32
        // if (!(block->flags() & Block::FLAG_X))
        //    return false;

        if (cloc_all || num == index) {
            uint32_t or_crc = 0x0;
            uint32_t mmap_crc = 0x0;

            ElfHeader* header = reinterpret_cast<ElfHeader*>(block->begin(LoadBlock::OPT_READ_MMAP));
            if (!memcmp(header->ident, ELFMAG, 4)) {
                // skip elf header
                or_crc = Utils::CRC32(reinterpret_cast<uint8_t*>(block->begin(LoadBlock::OPT_READ_OR)) + SIZEOF(Elfx_Ehdr),
                        block->size() - SIZEOF(Elfx_Ehdr));
                mmap_crc = Utils::CRC32(reinterpret_cast<uint8_t*>(block->begin(LoadBlock::OPT_READ_MMAP)) + SIZEOF(Elfx_Ehdr),
                        block->size() - SIZEOF(Elfx_Ehdr));
            } else {
                or_crc = block->GetCRC32(LoadBlock::OPT_READ_OR);
                mmap_crc = block->GetCRC32(LoadBlock::OPT_READ_MMAP);
            }

            if (or_crc != mmap_crc) {
                std::string name;
                name.append(ANSI_COLOR_GREEN);
                name.append(block->name());
                name.append(ANSI_COLOR_RESET);

                if (!first) { ENTER(); }
                first = false;
                LOGI("%-5d " ANSI_COLOR_CYAN "[%lx, %lx)" ANSI_COLOR_RESET "  %s  %010lx  ""%s""\n",
                        index, block->vaddr(), block->vaddr() + block->size(), block->convertFlags().c_str(),
                        block->realSize(), name.c_str());

                uint64_t* orv = reinterpret_cast<uint64_t*>(block->begin(LoadBlock::OPT_READ_OR));
                uint64_t* mmv = reinterpret_cast<uint64_t*>(block->begin(LoadBlock::OPT_READ_MMAP));
                int count = RoundUp(block->size() / 8, 2);
                LinkMap::NiceSymbol symbol;
                for (int k = 0; k < count; k += 2) {
                    uint64_t orv1 = orv[k];
                    uint64_t orv2 = orv[k + 1];
                    uint64_t mmv1 = mmv[k];
                    uint64_t mmv2 = mmv[k + 1];
                    if (LIKELY(orv1 == mmv1) && LIKELY(orv2 == mmv2))
                        continue;

                    uint64_t current = block->vaddr() + k * 8;
                    if (!symbol.IsValid() ||
                            (current < symbol.GetOffset() ||
                             current >= symbol.GetOffset() + symbol.GetSize())) {
                        if (block->handle()) {
                            symbol = LinkMap::NiceSymbol::Invalid();
                            block->handle()->NiceMethod(current, symbol);
                            if (symbol.IsValid()) LOGI(ANSI_COLOR_YELLOW "%s" ANSI_COLOR_RESET ":\n", symbol.GetSymbol().c_str());
                        }
                    }
                    LOGI(ANSI_COLOR_CYAN "%lx" ANSI_COLOR_RESET ": %016lx  %016lx  %s%s  |  %016lx  %016lx  %s%s\n",
                            current, orv1, orv2,
                            Utils::ConvertAscii(orv1, 8).c_str(), Utils::ConvertAscii(orv2, 8).c_str(),
                            mmv1, mmv2,
                            Utils::ConvertAscii(mmv1, 8).c_str(), Utils::ConvertAscii(mmv2, 8).c_str());
                }
            }
        }
        return false;
    };
    CoreApi::ForeachLoadBlock(callback, false);
    return 0;
}

int EnvCommand::dumpEnv() {
    if (CoreApi::IsReady()) {
        CoreApi::Dump();
        Env::Dump();

        if (Android::IsReady()) {
            Android::Dump();
        }
    }
    return 0;
}

void EnvCommand::usage() {
    LOGI("Usage: env <COMMAND> [option] ...\n");
    LOGI("Command:\n");
    LOGI("    config  logger  art  core\n");
    ENTER();
    LOGI("Usage: env config <option> ..\n");
    LOGI("Option:\n");
    LOGI("   --sdk: <VERSION>\n");
    LOGI("   --oat: <VERSION>\n");
    LOGI("   --pid|-p <PID>\n");
    ENTER();
    LOGI("Usage: env logger <option>\n");
    LOGI("Option:\n");
    LOGI("   --[debug|info|warn|error|fatal]\n");
    ENTER();
    LOGI("Usage: env art [option] ...\n");
    LOGI("Option:\n");
    LOGI("   --clean-cache|-c: clean art::Runtime cache\n");
    LOGI("   --entry-points|-e: show art quick entry points\n");
    LOGI("   --nterp|-n: show art nterp cache\n");
    ENTER();
    LOGI("Usage: env core [option]...\n");
    LOGI("Option:\n");
    LOGI("   --load: show code load segments\n");
    LOGI("   --arm <thumb|arm>\n");
    LOGI("   --crc: check consistency of mmap file data.\n");
}
