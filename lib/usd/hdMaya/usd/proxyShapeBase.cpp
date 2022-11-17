//
// Copyright 2018 Pixar
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

#if defined(MAYAUSD_VERSION)
    #error This file should not be included when MAYAUSD_VERSION is defined
#endif

#include <hdMaya/usd/proxyShapeBase.h>

#include <pxr/base/tf/instantiateType.h>

#include <maya/MDagPath.h>
#include <maya/MString.h>
#include <maya/MObject.h>

#include <maya/MPxGeometryData.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnDagNode.h>

PXR_NAMESPACE_OPEN_SCOPE

const MString MayaUsdProxyShapeBase::typeName("MayaUsdProxyShapeBase");

class MayaUsdStageData : public MPxGeometryData
{
public:
    /**
     * \name data
     */
    //@{

    UsdStageRefPtr stage;

    SdfPath primPath;

    //@}

protected:
    MayaUsdStageData() = default;
    ~MayaUsdStageData() override = default;
};

UsdStageRefPtr MayaUsdProxyShapeBase::getUsdStage() const
{
    MStatus          localStatus;
    MayaUsdProxyShapeBase* nonConstThis = const_cast<MayaUsdProxyShapeBase*>(this);
    MDataBlock             dataBlock = nonConstThis->forceCache();

    MFnDependencyNode fn(thisMObject(), &localStatus);
    MDataHandle outDataHandle = dataBlock.inputValue(fn.findPlug("outStageDataAttr").attribute(), &localStatus);
    CHECK_MSTATUS_AND_RETURN(localStatus, UsdStageRefPtr());

    MayaUsdStageData* outData = dynamic_cast<MayaUsdStageData*>(outDataHandle.asPluginData());

    if (outData && outData->stage)
        return outData->stage;
    else
        return {};
}

UsdTimeCode MayaUsdProxyShapeBase::getTime() const
{
    MStatus localStatus;
    MayaUsdProxyShapeBase* nonConstThis = const_cast<MayaUsdProxyShapeBase*>(this);
    MDataBlock             dataBlock = nonConstThis->forceCache();
    MFnDependencyNode fn(thisMObject(), &localStatus);
    return UsdTimeCode(dataBlock.inputValue(fn.findPlug("outTimeAttr").attribute(), &localStatus).asTime().value());
}

MDagPath MayaUsdProxyShapeBase::parentTransform()
{
    MFnDagNode fn(thisMObject());
    MDagPath   proxyTransformPath;
    fn.getPath(proxyTransformPath);
    proxyTransformPath.pop();
    return proxyTransformPath;
}

PXR_NAMESPACE_CLOSE_SCOPE
