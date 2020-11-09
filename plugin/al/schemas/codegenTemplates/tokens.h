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
#ifndef {{ Upper(tokensPrefix) }}_TOKENS_H
#define {{ Upper(tokensPrefix) }}_TOKENS_H

/// \file {{ libraryName }}/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// 
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
// 
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

{% if useExportAPI %}
#include <pxr/pxr.h>
#include "{{ libraryPath }}/api.h"
{% endif %}
#include <pxr/base/tf/staticTokens.h>

{% if useExportAPI %}
{{ namespaceOpen }}

{% endif %}
// clang-format off
/// \hideinitializer
#define {{ Upper(tokensPrefix) }}_TOKENS \
{% for token in tokens %}
    {% if token.id == token.value -%}({{ token.id }})
    {%- else -%}                     (({{ token.id }}, "{{ token.value}}"))
    {%- endif -%}{% if not loop.last %} \{% endif %}
// clang-format on

{% endfor %}

/// \anchor {{ tokensPrefix }}Tokens
///
/// <b>{{ tokensPrefix }}Tokens</b> provides static, efficient TfToken's for
/// use in all public USD API
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// {{ tokensPrefix }}Tokens also contains all of the \em allowedTokens values declared
/// for schema builtin attributes of 'token' scene description type.
/// Use {{ tokensPrefix }}Tokens like so:
///
/// \code
///     gprim.GetVisibilityAttr().Set({{ tokensPrefix }}Tokens->invisible);
/// \endcode
///
/// The tokens are:
{% for token in tokens %}
/// \li <b>{{ token.id }}</b> - {{ token.desc }}
{% endfor %}
TF_DECLARE_PUBLIC_TOKENS({{ tokensPrefix }}Tokens, {% if useExportAPI %}{{ Upper(libraryName) }}_API, {% endif %}{{ Upper(tokensPrefix) }}_TOKENS);
{% if useExportAPI %}

{{ namespaceClose }}
{% endif %}

#endif
