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
// GENERATED FILE.  DO NOT EDIT.
#include "tokens.h"

#include <boost/python/class.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// Helper to return a static token as a string.  We wrap tokens as Python
// strings and for some reason simply wrapping the token using def_readonly
// bypasses to-Python conversion, leading to the error that there's no
// Python type for the C++ TfToken type.  So we wrap this functor instead.
class _WrapStaticToken
{
public:
    _WrapStaticToken(const TfToken* token)
        : _token(token)
    {
    }

    std::string operator()() const { return _token->GetString(); }

private:
    const TfToken* _token;
};

template <typename T> void _AddToken(T& cls, const char* name, const TfToken& token)
{
    cls.add_static_property(
        name,
        boost::python::make_function(
            _WrapStaticToken(&token),
            boost::python::return_value_policy<boost::python::return_by_value>(),
            boost::mpl::vector1<std::string>()));
}

} // namespace

void wrapAL_USDMayaSchemasTokens()
{
    boost::python::class_<AL_USDMayaSchemasTokensType, boost::noncopyable> cls(
        "Tokens", boost::python::no_init);
    _AddToken(cls, "animationEndFrame", AL_USDMayaSchemasTokens->animationEndFrame);
    _AddToken(cls, "animationStartFrame", AL_USDMayaSchemasTokens->animationStartFrame);
    _AddToken(cls, "currentFrame", AL_USDMayaSchemasTokens->currentFrame);
    _AddToken(cls, "endFrame", AL_USDMayaSchemasTokens->endFrame);
    _AddToken(cls, "lock", AL_USDMayaSchemasTokens->lock);
    _AddToken(cls, "lock_inherited", AL_USDMayaSchemasTokens->lock_inherited);
    _AddToken(cls, "lock_transform", AL_USDMayaSchemasTokens->lock_transform);
    _AddToken(cls, "lock_unlocked", AL_USDMayaSchemasTokens->lock_unlocked);
    _AddToken(cls, "mergedTransform", AL_USDMayaSchemasTokens->mergedTransform);
    _AddToken(cls, "mergedTransform_unmerged", AL_USDMayaSchemasTokens->mergedTransform_unmerged);
    _AddToken(cls, "selectability", AL_USDMayaSchemasTokens->selectability);
    _AddToken(cls, "selectability_inherited", AL_USDMayaSchemasTokens->selectability_inherited);
    _AddToken(cls, "selectability_selectable", AL_USDMayaSchemasTokens->selectability_selectable);
    _AddToken(
        cls, "selectability_unselectable", AL_USDMayaSchemasTokens->selectability_unselectable);
    _AddToken(cls, "startFrame", AL_USDMayaSchemasTokens->startFrame);
}
