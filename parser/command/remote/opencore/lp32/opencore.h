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

#ifndef PARSER_COMMAND_REMOTE_OPENCORE_LP32_OPENCORE_IMPL_H_
#define PARSER_COMMAND_REMOTE_OPENCORE_LP32_OPENCORE_IMPL_H_

#include "command/remote/opencore/opencore.h"
#include "lp32/core.h"
#include <linux/elf.h>

namespace lp32 {

class OpencoreImpl : public Opencore {
public:
    OpencoreImpl() : Opencore() {}
    ~OpencoreImpl();
    bool DoCoredump(const char* filename);
    bool NeedFilterFile(const char* filename, int offset);
    void Prepare(const char* filename);
    void ParseProcessMapsVma(int pid);
    void ParserPhdr(int index, uint32_t start, uint32_t end, char* flags, char* filename);
    void ParserNtFile(int index, uint32_t start, uint32_t end, int fileofs, char* filename);
    void CreateCoreHeader();
    void CreateCoreNoteHeader();
    void CreateCoreAUXV(int pid);

    // ELF Header
    void WriteCoreHeader(FILE* fp);

    // Program Headers
    void WriteCoreNoteHeader(FILE* fp);
    void WriteCoreProgramHeaders(FILE* fp);

    // Segments
    void WriteCoreAUXV(FILE* fp);
    void WriteNtFile(FILE* fp);
    void AlignNoteSegment(FILE* fp);
    void WriteCoreLoadSegment(int pid, FILE* fp);

    virtual void CreateCorePrStatus(int pid) = 0;
    virtual void WriteCorePrStatus(FILE* fp) = 0;
protected:
    Elf32_Ehdr ehdr;
    Elf32_Phdr *phdr;
    int phnum;
    Elf32_Phdr note;
    lp32::Auxv *auxv;
    int auxvnum;
    lp32::File *file;
    int fileslen;
};

} // namespace lp32

#endif // PARSER_COMMAND_REMOTE_OPENCORE_LP32_OPENCORE_IMPL_H_
