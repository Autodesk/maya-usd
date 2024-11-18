//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "tokens.h"

#include <pxr_python.h>

PXR_NAMESPACE_USING_DIRECTIVE

#define _ADD_TOKEN(cls, name) \
    cls.add_static_property(  \
        #name, +[]() { return AL_USDMayaSchemasTokens->name.GetString(); });

void wrapAL_USDMayaSchemasTokens()
{
    PXR_BOOST_PYTHON_NAMESPACE::
        class_<AL_USDMayaSchemasTokensType, PXR_BOOST_PYTHON_NAMESPACE::noncopyable>
            cls("Tokens", PXR_BOOST_PYTHON_NAMESPACE::no_init);
    _ADD_TOKEN(cls, animationEndFrame);
    _ADD_TOKEN(cls, animationStartFrame);
    _ADD_TOKEN(cls, currentFrame);
    _ADD_TOKEN(cls, endFrame);
    _ADD_TOKEN(cls, lock);
    _ADD_TOKEN(cls, lock_inherited);
    _ADD_TOKEN(cls, lock_transform);
    _ADD_TOKEN(cls, lock_unlocked);
    _ADD_TOKEN(cls, mergedTransform);
    _ADD_TOKEN(cls, mergedTransform_unmerged);
    _ADD_TOKEN(cls, selectability);
    _ADD_TOKEN(cls, selectability_inherited);
    _ADD_TOKEN(cls, selectability_selectable);
    _ADD_TOKEN(cls, selectability_unselectable);
    _ADD_TOKEN(cls, startFrame);
}
