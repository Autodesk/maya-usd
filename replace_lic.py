#!/usr/bin/env python

'''Find / replace all files that have the "old" pixar-style apache license with
 the new autodesk-style apache license'''

from __future__ import print_function

import argparse
import inspect
import os

THIS_FILE = os.path.normpath(os.path.abspath(inspect.getsourcefile(lambda: None)))

parser = argparse.ArgumentParser(description=__doc__)
mode_group = parser.add_mutually_exclusive_group()
mode_group.add_argument('--mode', choices=['pxr', 'al', 'both'], default='both',
    help="Which repo we're trying to replace lics for")
mode_group.add_argument('--al', help='Replace al lics',
    action='store_const', const='al', dest='mode')
mode_group.add_argument('--pxr', help='Replace pixar lics',
    action='store_const', const='pxr', dest='mode')

args = parser.parse_args()

old_pxr_cpp_lic = """// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License."""

old_pxr_py_lic = """#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#
"""

old_al_py_lic = """# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.# 
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# 
"""

new_cpp_lic = """// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License."""

new_py_lic = """#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
"""

old_al_cpp_lic_line = """// you may not use this file except in compliance with the License.//"""
new_cpp_lic_line = """// you may not use this file except in compliance with the License."""

old_al_py_lic_line = """# you may not use this file except in compliance with the License.//"""
new_py_lic_line = """# you may not use this file except in compliance with the License."""

if args.mode == 'pxr':
    replacement_pairs = [
        (old_pxr_cpp_lic, new_cpp_lic),
        (old_pxr_py_lic, new_py_lic),
    ]
elif args.mode == 'al':
    replacement_pairs = [
        (old_al_cpp_lic_line, new_cpp_lic_line),
        (old_al_py_lic, new_py_lic),
        (old_pxr_py_lic, new_py_lic),
        (old_al_py_lic_line, new_py_lic_line),
    ]
elif args.mode == 'both':
    replacement_pairs = [
        (old_pxr_cpp_lic, new_cpp_lic),
        (old_pxr_py_lic, new_py_lic),
        (old_al_cpp_lic_line, new_cpp_lic_line),
        (old_al_py_lic, new_py_lic),
        (old_al_py_lic_line, new_py_lic_line),
    ]
else:
    raise ValueError("Unrecognized mode: {}".format(args.mode))

for dirpath, dirnames, filenames in os.walk('.'):
    for filename in filenames:
        filepath = os.path.normpath(os.path.abspath(os.path.join(dirpath, filename)))
        if filepath == THIS_FILE:
            continue
        with open(filepath, 'rb') as f:
            text = f.read()
        altered = False
        new_text = text
        for old_lic, new_lic in replacement_pairs:
            if old_lic in new_text:
                new_text = new_text.replace(old_lic, new_lic)
                altered = True
        if altered:
            with open(filepath, 'wb') as f:
                f.write(new_text)
            print("Replaced license in: {}".format(filepath))
