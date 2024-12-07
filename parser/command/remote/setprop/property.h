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

#ifndef PARSER_COMMAND_REMOTE_SETPROP_PROPERTY_H_
#define PARSER_COMMAND_REMOTE_SETPROP_PROPERTY_H_

#include <string>

class AndroidProperty {
public:
    static int Main(int argc, char* const argv[]);
    static void Usage();
    static int Set(std::string& name, std::string& value);
};

#endif  // PARSER_COMMAND_REMOTE_SETPROP_PROPERTY_H_