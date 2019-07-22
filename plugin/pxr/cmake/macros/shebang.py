#
# Copyright 2017 Pixar
#
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
#
# Usage:
#   shebang shebang-str source.py dest.py
#   shebang file output.cmd
#
# The former substitutes '/pxrpythonsubst' in <source.py> with <shebang-str>
# and writes the result to <dest.py>.  The latter generates a file named
# <output.cmd> with the contents '@python "%~dp0<file>"';  this is a
# Windows command script to execute "python <file>" where <file> is in
# the same directory as the command script.

import sys

if len(sys.argv) < 3 or len(sys.argv) > 4:
    print "Usage: %s {shebang-str source.py dest|file output.cmd}" % sys.argv[0]
    sys.exit(1)

if len(sys.argv) == 3:
    with open(sys.argv[2], 'w') as f:
        print >>f, '@python "%%~dp0%s" %%*' % (sys.argv[1], )

else:
    with open(sys.argv[2], 'r') as s:
        with open(sys.argv[3], 'w') as d:
            for line in s:
                d.write(line.replace('/pxrpythonsubst', sys.argv[1]))
