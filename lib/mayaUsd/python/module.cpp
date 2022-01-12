//
// Copyright 2021 Autodesk
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
#include <pxr/base/tf/pyModule.h>
#include <pxr/pxr.h>

// UFE v3 used as an indicator of support for edit as Maya.
#if defined(WANT_UFE_BUILD)
#include <ufe/ufe.h>
#endif

PXR_NAMESPACE_USING_DIRECTIVE

TF_WRAP_MODULE
{
    TF_WRAP(Adaptor);
    TF_WRAP(BlockSceneModificationContext);
    TF_WRAP(ColorSpace);
    TF_WRAP(Converter);
    TF_WRAP(ConverterArgs);
    TF_WRAP(DiagnosticDelegate);
    TF_WRAP(MeshWriteUtils);
#ifdef UFE_V3_FEATURES_AVAILABLE
    TF_WRAP(PrimUpdater);
    TF_WRAP(PrimUpdaterArgs);
    TF_WRAP(PrimUpdaterContext);
    TF_WRAP(PrimUpdaterManager);
#endif
    TF_WRAP(OpUndoItem);
    TF_WRAP(Query);
    TF_WRAP(ReadUtil);
    TF_WRAP(RoundTripUtil);
    TF_WRAP(StageCache);
    TF_WRAP(Tokens);
    TF_WRAP(UsdUndoManager);
    TF_WRAP(UserTaggedAttribute);
    TF_WRAP(WriteUtil);
    TF_WRAP(XformStack);

    TF_WRAP(OpenMaya);
    TF_WRAP(PrimReaderContext);
    TF_WRAP(PrimReaderArgs);
    TF_WRAP(PrimReader);
    TF_WRAP(JobExportArgs);
    TF_WRAP(JobImportArgs);
    TF_WRAP(PrimWriter);
    TF_WRAP(ShaderWriter);
    TF_WRAP(ShadingModeImportContext);
    TF_WRAP(ShaderReader);
    TF_WRAP(ExportChaser);
    TF_WRAP(ExportChaserRegistryFactoryContext);
    TF_WRAP(ImportChaser);
    TF_WRAP(ImportChaserRegistryFactoryContext);
    TF_WRAP(JobContextRegistry);
    TF_WRAP(SchemaApiAdaptor);
    TF_WRAP(ShadingUtil);
    TF_WRAP(ShadingMode);
}
