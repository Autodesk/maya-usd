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
#include "AL/usdmaya/utils/ForwardDeclares.h"

#include <cstdint>

// forward declare usd types
namespace AL {

namespace usdmaya {
struct guid;
class Global;
struct MObjectMap;
class StageCache;

namespace cmds {
struct CompareLayerHandle;
class LayerCommandBase;
class LayerConstructTree;
class LayerCreateSubLayer;
class LayerCurrentEditTarget;
class LayerExport;
class LayerGetLayers;
class LayerSave;
class LayerSetMuted;
class ProxyShapeCommandBase;
class ProxyShapeImport;
class ProxyShapeFindLoadable;
class ProxyShapeImportAllTransforms;
class ProxyShapeImportPrimPathAsMaya;
class ProxyShapePostLoadProcess;
class ProxyShapePrintRefCountState;
class ProxyShapeRemoveAllTransforms;
class TransformationMatrixToggleTimeSource;
} // namespace cmds

namespace fileio {
class AnimationTranslator;
class Export;
class ExportCommand;
class ExportTranslator;
struct ExporterParams;
class Import;
class ImportCommand;
class ImportTranslator;
struct ImporterParams;
struct NativeTranslatorRegistry;
struct NodeFactory;
class TransformIterator;
namespace translators {
class DagNodeTranslator;
class DgNodeTranslator;
class TransformTranslator;
} // namespace translators
} // namespace fileio

namespace nodes {
class Layer;
class ProxyShape;
class SchemaNodeRefDB;
class Transform;
class Scope;
class TransformationMatrix;
class BasicTransformationMatrix;
} // namespace nodes

} // namespace usdmaya
} // namespace AL
