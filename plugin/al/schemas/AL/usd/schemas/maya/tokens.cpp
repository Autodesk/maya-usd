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

PXR_NAMESPACE_OPEN_SCOPE

AL_USDMayaSchemasTokensType::AL_USDMayaSchemasTokensType()
    : animationEndFrame("animationEndFrame", TfToken::Immortal)
    , animationStartFrame("animationStartFrame", TfToken::Immortal)
    , currentFrame("currentFrame", TfToken::Immortal)
    , endFrame("endFrame", TfToken::Immortal)
    , lock("al_usdmaya_lock", TfToken::Immortal)
    , lock_inherited("inherited", TfToken::Immortal)
    , lock_transform("transform", TfToken::Immortal)
    , lock_unlocked("unlocked", TfToken::Immortal)
    , mergedTransform("al_usdmaya_mergedTransform", TfToken::Immortal)
    , mergedTransform_unmerged("unmerged", TfToken::Immortal)
    , selectability("al_usdmaya_selectability", TfToken::Immortal)
    , selectability_inherited("inherited", TfToken::Immortal)
    , selectability_selectable("selectable", TfToken::Immortal)
    , selectability_unselectable("unselectable", TfToken::Immortal)
    , startFrame("startFrame", TfToken::Immortal)
    , allTokens({ animationEndFrame,
                  animationStartFrame,
                  currentFrame,
                  endFrame,
                  lock,
                  lock_inherited,
                  lock_transform,
                  lock_unlocked,
                  mergedTransform,
                  mergedTransform_unmerged,
                  selectability,
                  selectability_inherited,
                  selectability_selectable,
                  selectability_unselectable,
                  startFrame })
{
}

TfStaticData<AL_USDMayaSchemasTokensType> AL_USDMayaSchemasTokens;

PXR_NAMESPACE_CLOSE_SCOPE
