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
#pragma once
#include <maya/MTypeId.h>
#include <maya/MString.h>

namespace AL {
namespace usdmaya {

// The type id's for the maya nodes
const MTypeId AL_USDMAYA_PROXYSHAPE(0x00112A20);
const MTypeId AL_USDMAYA_TRANSFORM(0x00112A21);
const MTypeId AL_USDMAYA_TRANSFORMATION_MATRIX(0x00112A22);
const MTypeId AL_USDMAYA_LAYER(0x00112A23);
const MTypeId AL_USDMAYA_STAGEDATA(0x00112A24);
const MTypeId AL_USDMAYA_USDPREVIEWSURFACE(0x00112A25);
const MTypeId AL_USDMAYA_LAYERMANAGER(0x00112A27);
const MTypeId AL_USDMAYA_RENDERERMANAGER(0x00112A28);
const MTypeId AL_USDMAYA_USDGEOMCAMERAPROXY(0x00112A2B);
const MTypeId AL_USDMAYA_SCOPE(0x00112A31);
const MTypeId AL_USDMAYA_IDENTITY_MATRIX(0x00112A32);

const MString AL_USDMAYA_PLUGIN_REGISTRANT_ID("AL_USDMayaPlugin");

#if defined(WANT_UFE_BUILD)
const int  MAYA_UFE_RUNTIME_ID(1);
const char MAYA_UFE_SEPARATOR('|');
const int  USD_UFE_RUNTIME_ID(2);
const char USD_UFE_SEPARATOR('/');
#endif

} // namespace usdmaya
} // namespace AL
