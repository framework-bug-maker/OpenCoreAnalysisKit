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

#include "command/command_manager.h"
#include "command/help.h"
#include "command/cmd_core.h"
#include "command/cmd_exec.h"
#include "command/cmd_sysroot.h"
#include "command/cmd_auxv.h"
#include "command/cmd_file.h"
#include "command/cmd_linkmap.h"
#include "command/cmd_read.h"
#include "command/cmd_write.h"
#include "command/cmd_shell.h"
#include "common/exception.h"
#include "base/utils.h"
#include <string.h>

CommandManager* CommandManager::INSTANCE;

void CommandManager::Init() {
    INSTANCE = new CommandManager;
    CommandManager::PushInlineCommand(new CoreCommand());
    CommandManager::PushInlineCommand(new ExecCommand());
    CommandManager::PushInlineCommand(new SysRootCommand());
    CommandManager::PushInlineCommand(new AuxvCommand());
    CommandManager::PushInlineCommand(new FileCommand());
    CommandManager::PushInlineCommand(new LinkMapCommand());
    CommandManager::PushInlineCommand(new ReadCommand());
    CommandManager::PushInlineCommand(new WriteCommand());
    CommandManager::PushInlineCommand(new ShellCommand());
    CommandManager::PushInlineCommand(new Help());
}

int CommandManager::Execute(const char* cmd, int argc, char* const argv[]) {
    if (!cmd) return -1;
    Command* command = FindCommand(cmd);
    if (command) {
        try {
            ShellCommand* shell;
            int position;
            bool writeout = false;
            for (int i = 0; i < argc; ++i) {
                if (!strcmp(argv[i], "|")) {
                    ++i;
                    if (i < argc) {
                        shell = reinterpret_cast<ShellCommand *>(FindCommand("shell"));
                        position = i;
                        break;
                    }
                } else if (!strcmp(argv[i], ">")) {
                    ++i;
                    if (i < argc) {
                        position = i;
                        writeout = true;
                        break;
                    }
                }
            }

            if (shell) {
                int nargc = position - 1;
                auto callback = [&command, nargc, &argv]() {
                    command->main(nargc, argv);
                };
                shell->main(argc - position, &argv[position], callback);
                return 0;
            } else if (writeout) {
                Utils::FreopenWrite(argv[position]);
                int nargc = position - 1;
                command->main(nargc, argv);
                Utils::CloseWriteout();
            }
            return command->main(argc, argv);
        } catch (InvalidAddressException e) {
            std::cout << e.what() << std::endl;
        }
    } else {
        std::cout << "Not found command." << std::endl;
    }
    return 0;
}

void CommandManager::ForeachCommand(std::function<void (Command *)> callback) {
    INSTANCE->foreachInlineCommand(callback);
    INSTANCE->foreachExtendCommand(callback);
}

Command* CommandManager::FindCommand(const char* cmd) {
    Command* result = nullptr;
    auto callback = [cmd, &result](Command* command) {
        if (command->get() == cmd || command->shortcut() == cmd) {
            result = command;
        }
    };
    ForeachCommand(callback);
    return result;
}

void CommandManager::foreachInlineCommand(std::function<void (Command *)> callback) {
    for (const auto& command : inline_commands)
        callback(command.get());
}

void CommandManager::foreachExtendCommand(std::function<void (Command *)> callback) {
    for (const auto& command : extend_commands)
        callback(command.get());
}

void CommandManager::PushInlineCommand(Command* command) {
    INSTANCE->pushInlineCommand(command);
}

void CommandManager::pushInlineCommand(Command* command) {
    std::unique_ptr<Command> ref(command);
    inline_commands.push_back(std::move(ref));
}

void CommandManager::PushExtendCommand(Command* command) {
    INSTANCE->pushExtendCommand(command);
}

void CommandManager::pushExtendCommand(Command* command) {
    std::unique_ptr<Command> ref(command);
    extend_commands.push_back(std::move(ref));
}