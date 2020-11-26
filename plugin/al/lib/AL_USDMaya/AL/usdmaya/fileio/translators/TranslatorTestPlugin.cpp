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
#include "AL/usdmaya/fileio/translators/TranslatorTestPlugin.h"

#include <maya/MFn.h>
#include <maya/MFnDagNode.h>

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

AL_USDMAYA_DEFINE_TRANSLATOR(TranslatorTestPlugin, TranslatorTestType);

//----------------------------------------------------------------------------------------------------------------------
MStatus TranslatorTestPlugin::initialize() { return MStatus::kSuccess; }

//----------------------------------------------------------------------------------------------------------------------
MStatus TranslatorTestPlugin::import(const UsdPrim& prim, MObject& parent, MObject& createdObj)
{
    MObject distanceShape = MFnDagNode().create("distanceDimShape", parent);
    createdObj = distanceShape;
    context()->insertItem(prim, createdObj);
    return MStatus::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TranslatorTestPlugin::postImport(const UsdPrim& prim) { return MStatus::kSuccess; }

//----------------------------------------------------------------------------------------------------------------------
MStatus TranslatorTestPlugin::preTearDown(UsdPrim& path) { return MStatus::kSuccess; }

//----------------------------------------------------------------------------------------------------------------------
MStatus TranslatorTestPlugin::tearDown(const SdfPath& path)
{
    context()->removeItems(path);
    return MStatus::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
UsdPrim TranslatorTestPlugin::exportObject(
    UsdStageRefPtr        stage,
    MDagPath              dagPath,
    const SdfPath&        usdPath,
    const ExporterParams& params)
{
    MFnDagNode         fn(dagPath);
    TranslatorTestType node = TranslatorTestType::Define(stage, usdPath);
    return node.GetPrim();
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace translators
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
