//
// Copyright 2017 Animal Logic
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
#include "Metadata.h"

namespace AL {
namespace usdmaya {

const TfToken Metadata::transformType("al_usdmaya_transformType");
const TfToken Metadata::excludeFromProxyShape("al_usdmaya_excludeFromProxyShape");
const TfToken Metadata::importAsNative("al_usdmaya_importAsNative");

const TfToken Metadata::selectability("al_usdmaya_selectability");
const TfToken Metadata::selectable("selectable");
const TfToken Metadata::unselectable("unselectable");

const TfToken Metadata::mergedTransform("al_usdmaya_mergedTransform");
const TfToken Metadata::unmerged("unmerged");

const TfToken Metadata::locked("al_usdmaya_lock");
const TfToken Metadata::lockTransform("transform");
const TfToken Metadata::lockInherited("inherited");
const TfToken Metadata::lockUnlocked("unlocked");

const TfToken Metadata::assetType("assettype");
//----------------------------------------------------------------------------------------------------------------------
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
