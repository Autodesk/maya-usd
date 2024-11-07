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
#include <mayaUsd_Schemas/tokens.h>

#include <pxr_python.h>

PXR_NAMESPACE_USING_DIRECTIVE

#define _ADD_TOKEN(cls, name) \
    cls.add_static_property(  \
        #name, +[]() { return MayaUsd_SchemasTokens->name.GetString(); });

void wrapMayaUsd_SchemasTokens()
{
    PXR_BOOST_PYTHON_NAMESPACE::
        class_<MayaUsd_SchemasTokensType, PXR_BOOST_PYTHON_NAMESPACE::noncopyable>
            cls("Tokens", PXR_BOOST_PYTHON_NAMESPACE::no_init);
    _ADD_TOKEN(cls, mayaAutoEdit);
    _ADD_TOKEN(cls, mayaNamespace);
    _ADD_TOKEN(cls, mayaReference);
    _ADD_TOKEN(cls, ALMayaReference);
    _ADD_TOKEN(cls, MayaReference);
}
