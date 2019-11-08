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

#include "maya/MFnReference.h"
#include "AL/usdmaya/fileio/translators/TranslatorBase.h"
#include "AL/usdmaya/utils/ForwardDeclares.h"

#include "pxr/usd/usd/stage.h"

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

//----------------------------------------------------------------------------------------------------------------------
/// \brief Class that has the actual test-able logic for maintenance of the Maya Reference.
//----------------------------------------------------------------------------------------------------------------------
class MayaReferenceLogic
{
public:

  MStatus LoadMayaReference(const UsdPrim& prim, MObject& parent, MString& mayaReferencePath, MString& rigNamespaceM) const;
  MStatus UnloadMayaReference(MObject& parent) const;
  MStatus update(const UsdPrim& prim, MObject parent) const;

private:
  MStatus connectReferenceAssociatedNode(MFnDagNode& dagNode, MFnReference& refNode) const;

  static const TfToken m_namespaceName;
  static const TfToken m_referenceName;

  static const char* const m_primNSAttr;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief Class to translate an image plane in and out of maya.
//----------------------------------------------------------------------------------------------------------------------
class MayaReference : public TranslatorBase
{
public:
  AL_USDMAYA_DECLARE_TRANSLATOR(MayaReference);
private:

  MStatus initialize() override;
  MStatus import(const UsdPrim& prim, MObject& parent, MObject& createdObj) override;
  MStatus tearDown(const SdfPath& path) override;
  MStatus update(const UsdPrim& path) override;
  bool supportsUpdate() const override 
    { return true; }

  bool canBeOverridden() override
    { return true; }
    
  MayaReferenceLogic m_mayaReferenceLogic;
};

//----------------------------------------------------------------------------------------------------------------------
} // translators
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
