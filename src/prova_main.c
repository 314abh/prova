/* Copyright 2026 Abhigyan Kumar Abhigyan Kumar <314abh at gmail dot com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "prova.h"

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    prova_run_tests(p_registry);
    prova_print_summary(p_registry);
    return (p_metadata.failing_tests + p_metadata.crashing_tests) > 0 ? 1 : 0;
}
