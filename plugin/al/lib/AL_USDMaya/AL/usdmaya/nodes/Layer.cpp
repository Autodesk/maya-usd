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
#include "AL/usdmaya/nodes/Layer.h"

#include "AL/usdmaya/DebugCodes.h"
#include "AL/usdmaya/TypeIDs.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace nodes {

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_NODE(Layer, AL_USDMAYA_LAYER, AL_usdmaya);

MObject Layer::m_comment = MObject::kNullObj;
MObject Layer::m_defaultPrim = MObject::kNullObj;
MObject Layer::m_documentation = MObject::kNullObj;
MObject Layer::m_startTime = MObject::kNullObj;
MObject Layer::m_endTime = MObject::kNullObj;
MObject Layer::m_timeCodesPerSecond = MObject::kNullObj;
MObject Layer::m_framePrecision = MObject::kNullObj;
MObject Layer::m_owner = MObject::kNullObj;
MObject Layer::m_sessionOwner = MObject::kNullObj;
MObject Layer::m_permissionToEdit = MObject::kNullObj;
MObject Layer::m_permissionToSave = MObject::kNullObj;

// connection to layers and proxy shapes
MObject Layer::m_subLayers = MObject::kNullObj;
MObject Layer::m_childLayers = MObject::kNullObj;
MObject Layer::m_parentLayer = MObject::kNullObj;
MObject Layer::m_proxyShape = MObject::kNullObj;

// read only identification
MObject Layer::m_displayName = MObject::kNullObj;
MObject Layer::m_realPath = MObject::kNullObj;
MObject Layer::m_fileExtension = MObject::kNullObj;
MObject Layer::m_version = MObject::kNullObj;
MObject Layer::m_repositoryPath = MObject::kNullObj;
MObject Layer::m_assetName = MObject::kNullObj;

// serialisation
MObject Layer::m_nameOnLoad = MObject::kNullObj;
MObject Layer::m_serialized = MObject::kNullObj;
MObject Layer::m_hasBeenEditTarget = MObject::kNullObj;

//----------------------------------------------------------------------------------------------------------------------
MStatus Layer::initialise()
{
    TF_DEBUG(ALUSDMAYA_LAYERS).Msg("Layer::initialise\n");
    try {
        setNodeType(kTypeName);
        addFrame("USD Layer Info");

        // do not write these nodes to the file. They will be created automagically by the proxy
        // shape
        m_comment = addStringAttr("comment", "cm", kReadable | kWritable);
        m_defaultPrim = addStringAttr("defaultPrim", "dp", kReadable | kWritable);
        m_documentation = addStringAttr("documentation", "docs", kReadable | kWritable);
        m_startTime = addDoubleAttr("startTime", "stc", 0, kReadable | kWritable);
        m_endTime = addDoubleAttr("endTime", "etc", 0, kReadable | kWritable);
        m_timeCodesPerSecond
            = addDoubleAttr("timeCodesPerSecond", "tcps", 0, kReadable | kWritable);
        m_framePrecision = addInt32Attr("framePrecision", "fp", 0, kReadable | kWritable);
        m_owner = addStringAttr("owner", "own", kReadable | kWritable);
        m_sessionOwner = addStringAttr("sessionOwner", "sho", kReadable | kWritable);
        m_permissionToEdit = addBoolAttr("permissionToEdit", "pte", false, kReadable | kWritable);
        m_permissionToSave = addBoolAttr("permissionToSave", "pts", false, kReadable | kWritable);

        // parent/child relationships
        m_proxyShape = addMessageAttr(
            "proxyShape", "psh", kConnectable | kReadable | kWritable | kHidden | kStorable);
        m_subLayers = addMessageAttr(
            "subLayers",
            "sl",
            kConnectable | kReadable | kWritable | kHidden | kArray | kUsesArrayDataBuilder
                | kStorable);
        m_parentLayer = addMessageAttr(
            "parentLayer", "pl", kConnectable | kReadable | kWritable | kHidden | kStorable);
        m_childLayers = addMessageAttr(
            "childLayer",
            "cl",
            kConnectable | kReadable | kWritable | kHidden | kArray | kUsesArrayDataBuilder
                | kStorable);

        addFrame("USD Layer Identification");
        m_displayName = addStringAttr("displayName", "dn", kReadable | kWritable);
        m_realPath = addStringAttr("realPath", "rp", kReadable | kWritable);
        m_fileExtension = addStringAttr("fileExtension", "fe", kReadable | kWritable);
        m_version = addStringAttr("version", "ver", kWritable | kReadable);
        m_repositoryPath = addStringAttr("repositoryPath", "rpath", kReadable | kWritable);
        m_assetName = addStringAttr("assetName", "an", kReadable | kWritable);

        // add attributes to store the serialisation info
        m_serialized
            = addStringAttr("serialised", "szd", kReadable | kWritable | kStorable | kHidden);
        m_nameOnLoad
            = addStringAttr("nameOnLoad", "nol", kReadable | kWritable | kStorable | kHidden);
        m_hasBeenEditTarget = addBoolAttr(
            "hasBeenEditTarget", "hbet", false, kReadable | kWritable | kStorable | kHidden);
    } catch (const MStatus& status) {
        return status;
    }
    generateAETemplate();
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace nodes
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
