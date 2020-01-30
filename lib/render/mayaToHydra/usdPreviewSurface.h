//
// Copyright 2019 Luma Pictures
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
#ifndef __MTOH_USD_PREVIEW_SURFACE_H__
#define __MTOH_USD_PREVIEW_SURFACE_H__

#include <pxr/pxr.h>

#include <maya/MPxNode.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>

PXR_NAMESPACE_OPEN_SCOPE

class MtohUsdPreviewSurface : public MPxNode {
public:
    static const MString classification;
    static const MString name;
    static const MTypeId typeId;
    static void* Creator() { return new MtohUsdPreviewSurface(); }
    static MStatus Initialize();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __MTOH_USD_PREVIEW_SURFACE_H__
