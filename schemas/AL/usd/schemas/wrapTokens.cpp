//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
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
// language governing permissions and limitations under the Apache License.
//
// GENERATED FILE.  DO NOT EDIT.
#include <boost/python/class.hpp>
#include "./tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// Helper to return a static token as a string.  We wrap tokens as Python
// strings and for some reason simply wrapping the token using def_readonly
// bypasses to-Python conversion, leading to the error that there's no
// Python type for the C++ TfToken type.  So we wrap this functor instead.
class _WrapStaticToken {
public:
    _WrapStaticToken(const TfToken* token) : _token(token) { }

    std::string operator()() const
    {
        return _token->GetString();
    }

private:
    const TfToken* _token;
};

template <typename T>
void
_AddToken(T& cls, const char* name, const TfToken& token)
{
    cls.add_static_property(name,
                            boost::python::make_function(
                                _WrapStaticToken(&token),
                                boost::python::return_value_policy<
                                    boost::python::return_by_value>(),
                                boost::mpl::vector1<std::string>()));
}

} // anonymous

void wrapAL_USDMayaSchemasTokens()
{
    boost::python::class_<AL_USDMayaSchemasTokensType, boost::noncopyable>
        cls("Tokens", boost::python::no_init);
    _AddToken(cls, "lock", AL_USDMayaSchemasTokens->lock);
    _AddToken(cls, "lock_inherited", AL_USDMayaSchemasTokens->lock_inherited);
    _AddToken(cls, "lock_transform", AL_USDMayaSchemasTokens->lock_transform);
    _AddToken(cls, "lock_unlocked", AL_USDMayaSchemasTokens->lock_unlocked);
    _AddToken(cls, "mayaNamespace", AL_USDMayaSchemasTokens->mayaNamespace);
    _AddToken(cls, "mayaReference", AL_USDMayaSchemasTokens->mayaReference);
    _AddToken(cls, "selectability", AL_USDMayaSchemasTokens->selectability);
    _AddToken(cls, "selectability_inherited", AL_USDMayaSchemasTokens->selectability_inherited);
    _AddToken(cls, "selectability_selectable", AL_USDMayaSchemasTokens->selectability_selectable);
    _AddToken(cls, "selectability_unselectable", AL_USDMayaSchemasTokens->selectability_unselectable);
}
