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
#include "./tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

AL_USDMayaSchemasTokensType::AL_USDMayaSchemasTokensType() :
    lock("al_usdmaya_lock", TfToken::Immortal),
    lock_inherited("inherited", TfToken::Immortal),
    lock_transform("transform", TfToken::Immortal),
    lock_unlocked("unlocked", TfToken::Immortal),
    mayaNamespace("mayaNamespace", TfToken::Immortal),
    mayaReference("mayaReference", TfToken::Immortal),
    selectability("al_usdmaya_selectability", TfToken::Immortal),
    selectability_inherited("inherited", TfToken::Immortal),
    selectability_selectable("selectable", TfToken::Immortal),
    selectability_unselectable("unselectable", TfToken::Immortal),
    allTokens({
        lock,
        lock_inherited,
        lock_transform,
        lock_unlocked,
        mayaNamespace,
        mayaReference,
        selectability,
        selectability_inherited,
        selectability_selectable,
        selectability_unselectable
    })
{
}

TfStaticData<AL_USDMayaSchemasTokensType> AL_USDMayaSchemasTokens;

PXR_NAMESPACE_CLOSE_SCOPE
