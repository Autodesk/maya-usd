# 
# Copyright 2017 Animal Logic
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

from pxr import Tf
if hasattr(Tf, 'PreparePythonModule'):
    Tf.PreparePythonModule('_AL_USDMaya')
else:
    from . import _AL_USDMaya
    Tf.PrepareModule(_AL_USDMaya, locals())
    del _AL_USDMaya
del Tf

try:
    import __DOC
    __DOC.Execute(locals())
    del __DOC
except Exception:
    pass
