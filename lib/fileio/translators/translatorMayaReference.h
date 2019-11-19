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

//
// Copyright 2017 Animal Logic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.//
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

#ifndef PXRUSDMAYA_TRANSLATOR_GPRIM_H
#define PXRUSDMAYA_TRANSLATOR_GPRIM_H

/// \file translatorReference.h

#include "../../base/api.h"
#include "../primReaderContext.h"
#include "../primWriterContext.h"

#include "pxr/pxr.h"

#include "pxr/usd/usd/prim.h"

#include <maya/MObject.h>
#include <maya/MFnReference.h>

PXR_NAMESPACE_OPEN_SCOPE


/// \brief Provides helper functions for reading UsdGeomGprim.
struct UsdMayaTranslatorMayaReference
{
    MAYAUSD_CORE_PUBLIC
    static MStatus LoadMayaReference(const UsdPrim& prim, MObject& parent, MString& mayaReferencePath, MString& rigNamespaceM);
    
    MAYAUSD_CORE_PUBLIC
    static MStatus UnloadMayaReference(const MObject& parent);
    
    MAYAUSD_CORE_PUBLIC
    static MStatus update(const UsdPrim& prim, MObject parent);

private:
    static MStatus connectReferenceAssociatedNode(MFnDagNode& dagNode, MFnReference& refNode);

    static const TfToken m_namespaceName;
    static const TfToken m_referenceName;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif

