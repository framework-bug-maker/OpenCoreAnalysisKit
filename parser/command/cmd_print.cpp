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
#include "common/bit.h"
#include "command/cmd_print.h"
#include "base/utils.h"
#include "api/core.h"
#include "runtime/runtime_globals.h"
#include "dex/modifiers.h"
#include "dex/primitive.h"
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <iomanip>

int PrintCommand::main(int argc, char* const argv[]) {
    if (!CoreApi::IsReady()
            || !Android::IsSdkReady()
            || !argc)
        return 0;

    bool binary = false;
    bool reference = false;
    int deep = 0;

    int opt;
    int option_index = 0;
    static struct option long_options[] = {
        {"--binary",  no_argument,       0,  'b'},
        {"--ref",     required_argument, 0,  'r'},
        {0,           0,                 0,   0 }
    };
    
    while ((opt = getopt_long(argc, argv, "br::",
                long_options, &option_index)) != -1) {
        switch (opt) {
            case 'b':
                binary = true;
                break;
            case 'r':
                reference = true;
                if (optind < argc) deep = atoi(optarg);
                break;
        }
    }

    // reset
    optind = 0;

    art::mirror::Object ref = Utils::atol(argv[0]);
    DumpObject(ref);
    return 0;
}

void PrintCommand::DumpObject(art::mirror::Object& object) {
    uint64_t size = object.SizeOf();
    uint64_t real_size = RoundUp(size, art::kObjectAlignment);
    LOGI("Size: 0x%lx\n", real_size);
    if (size != real_size) {
        LOGI("Padding: 0x%lx\n", real_size - size);
    }

    art::mirror::Class clazz = object.GetClass();
    if (clazz.Ptr()) {
        if (object.IsClass()) {
            art::mirror::Class thiz = object;
            DumpClass(thiz);
        } else if (clazz.IsArrayClass()) {
            art::mirror::Array thiz = object;
            DumpArray(thiz);
        } else {
            DumpInstance(object);
        }
    }
}

void PrintCommand::DumpClass(art::mirror::Class& clazz) {
    LOGI("Class Name: %s\n", clazz.PrettyDescriptor().c_str());

    std::string format = FormatSize(clazz.SizeOf());
    art::mirror::Class current = clazz;
    auto callback = [&](art::ArtField& field) -> bool {
        PrintCommand::PrintField(format.c_str(), current, clazz, field);
        return false;
    };

    Android::ForeachStaticField(current, callback);
    current = clazz.GetClass();
    LOGI("  info %s\n", current.PrettyDescriptor().c_str());
    Android::ForeachInstanceField(current, callback);
    current = current.GetSuperClass();
    LOGI("  extends %s\n", current.PrettyDescriptor().c_str());
    Android::ForeachInstanceField(current, callback);
}

void PrintCommand::DumpArray(art::mirror::Array& array) {
    art::mirror::Class clazz = array.GetClass();
    LOGI("Array Name: %s\n", clazz.PrettyDescriptor().c_str());

    uint32_t length = array.GetLength();
    if (array.IsObjectArray()) {
        for (int32_t i = 0; i < length; ++i) {
            api::MemoryRef ref(array.GetRawData(sizeof(uint32_t), i), array);
            art::mirror::Object object(*reinterpret_cast<uint32_t *>(ref.Real()), array);
            if (object.Ptr() && object.IsString()) {
                art::mirror::String str = object;
                LOGI("    [%d] %s\n", i, str.ToModifiedUtf8().c_str());
            } else {
                LOGI("    [%d] 0x%lx\n", i, object.Ptr());
            }
        }
    } else {
        uint64_t size;
        art::mirror::Class component = clazz.GetComponentType();
        Android::BasicType type = Android::SignatureToBasicTypeAndSize(art::Primitive::Descriptor(component.GetPrimitiveType()), &size);
        for (int32_t i = 0; i < length; ++i) {
            switch (size) {
                case 1: {
                    api::MemoryRef ref(array.GetRawData(sizeof(uint8_t), i), array);
                    PrintArrayElement(i, type, ref);
                } break;
                case 2: {
                    api::MemoryRef ref(array.GetRawData(sizeof(uint16_t), i), array);
                    PrintArrayElement(i, type, ref);
                } break;
                case 4: {
                    api::MemoryRef ref(array.GetRawData(sizeof(uint32_t), i), array);
                    PrintArrayElement(i, type, ref);
                } break;
                case 8: {
                    api::MemoryRef ref(array.GetRawData(sizeof(uint64_t), i), array);
                    PrintArrayElement(i, type, ref);
                } break;
            }
        }
    }
}

