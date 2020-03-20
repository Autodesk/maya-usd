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
// Modifications copyright (C) 2020 Autodesk
//

#include "pxr/pxr.h"

#include "../../fileio/primReaderRegistry.h"
#include "../../fileio/translators/translatorMesh.h"

#include "pxr/usd/usdGeom/mesh.h"

PXR_NAMESPACE_OPEN_SCOPE

// Prim reader for mesh
class MayaUsdPrimReaderMesh final : public UsdMayaPrimReader
{
public:
    MayaUsdPrimReaderMesh(const UsdMayaPrimReaderArgs& args) 
        : UsdMayaPrimReader(args) {}

    ~MayaUsdPrimReaderMesh() override {}

    bool Read(UsdMayaPrimReaderContext* context) override;
};

TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaPrimReaderRegistry, UsdGeomMesh) 
{
    UsdMayaPrimReaderRegistry::Register<UsdGeomMesh>(
        [](const UsdMayaPrimReaderArgs& args) {
            return std::make_shared<MayaUsdPrimReaderMesh>(args);
        });
}

bool
MayaUsdPrimReaderMesh::Read(UsdMayaPrimReaderContext* context)
{
    const UsdPrim& usdPrim = _GetArgs().GetUsdPrim();

    auto parentNode = context->GetMayaNode(usdPrim.GetPath().GetParentPath(), true);

    return UsdMayaTranslatorMesh::Create(UsdGeomMesh(usdPrim),
                                         parentNode,
                                         _GetArgs(),
                                         context);
}

PXR_NAMESPACE_CLOSE_SCOPE
