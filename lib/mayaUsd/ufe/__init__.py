#
# Copyright 2020 Autodesk
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
    Tf.PreparePythonModule('_ufe')
else:
    from . import _ufe
    Tf.PrepareModule(_ufe, locals())
    del _ufe
del Tf

# TEMP (UsdUfe)
# Import all the names from the usdUfe module into this (mayaUsd.ufe) module.
# During the transition this helps by not having to update all the maya-usd tests.
from usdUfe import *