void PrintCommand::DumpInstance(art::mirror::Object& object) {
    art::mirror::Class clazz = object.GetClass();
    LOGI("Object Name: %s\n", clazz.PrettyDescriptor().c_str());

    std::string format = FormatSize(object.SizeOf());
    art::mirror::Class super = clazz;
    do {
        if (clazz != super) {
            LOGI("  extends %s\n", super.PrettyDescriptor().c_str());
        }

        auto callback = [&](art::ArtField& field) -> bool {
            PrintCommand::PrintField(format.c_str(), super, object, field);
            return false;
        };
        Android::ForeachInstanceField(super, callback);

        if (super.IsStringClass()) {
            art::mirror::String str = object;
            if (str.GetLength() != 0) {
                LOGI("[%s]\n", str.ToModifiedUtf8().c_str());
            }
        }

        super = super.GetSuperClass();
    } while (super.Ptr());
}

void PrintCommand::PrintField(const char* format, art::mirror::Class& clazz, art::mirror::Object& object, art::ArtField& field) {
    uint64_t size;
    const char* sig = field.GetTypeDescriptor();
    Android::BasicType type = Android::SignatureToBasicTypeAndSize(sig, &size, "B");
    LOGI(format, field.offset(), art::PrettyJavaAccessFlags(field.access_flags()).c_str(), field.PrettyTypeDescriptor().c_str(), field.GetName());
    switch (type) {
        case Android::basic_byte:
            LOGI(" = 0x%x\n", field.GetByte(object));
            break;
        case Android::basic_boolean:
            LOGI(" = %s\n", field.GetBoolean(object) ? "true" : "false");
            break;
        case Android::basic_char:
            LOGI(" = 0x%c\n", field.GetChar(object));
            break;
        case Android::basic_short:
            LOGI(" = 0x%x\n", field.GetShort(object));
            break;
        case Android::basic_int:
            if (field.offset() == OFFSET(String, count_) && clazz.IsStringClass()) {
                art::mirror::String str = object;
                LOGI(" = 0x%x\n", str.GetLength());
                break;
            } else {
                LOGI(" = 0x%x\n", field.GetInt(object));
            }
            break;
        case Android::basic_float:
            LOGI(" = 0x%f\n", field.GetFloat(object));
            break;
        case Android::basic_object: {
            art::mirror::Object tmp(field.GetObj(object), object);
            if (tmp.Ptr() && tmp.IsString()) {
                art::mirror::String str = tmp;
                LOGI(" = %s\n", str.ToModifiedUtf8().c_str());
            } else {
                LOGI(" = 0x%lx\n", tmp.Ptr());
            }
        } break;
        case Android::basic_double:
            LOGI(" = 0x%lf\n", field.GetDouble(object));
            break;
        case Android::basic_long:
            LOGI(" = 0x%lx\n", field.GetLong(object));
            break;
    }
}

std::string PrintCommand::FormatSize(uint64_t size) {
    std::string format;
    format.append("    [0x%0");
    int num = 0;
    uint64_t current = size;
    do {
        current = current / 0xF;
        ++num;
    } while(current != 0);
    format.append(std::to_string(num));
    format.append("x] %s%s %s");
    return format;
}

void PrintCommand::PrintArrayElement(uint32_t i, Android::BasicType type, api::MemoryRef& ref) {
    switch (static_cast<uint32_t>(type)) {
        case Android::basic_byte:
            LOGI("    [%d] 0x%x\n", i, *reinterpret_cast<int8_t *>(ref.Real()));
            break;
        case Android::basic_boolean:
            LOGI("    [%d] %s\n", i, *reinterpret_cast<uint8_t *>(ref.Real()) ? "true" : "false");
            break;
        case Android::basic_char:
            LOGI("    [%d] 0x%x\n", i, *reinterpret_cast<uint16_t *>(ref.Real()));
            break;
        case Android::basic_short:
            LOGI("    [%d] 0x%x\n", i, *reinterpret_cast<int16_t *>(ref.Real()));
            break;
        case Android::basic_int:
            LOGI("    [%d] 0x%x\n", i, *reinterpret_cast<int32_t *>(ref.Real()));
            break;
        case Android::basic_float:
            LOGI("    [%d] %f\n", i, (double)*reinterpret_cast<uint32_t *>(ref.Real()));
            break;
        case Android::basic_double:
            LOGI("    [%d] %Lf\n", i, (long double)*reinterpret_cast<uint64_t *>(ref.Real()));
            break;
        case Android::basic_long:
            LOGI("    [%d] %lx\n", i, *reinterpret_cast<uint64_t *>(ref.Real()));
            break;
    }
}

void PrintCommand::usage() {
    LOGI("Usage: print|p object -[br]\n");
}
